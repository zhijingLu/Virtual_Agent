from queue import Queue
from threading import Thread, Event

import numpy as np
import torch
import whisper


# -------------------------------------------------
# A single STT request
# -------------------------------------------------
#
# This object stores:
# 1. the input PCM audio
# 2. audio metadata
# 3. recognized text
# 4. possible errors
# 5. an Event used to notify the waiting Flask request
#
class STTRequest:
    def __init__(
        self,
        pcm,
        sample_rate,
        channels
    ):
        # Raw PCM bytes received from the C++ client.
        self.pcm = pcm

        # Sample rate of the recorded audio.
        self.sample_rate = sample_rate

        # Number of audio channels.
        self.channels = channels

        # Whisper recognition result.
        self.text = ""

        # Stores an exception if recognition fails.
        self.error = None

        # Used to notify the Flask thread when
        # Whisper recognition has finished.
        self.finished = Event()


# -------------------------------------------------
# Whisper worker
# -------------------------------------------------
#
# This class owns:
# - the Whisper model
# - the STT request queue
# - one dedicated worker thread
#
class WhisperWorker:
    def __init__(self):
        # Thread-safe queue containing STTRequest objects.
        self.request_queue = Queue()

        # Controls the worker thread lifetime.
        self.running = False

        # Background Whisper thread.
        self.worker_thread = None

        # Whisper model.
        self.model = None


    # -------------------------------------------------
    # Start Whisper worker
    # -------------------------------------------------
    def start(self):
        if self.running:
            return

        # Load Whisper before starting the worker loop.
        self.loadModel()

        self.running = True

        # workerLoop() will run inside a separate thread.
        self.worker_thread = Thread(
            target=self.workerLoop
        )

        self.worker_thread.start()

        print("Whisper worker started.")


    # -------------------------------------------------
    # Stop Whisper worker
    # -------------------------------------------------
    def stop(self):
        if not self.running:
            return

        self.running = False

        # Queue.get() may be waiting for a new request.
        #
        # None wakes the worker so it can exit.
        self.request_queue.put(None)

        if self.worker_thread is not None:
            self.worker_thread.join()

        print("Whisper worker stopped.")


    # -------------------------------------------------
    # Submit audio for speech recognition
    # -------------------------------------------------
    def submit(self,pcm,sample_rate,channels):
        request = STTRequest(
            pcm,
            sample_rate,
            channels
        )

        # Send the request to the Whisper worker thread.
        self.request_queue.put(request)

        # Flask receives this object and can wait on:
        #
        # request.finished
        #
        # Then read:
        #
        # request.text
        # request.error
        return request


    # -------------------------------------------------
    # Load Whisper model
    # -------------------------------------------------
    def loadModel(self):
        if not torch.cuda.is_available():
            raise RuntimeError(
                "No CUDA available for Whisper"
            )

        print("Loading Whisper...")

        # Start with a smaller model while developing.
        #
        # Possible models include:
        # tiny
        # base
        # small
        # medium
        # large
        self.model = whisper.load_model(
            "base",
            device="cuda"
        )

        print("Whisper model loaded.")


    # -------------------------------------------------
    # Worker thread loop
    # -------------------------------------------------
    def workerLoop(self):
        while self.running:
            # Wait until an STT request becomes available.
            #
            # Queue.get() blocks efficiently and does not
            # continuously consume CPU.
            request = self.request_queue.get()

            # None means the worker should stop.
            if request is None:
                break

            try:
                self.transcribe(request)

            except Exception as error:
                request.error = error

            finally:
                # Notify the Flask thread that recognition
                # has finished.
                request.finished.set()


    # -------------------------------------------------
    # Run Whisper speech recognition
    # -------------------------------------------------
    def transcribe(self, request):
        # -------------------------------------------------
        # Convert raw PCM bytes to NumPy int16 ，little-endian signed 16-bit integers
        # -------------------------------------------------

        audio = np.frombuffer(request.pcm, dtype="<i2" ) 


        # -------------------------------------------------
        # Handle stereo audio
        # -------------------------------------------------

        if request.channels == 2:
            # Convert:
            #
            # L R L R L R ...
            #
            # into:
            #
            # [[L, R],
            #  [L, R],
            #  ...]
            audio = audio.reshape(-1,2 )

            # Mix left and right channels into mono.
            audio = audio.mean(axis=1)


        # -------------------------------------------------
        # Convert int16 PCM to float32
        # -------------------------------------------------
        #
        # int16:
        #
        # -32768 ... +32767
        #
        # Whisper expects normalized floating-point audio:
        #
        # approximately -1.0 ... +1.0
        #
        audio = audio.astype(np.float32)

        audio = (audio / 32768.0)


        # -------------------------------------------------
        # Whisper expects 16 kHz audio
        # -------------------------------------------------

        if request.sample_rate != 16000:
            raise ValueError(
                "Whisper currently expects "
                f"16000 Hz audio, but received "
                f"{request.sample_rate} Hz."
            )


        # -------------------------------------------------
        # Run Whisper
        # -------------------------------------------------

        with torch.inference_mode():
            result = self.model.transcribe( audio, fp16=True)


        # Extract recognized text.
        text = result["text"].strip()

        request.text = text

        print(
            f"Recognized: {text}",
            flush=True
        )