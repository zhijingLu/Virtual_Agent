from pathlib import Path

from flask import Flask, request, Response

from whisper_worker import WhisperWorker
from tts_worker import QwenTTSWorker
from llm_worker import LLMWorker
import re
from threading import Lock
from uuid import uuid4
from microphone_worker import MicrophoneWorker
import logging
app = Flask(__name__)

folder = Path(__file__).resolve().parent



logging.getLogger("werkzeug").setLevel(logging.WARNING)
# -------------------------------------------------
# Workers
# -------------------------------------------------

tts_worker = QwenTTSWorker(
    folder / "character_voice.pt"
)
whisper_worker = WhisperWorker()
llm_worker = LLMWorker()

"""
tts_worker.start()
whisper_worker.start()
llm_worker.start()
"""
# -------------------------------------------------
# Wake word
# -------------------------------------------------

WAKE_WORD_PATTERN = re.compile(
    r"\bLily\b",
    flags=re.IGNORECASE,
)


# -------------------------------------------------
# TTS 
# -------------------------------------------------
'''
@app.route("/tts", methods=["POST"])
def tts():

    data = request.get_json(
        silent=True
    )

    if (
        not isinstance(data, dict)
        or "text" not in data
    ):
        return {
            "error": "Missing text"
        }, 400


    text = data["text"]

    if not isinstance(text, str):
        return {
            "error": "Text must be a string"
        }, 400


    text = text.strip()

    if not text:
        return {
            "error": "Text is empty"
        }, 400


    # Submit TTS request.
    tts_request = tts_worker.submit(
        text
    )


    # Wait for worker.
    if not tts_request.finished.wait(
        timeout=60
    ):
        return {
            "error": "TTS request timed out"
        }, 504


    if tts_request.error is not None:

        print(
            "TTS generation failed:",
            tts_request.error
        )

        return {
            "error": "TTS generation failed"
        }, 500


    response = Response(
        tts_request.pcm,
        mimetype="application/octet-stream"
    )


    response.headers[
        "X-Sample-Rate"
    ] = str(
        tts_request.sample_rate
    )

    response.headers[
        "X-Channels"
    ] = "1"


    return response


# -------------------------------------------------
# STT 
# -------------------------------------------------

@app.route("/stt", methods=["POST"])
def stt():

    # Raw int16 PCM bytes from C++.
    pcm = request.get_data()

    if not pcm:
        return {
            "error": "Audio data is empty"
        }, 400


    try:

        sample_rate = int(
            request.headers[
                "X-Sample-Rate"
            ]
        )

        channels = int(
            request.headers[
                "X-Channels"
            ]
        )

    except (KeyError, ValueError):

        return {
            "error": "Invalid audio metadata"
        }, 400


    # Submit STT request.
    stt_request = whisper_worker.submit(
        pcm,
        sample_rate,
        channels
    )


    # Wait for Whisper worker.
    if not stt_request.finished.wait(
        timeout=60
    ):
        return {
            "error": "STT request timed out"
        }, 504


    if stt_request.error is not None:

        print(
            "Whisper failed:",
            stt_request.error
        )

        return {
            "error": "Speech recognition failed"
        }, 500


    return Response(
        stt_request.text,
        mimetype="text/plain"
    )

# -------------------------------------------------
# Chat llm
# -------------------------------------------------

@app.route("/chat", methods=["POST"])
def chat():
    data = request.get_json(silent=True)

    if not isinstance(data, dict):
        return {"error": "Expected a JSON object"}, 400

    text = data.get("text")

    if not isinstance(text, str) or not text.strip():
        return {"error": "Text must be a non-empty string"}, 400

    # Send text to the LLM worker.
    llm_request = llm_worker.submit(text)

    # Wait for the generated reply.
    if not llm_request.finished.wait(timeout=60):
        return {"error": "LLM request timed out"}, 504

    if llm_request.error is not None:
        print("LLM failed:", llm_request.error)
        return {"error": "Reply generation failed"}, 500

    return {"reply": llm_request.reply}


# -------------------------------------------------
# Conversation: Audio -> Whisper -> LLM -> TTS
# -------------------------------------------------
@app.route("/chat", methods=["POST"])
def conversation():
    # 1. Read recorded audio from C++.
    pcm = request.get_data()

    if not pcm:
        return {"error": "Audio data is empty"}, 400

    try:
        sample_rate = int(
            request.headers["X-Sample-Rate"]
        )
        channels = int(
            request.headers["X-Channels"]
        )
    except (KeyError, ValueError):
        return {"error": "Invalid audio metadata"}, 400

    # Your Whisper worker currently expects 16 kHz PCM.
    if sample_rate != 16000 or channels not in (1, 2):
        return {
            "error": "Expected 16000 Hz mono or stereo audio"
        }, 400

    # Each PCM frame contains one int16 sample per channel.
    if len(pcm) % (2 * channels) != 0:
        return {"error": "Invalid int16 PCM length"}, 400

    # 2. Whisper: audio -> user text.
    stt_request = whisper_worker.submit(
        pcm,
        sample_rate,
        channels
    )

    if not stt_request.finished.wait(timeout=60):
        return {"error": "STT request timed out"}, 504

    if stt_request.error is not None:
        print("Whisper failed:", stt_request.error)
        return {"error": "Speech recognition failed"}, 500

    user_text = stt_request.text.strip()

    if not user_text:
        return {"error": "No speech recognized"}, 422

    # 3. LLM: user text -> reply text.
    llm_request = llm_worker.submit(user_text)

    if not llm_request.finished.wait(timeout=60):
        return {"error": "LLM request timed out"}, 504

    if llm_request.error is not None:
        print("LLM failed:", llm_request.error)
        return {"error": "Reply generation failed"}, 500

    reply_text = llm_request.reply.strip()

    if not reply_text:
        return {"error": "LLM returned an empty reply"}, 500

    # 4. TTS: reply text -> audio.
    tts_request = tts_worker.submit(reply_text)

    if not tts_request.finished.wait(timeout=60):
        return {"error": "TTS request timed out"}, 504

    if tts_request.error is not None:
        print("TTS failed:", tts_request.error)
        return {"error": "Speech generation failed"}, 500

    # 5. Return raw int16 PCM audio to C++.
    response = Response(
        tts_request.pcm,
        mimetype="application/octet-stream"
    )

    response.headers["X-Sample-Rate"] = str(
        tts_request.sample_rate
    )
    response.headers["X-Channels"] = "1"

    return response
'''
# -------------------------------------------------
# Conversation state
# -------------------------------------------------

