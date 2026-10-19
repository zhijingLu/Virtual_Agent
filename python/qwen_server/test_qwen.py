from pathlib import Path
from dataclasses import asdict

import torch
import soundfile as sf
from qwen_tts import Qwen3TTSModel

folder = Path(__file__).resolve().parent

if not torch.cuda.is_available():
    raise RuntimeError("No CUDA available")

model_id = "Qwen/Qwen3-TTS-12Hz-0.6B-Base"

print("Loading model...")

model = Qwen3TTSModel.from_pretrained(
    model_id,
    device_map="cuda:0",
    dtype=torch.bfloat16,
    attn_implementation="sdpa",
)

ref_text = (
    "Hello, this is my natural voice. I’m speaking clearly and calmly, just as I would in a normal conversation."
)

print("Extracting voice prompt...")

voice_prompt = model.create_voice_clone_prompt(
    ref_audio=str(folder / "reference.wav"),
    ref_text=ref_text,
    x_vector_only_mode=False,
)

# Move tensors to CPU before saving.
def to_cpu(value):
    if isinstance(value, torch.Tensor):
        return value.detach().cpu()
    if isinstance(value, dict):
        return {key: to_cpu(item) for key, item in value.items()}
    if isinstance(value, list):
        return [to_cpu(item) for item in value]
    if isinstance(value, tuple):
        return tuple(to_cpu(item) for item in value)
    return value


voice_file = folder / "character_voice.pt"

torch.save(
    {
        "model_id": model_id,
        "prompt_items": [
            to_cpu(asdict(item)) for item in voice_prompt
        ],
    },
    voice_file,
)

print(f"Voice saved: {voice_file}")

# Generate a preview using the extracted voice.
print("Generating preview...")

wavs, sample_rate = model.generate_voice_clone(
    text=(
        "Hello! I'm your virtual agent. "
        "It's great to finally speak with you in my own voice."
    ),
    language="English",
    voice_clone_prompt=voice_prompt,
)

output = folder / "output_clone.wav"
sf.write(str(output), wavs[0], sample_rate, subtype="PCM_16")

print(f"Preview saved: {output}")