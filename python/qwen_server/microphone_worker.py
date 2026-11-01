from collections import deque
from queue import Queue, Empty, Full
from threading import Thread

import numpy as np
import sounddevice as sd


class MicrophoneWorker:
    def __init__(
        self,
        whisper_worker,
        on_text=None,
        device=None,
        speech_threshold=0.015,
    ):
        self.whisper_worker = whisper_worker

        # Optional callback receiving recognized text.
        self.on_text = on_text

        self.device = device
        self.sample_rate = 16000
        self.channels = 1

        self.frame_ms = 20
        self.frame_samples = (
            self.sample_rate * self.frame_ms // 1000
        )

        # Normalized RMS threshold; adjust for your microphone.
        self.speech_threshold = speech_threshold

        self.silence_ms = 800
        self.min_speech_ms = 200
        self.max_recording_ms = 15000
        self.pre_roll_ms = 300

        self.audio_queue = Queue(maxsize=100)

        self.running = False
        self.worker_thread = None
        self.stream = None

    # -------------------------------------------------
    # Start microphone and processing thread
    # -------------------------------------------------
    def start(self):
        if self.running:
            return

        if not self.whisper_worker.running:
            raise RuntimeError(
                "Start WhisperWorker before MicrophoneWorker."
            )

        self.clearQueue()

        stream = sd.RawInputStream(
            device=self.device,
            samplerate=self.sample_rate,
            channels=self.channels,
            dtype="int16",
            blocksize=self.frame_samples,
            callback=self.audioCallback,
        )

        self.running = True
        self.stream = stream

        try:
            stream.start()

            self.worker_thread = Thread(
                target=self.workerLoop,
                name="microphone-worker",
            )
            self.worker_thread.start()

        except Exception:
            self.running = False
            stream.close()
            self.stream = None
            raise

        print("Microphone worker started.")

    # -------------------------------------------------
    # Stop microphone and processing thread
    # -------------------------------------------------
    def stop(self):
        self.running = False

        # Stop callbacks before clearing the queue.
        try:
            if self.stream is not None:
                try:
                    self.stream.stop()
                finally:
                    self.stream.close()
                    self.stream = None

        finally:
            self.clearQueue()

            # Wake Queue.get() if it is waiting.
            self.audio_queue.put_nowait(None)

            if self.worker_thread is not None:
                self.worker_thread.join()
                self.worker_thread = None

        print("Microphone worker stopped.")

    # -------------------------------------------------
    # Audio callback
    # -------------------------------------------------
    def audioCallback(
        self,
        indata,
        frames,
        time_info,
        status,
    ):
        if not self.running:
            return

        # Never run Whisper inside the audio callback.
        try:
            self.audio_queue.put_nowait(bytes(indata))

        except Full:
            # Drop new audio rather than grow memory indefinitely.
            pass

    # -------------------------------------------------
    # Discard queued audio
    # -------------------------------------------------
    def clearQueue(self):
        while True:
            try:
                self.audio_queue.get_nowait()

            except Empty:
                break

    # -------------------------------------------------
    # Calculate normalized audio volume
    # -------------------------------------------------
    def calculateRMS(self, pcm):
        audio = np.frombuffer(
            pcm,
            dtype="<i2",
        ).astype(np.float32)

        audio /= 32768.0

        return float(
            np.sqrt(np.mean(audio * audio))
        )

    # -------------------------------------------------
    # Collect one utterance, then send it to Whisper
    # -------------------------------------------------
    def workerLoop(self):
        pre_roll = deque(
            maxlen=self.pre_roll_ms // self.frame_ms
        )

        recorded_chunks = []
        recording = False

        silence_ms = 0
        speech_ms = 0
        recording_ms = 0

        while self.running:
            pcm = self.audio_queue.get()

            if pcm is None or not self.running:
                break

            is_speech = (
                self.calculateRMS(pcm)
                >= self.speech_threshold
            )

            if not recording:
                pre_roll.append(pcm)

                if not is_speech:
                    continue

                # Include audio immediately before speech onset.
                recorded_chunks = list(pre_roll)
                pre_roll.clear()

                recording = True
                silence_ms = 0
                speech_ms = self.frame_ms
                recording_ms = (
                    len(recorded_chunks) * self.frame_ms
                )

                print("Speech started.")
                continue

            recorded_chunks.append(pcm)
            recording_ms += self.frame_ms

            if is_speech:
                speech_ms += self.frame_ms
                silence_ms = 0
            else:
                silence_ms += self.frame_ms

            utterance_finished = (
                silence_ms >= self.silence_ms
                or recording_ms >= self.max_recording_ms
            )

            if not utterance_finished:
                continue

            audio_pcm = b"".join(recorded_chunks)
            valid_speech = speech_ms >= self.min_speech_ms

            recorded_chunks = []
            recording = False

            silence_ms = 0
            speech_ms = 0
            recording_ms = 0
            pre_roll.clear()

            if not valid_speech:
                continue

            print("Speech ended. Recognizing...")

            try:
                self.recognize(audio_pcm)

            except Exception as error:
                print("Microphone processing failed:", error)

            finally:
                # This version ignores speech captured during inference.
                # Prevent that old audio from being processed later.
                self.clearQueue()

    # -------------------------------------------------
    # Submit a complete utterance to Whisper
    # -------------------------------------------------
    def recognize(self, pcm):
        request = self.whisper_worker.submit(
            pcm,
            self.sample_rate,
            self.channels,
        )

        # Wait in short intervals so stop() remains responsive.
        while not request.finished.wait(timeout=0.1):
            if not self.running:
                return

        if not self.running:
            return

        if request.error is not None:
            print("Whisper failed:", request.error)
            return

        text = request.text.strip()

        if not text:
            return

        if self.on_text is not None:
            self.on_text(text)