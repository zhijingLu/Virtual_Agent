from flask import Flask, request, Response
from melo.api import TTS
import torch
import numpy as np

app = Flask(__name__)

device = "cuda" if torch.cuda.is_available() else "cpu"

# Load MeloTTS only once when the server starts.
tts_model = TTS(
    language="EN_NEWEST",
    device=device
)

speaker_ids = tts_model.hps.data.spk2id

print("Available speakers:")
print(speaker_ids)

# Use the first available base speaker for now.
speaker_name = list(speaker_ids.keys())[0]
speaker_id = speaker_ids[speaker_name]

sample_rate = tts_model.hps.data.sampling_rate

print("Using speaker:", speaker_name)
print("Sample rate:", sample_rate)


@app.route("/tts", methods=["POST"])
def tts():
    data = request.get_json()

    if data is None or "text" not in data:
        return {"error": "Missing text"}, 400

    text = data["text"]

    if not text:
        return {"error": "Text is empty"}, 400

    # Generate audio directly in memory. NumPy array of float32 samples in range [-1.0, 1.0].
    audio = tts_model.tts_to_file(
        text,
        speaker_id,
        output_path=None,
        speed=1.0,
        quiet=True
    )

    # Convert MeloTTS float samples to int16 PCM.
    audio = np.asarray(audio, dtype=np.float32)

    audio = np.clip(audio, -1.0, 1.0)

    pcm = (audio * 32767).astype(np.int16)  #-2^15 ~ 2^15 - 1  -》-32768 ~ 32767

    # Return raw PCM bytes. 
    # Set the MIME type to indicate raw binary data
    response = Response(
        pcm.tobytes(),
        mimetype="application/octet-stream"   
    )

    # Send audio metadata in HTTP headers.
    response.headers["X-Sample-Rate"] = str(sample_rate)
    response.headers["X-Channels"] = "1"

    return response


if __name__ == "__main__":
    print("TTS server running on http://127.0.0.1:5000")
    app.run(
        host="127.0.0.1",
        port=5000
    )