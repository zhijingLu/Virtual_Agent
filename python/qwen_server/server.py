from pathlib import Path

from flask import Flask, request, Response

from whisper_worker import WhisperWorker
from qwen_tts_worker import QwenTTSWorker


app = Flask(__name__)

folder = Path(__file__).resolve().parent


# -------------------------------------------------
# Workers
# -------------------------------------------------

tts_worker = QwenTTSWorker(
    folder / "character_voice.pt"
)

whisper_worker = WhisperWorker()


tts_worker.start()
whisper_worker.start()


# -------------------------------------------------
# TTS Endpoint
# -------------------------------------------------

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
# STT Endpoint
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