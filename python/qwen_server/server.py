from pathlib import Path
from threading import Lock

from flask import Flask, request, Response
import torch
import numpy as np
from qwen_tts import Qwen3TTSModel, VoiceClonePromptItem
import time
app = Flask(__name__) #create a Flask application instance

folder = Path(__file__).resolve().parent
generation_lock = Lock()

if not torch.cuda.is_available():
    raise RuntimeError("No CUDA available")

# Load the saved character voice once.
voice_file = folder / "character_voice.pt"

if not voice_file.is_file():
    raise FileNotFoundError(f"Voice file not found: {voice_file}")

saved = torch.load(
    voice_file,
    map_location="cpu",
    weights_only=True,
)

voice_prompt = [
    VoiceClonePromptItem(**item)
    for item in saved["prompt_items"]
]

# Load the same model used to create the voice prompt.
print("Loading Qwen3-TTS...")

tts_model = Qwen3TTSModel.from_pretrained(
    saved["model_id"],
    device_map="cuda:0",
    dtype=torch.bfloat16,
    attn_implementation="sdpa",
)

print("Character voice loaded.")


@app.route("/tts", methods=["POST"]) #receives POST requests at the /tts endpoints
def tts():
    data = request.get_json(silent=True)

    if not isinstance(data, dict) or "text" not in data:
        return {"error": "Missing text"}, 400

    text = data["text"]

    if not isinstance(text, str):
        return {"error": "Text must be a string"}, 400

    text = text.strip()

    if not text:
        return {"error": "Text is empty"}, 400

    # Serialize requests so only one generation runs at a time.
    with generation_lock, torch.inference_mode():
        torch.cuda.synchronize()
        start = time.perf_counter()

        wavs, sample_rate = tts_model.generate_voice_clone(
            text=text,
            language="English",
            voice_clone_prompt=voice_prompt,
        )

        torch.cuda.synchronize()
        elapsed = time.perf_counter() - start
        duration = len(wavs[0]) / sample_rate

        print(
            f"Generation: {elapsed:.2f}s | "
            f"Audio: {duration:.2f}s | "
            f"RTF: {elapsed / duration:.2f}",
            flush=True,
        )
    # Convert generated audio to little-endian int16 PCM.
    audio = np.asarray(wavs[0], dtype=np.float32).reshape(-1)
    audio = np.clip(audio, -1.0, 1.0)
    pcm = (audio * 32767).astype("<i2")

    response = Response(
        pcm.tobytes(),
        mimetype="application/octet-stream",
    )

    # Keep the metadata expected by the C++ client.
    response.headers["X-Sample-Rate"] = str(sample_rate)
    response.headers["X-Channels"] = "1"

    return response


if __name__ == "__main__":
    print("TTS server running on http://127.0.0.1:5000")

    app.run(
        host="127.0.0.1",
        port=5000,
        debug=False,
        use_reloader=False,
    )