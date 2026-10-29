
from queue import Queue
from threading import Thread, Event

import time
import numpy as np
import torch

from qwen_tts import Qwen3TTSModel, VoiceClonePromptItem


# -------------------------------------------------
# A single TTS request
# -------------------------------------------------
#
# This object stores:
# 1. the input text
# 2. the generated PCM audio
# 3. the sample rate
# 4. possible errors
# 5. an Event used to notify the waiting HTTP request
#
class TTSRequest:
    def __init__(self, text):
        # Text that should be converted to speech.
        self.text = text

        # Generated PCM audio bytes.
        # None means generation has not finished yet.
        self.pcm = None

        # Sample rate of the generated audio.
        self.sample_rate = 0

        # Stores an exception if TTS generation fails.
        self.error = None

        # Event used for thread synchronization.
        #
        # Flask can wait on this Event.
        # The TTS worker calls set() when generation is finished.
        self.finished = Event()


# -------------------------------------------------
# Qwen TTS worker
# -------------------------------------------------
#
# This class owns:
# - the Qwen3-TTS model
# - the voice prompt
# - a request queue
# - one dedicated worker thread
#
# The Flask server submits text requests into the queue.
# The worker thread processes them one by one.
#
class QwenTTSWorker:
    def __init__(self, voice_file):
        # Path to the saved character voice prompt.
        self.voice_file = voice_file

        # Thread-safe queue containing TTSRequest objects.
        self.request_queue = Queue()

        # Controls the lifetime of the worker loop.
        self.running = False

        # The background thread used for TTS generation.
        self.worker_thread = None

        # These will be initialized when the model is loaded.
        self.tts_model = None
        self.voice_prompt = None


    # -------------------------------------------------
    # Start the TTS worker
    # -------------------------------------------------
    def start(self):
        # Avoid starting the worker twice.
        if self.running:
            return

        # Load the Qwen model and voice prompt first.
        self.loadModel()

        self.running = True

        # Create the background worker thread.
        #
        # target=self.workerLoop means:
        # run self.workerLoop() inside the new thread.
        self.worker_thread = Thread(
            target=self.workerLoop,daemon=True
        )

        # Start the worker thread.
        self.worker_thread.start()

        print("Qwen TTS worker started.")


    # -------------------------------------------------
    # Stop the TTS worker
    # -------------------------------------------------
    def stop(self):
        if not self.running:
            return

        # Tell the worker loop to stop.
        self.running = False

        # Queue.get() blocks while the queue is empty.
        #
        # Putting None into the queue wakes the worker up
        # so it can notice that it should stop.
        self.request_queue.put(None)

        # Wait for the worker thread to exit cleanly.
        if self.worker_thread is not None:
            self.worker_thread.join()

        print("Qwen TTS worker stopped.")


    # -------------------------------------------------
    # Submit a new TTS request
    # -------------------------------------------------
    def submit(self, text):
        # Create an object that stores both the request
        # and the future result.
        request = TTSRequest(text)

        # Put it into the thread-safe queue.
        self.request_queue.put(request)

        # Return the request object to the Flask thread.
        #
        # Flask can later wait for:
        # request.finished
        #
        # and read:
        # request.pcm
        # request.sample_rate
        # request.error
        return request


    # -------------------------------------------------
    # Load Qwen3-TTS and the saved character voice
    # -------------------------------------------------
    def loadModel(self):
        # The current project expects CUDA GPU inference.
        if not torch.cuda.is_available():
            raise RuntimeError("No CUDA available")

        # Make sure the voice prompt file exists.
        if not self.voice_file.is_file():
            raise FileNotFoundError(
                f"Voice file not found: {self.voice_file}"
            )

        # Load the previously saved voice-cloning prompt.
        #
        # The file contains:
        # - model_id
        # - prompt_items
        saved = torch.load(
            self.voice_file,
            map_location="cpu",
            weights_only=True
        )

        # Reconstruct Qwen VoiceClonePromptItem objects
        # from the saved dictionaries.
        self.voice_prompt = [
            VoiceClonePromptItem(**item)
            for item in saved["prompt_items"]
        ]

        print("Loading Qwen3-TTS...")

        # Load the same Qwen3-TTS model that was used
        # when the voice prompt was created.
        self.tts_model = Qwen3TTSModel.from_pretrained(
            saved["model_id"],
            device_map="cuda:0",
            dtype=torch.bfloat16,
            attn_implementation="sdpa"
        )

        print("Character voice loaded.")


    # -------------------------------------------------
    # Worker thread loop
    # -------------------------------------------------
    def workerLoop(self):
        while self.running:
            # Wait until a TTS request becomes available.
            #
            # Queue.get() blocks efficiently.
            # It does not waste CPU while waiting.
            request = self.request_queue.get()

            # None is used as a shutdown signal.
            if request is None:
                break

            try:
                # Generate speech for this request.
                self.generate(request)

            except Exception as error:
                # Save the error so the Flask thread
                # can return an HTTP error response.
                request.error = error

            finally:
                # Notify the waiting Flask request
                # that generation has finished.
                request.finished.set()


    # -------------------------------------------------
    # Generate speech with Qwen3-TTS
    # -------------------------------------------------
    def generate(self, request):
        # inference_mode disables gradient calculation.
        # This reduces memory usage and improves inference speed.
        with torch.inference_mode():
            # Wait for previous CUDA operations to finish.
            # This is useful for accurate timing.
            torch.cuda.synchronize()

            start = time.perf_counter()

            # Generate cloned speech from the request text.
            wavs, sample_rate = (
                self.tts_model.generate_voice_clone(
                    text=request.text,
                    language="English",
                    voice_clone_prompt=self.voice_prompt
                )
            )

            # Wait until GPU generation is fully finished.
            torch.cuda.synchronize()

            elapsed = (
                time.perf_counter()
                - start
            )

            # Calculate the duration of the generated audio.
            duration = (
                len(wavs[0])
                / sample_rate
            )

            # RTF = Real-Time Factor.
            #
            # Example:
            # RTF = 1.0
            # → generating 1 second of audio takes 1 second.
            #
            # RTF = 0.5
            # → generating 1 second of audio takes 0.5 seconds.
            print(
                f"Generation: {elapsed:.2f}s | "
                f"Audio: {duration:.2f}s | "
                f"RTF: {elapsed / duration:.2f}",
                flush=True
            )


        # -------------------------------------------------
        # Convert generated waveform to PCM int16
        # -------------------------------------------------

        # Convert the generated waveform into a NumPy float32 array.
        audio = np.asarray(
            wavs[0],
            dtype=np.float32
        ).reshape(-1)

        # Make sure every sample stays in the valid range.
        audio = np.clip(audio,-1.0,1.0)

        # Convert normalized float audio:
        #
        # -1.0 ... +1.0
        #
        # into signed 16-bit PCM:
        #
        # -32768 ... +32767
        pcm = ( audio * 32767).astype("<i2")

        # Convert PCM samples into raw bytes.
        #
        # These bytes will later be sent to the C++ client
        # through the HTTP response.
        request.pcm = pcm.tobytes()

        # Save audio metadata for the C++ client.
        request.sample_rate = sample_rate