state_lock = Lock()

# idle -> processing -> ready -> playing -> idle
conversation_state = {
    "status": "idle",
    "reply": None,
    "error": None,
}


def reset_conversation(error=None):
    with state_lock:
        conversation_state["status"] = "idle"
        conversation_state["reply"] = None
        conversation_state["error"] = error


def wait_for_worker(worker_request, name):
    # Do not submit another conversation while this task is running.
    # Actual inference completion is signalled by the worker.
    worker_request.finished.wait()

    if worker_request.error is not None:
        raise RuntimeError(
            f"{name} failed: {worker_request.error}"
        )

# -------------------------------------------------
# Microphone text -> keyword -> LLM -> TTS
# -------------------------------------------------

def handle_microphone_text(text):
    print(f"Microphone text: {text}", flush=True)

    with state_lock:
        if conversation_state["status"] != "idle":
            return

    match = WAKE_WORD_PATTERN.search(text)

    if match is None:
        print("No wake word detected.", flush=True)
        return

    # "Hello Alice, how are you?" -> "how are you?"
    user_text = text[match.end():].strip(
        " \t\r\n,，.!！?？:：;；"
    )

    if not user_text:
        print(
            "Wake word detected. "
            "Please say the wake word and question together.",
            flush=True,
        )
        return

    with state_lock:
        if conversation_state["status"] != "idle":
            return

        conversation_state["status"] = "processing"
        conversation_state["reply"] = None
        conversation_state["error"] = None

    try:
        print(f"User: {user_text}", flush=True)

        # 1. LLM
        llm_request = llm_worker.submit(user_text)

        wait_for_worker(
            llm_request,
            "LLM",
        )

        reply_text = llm_request.reply.strip()

        if not reply_text:
            raise RuntimeError(
                "LLM returned an empty reply."
            )

        print(f"Assistant: {reply_text}", flush=True)

        # 2. TTS
        tts_request = tts_worker.submit(reply_text)

        wait_for_worker(
            tts_request,
            "TTS",
        )

        if not tts_request.pcm:
            raise RuntimeError(
                "TTS returned empty audio."
            )

        reply = {
            "id": uuid4().hex,
            "user_text": user_text,
            "reply_text": reply_text,
            "pcm": bytes(tts_request.pcm),
            "sample_rate": int(tts_request.sample_rate),
            "channels": 1,
        }

        # 3. Store the reply for C++.
        with state_lock:
            conversation_state["reply"] = reply
            conversation_state["status"] = "ready"

        print(
            "Reply ready. Waiting for C++.",
            flush=True,
        )

    except Exception as error:
        print(
            f"Conversation failed: {error}",
            flush=True,
        )

        reset_conversation(
            error=str(error)
        )


