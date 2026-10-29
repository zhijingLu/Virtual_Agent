from pathlib import Path

from flask import Flask, request, Response

from whisper_worker import WhisperWorker
from tts_worker import QwenTTSWorker
from llm_worker import LLMWorker

app = Flask(__name__)

folder = Path(__file__).resolve().parent


# -------------------------------------------------
# Workers
# -------------------------------------------------

tts_worker = QwenTTSWorker(
    folder / "character_voice.pt"
)
whisper_worker = WhisperWorker()
llm_worker = LLMWorker()


tts_worker.start()
whisper_worker.start()
llm_worker.start()

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

'''
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


# -------------------------------------------------
# Main
# -------------------------------------------------

if __name__ == "__main__":

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