microphone_worker = MicrophoneWorker(
    whisper_worker=whisper_worker,
    on_text=handle_microphone_text,
    speech_threshold=0.015,
)
# -------------------------------------------------
# GET /status
# C++ checks whether a reply is ready.
# -------------------------------------------------

@app.route("/status", methods=["GET"])
def get_status():
    with state_lock:
        reply = conversation_state["reply"]

        return {
            "status": conversation_state["status"],
            "reply_id": (
                reply["id"] if reply is not None else None
            ),
            "user_text": (
                reply["user_text"] if reply is not None else None
            ),
            "reply_text": (
                reply["reply_text"] if reply is not None else None
            ),
            "error": conversation_state["error"],
        }


# -------------------------------------------------
# GET /reply/<reply_id>
# C++ downloads raw int16 PCM audio.
# Repeated downloads return the same pending reply.
# -------------------------------------------------

@app.route("/reply/<reply_id>", methods=["GET"])
def get_reply(reply_id):
    with state_lock:
        reply = conversation_state["reply"]

        if reply is None or reply["id"] != reply_id:
            return {
                "error": "Reply not found"
            }, 404

        if conversation_state["status"] not in (
            "ready",
            "playing",
        ):
            return {
                "error": "Reply is not available"
            }, 409

        pcm = reply["pcm"]
        sample_rate = reply["sample_rate"]
        channels = reply["channels"]

    response = Response(
        pcm,
        mimetype="application/octet-stream",
    )

    response.headers["X-Reply-Id"] = reply_id
    response.headers["X-Sample-Rate"] = str(sample_rate)
    response.headers["X-Channels"] = str(channels)
    response.headers["X-Audio-Format"] = "pcm_s16le"
    response.headers["Cache-Control"] = "no-store"

    return response


# -------------------------------------------------
# POST /playback
#
# C++ sends:
# {"reply_id": "...", "state": "started"}
# {"reply_id": "...", "state": "finished"}
# {"reply_id": "...", "state": "failed"}
# -------------------------------------------------

@app.route("/playback", methods=["POST"])
def update_playback():
    data = request.get_json(silent=True)

    if not isinstance(data, dict):
        return {
            "error": "Expected a JSON object"
        }, 400

    reply_id = data.get("reply_id")
    playback_state = data.get("state")

    if (
        not isinstance(reply_id, str)
        or playback_state not in (
            "started",
            "finished",
            "failed",
        )
    ):
        return {
            "error": "Invalid reply_id or playback state"
        }, 400

    with state_lock:
        reply = conversation_state["reply"]

        if reply is None or reply["id"] != reply_id:
            return {
                "error": "Reply not found"
            }, 404

        if playback_state == "started":
            conversation_state["status"] = "playing"

        else:
            conversation_state["reply"] = None
            conversation_state["status"] = "idle"
            conversation_state["error"] = (
                "C++ playback failed"
                if playback_state == "failed"
                else None
            )

        status = conversation_state["status"]

    print(
        f"Playback: {playback_state} | Status: {status}",
        flush=True,
    )

    return {
        "status": status
    }


# -------------------------------------------------
# Main
# -------------------------------------------------

if __name__ == "__main__":
    '''
    print(
        "Voice server running on "
        "http://127.0.0.1:5000"
    )

    app.run(
        host="127.0.0.1",
        port=5000,
        debug=False,
        use_reloader=False
    )
    '''

    try:
        # Load models before opening the microphone.
        whisper_worker.start()
        llm_worker.start()
        tts_worker.start()

        microphone_worker.start()

        print(
            "Voice server running on "
            "http://127.0.0.1:5000",
            flush=True,
        )

        print(
            'example Say: "Lily, how are you?"',
            flush=True,
        )

        app.run(
            host="127.0.0.1",
            port=5000,
            debug=False,
            use_reloader=False,
            threaded=True,
        )

    finally:
        # Stop the microphone before stopping model workers.
        # Assumes all three model workers provide stop().
        for name, worker in (
            ("Microphone", microphone_worker),
            ("Whisper", whisper_worker),
            ("LLM", llm_worker),
            ("TTS", tts_worker),
        ):
            try:
                worker.stop()
            except Exception as error:
                print(
                    f"Failed to stop {name}: {error}",
                    flush=True,
                )