import os
import torch

from openvoice import se_extractor
from openvoice.api import ToneColorConverter
from melo.api import TTS


# ---------------------------------------------------------
# Configuration
# ---------------------------------------------------------

device = "cuda:0" if torch.cuda.is_available() else "cpu"

print("Device:", device)

ckpt_converter = "../OpenVoice/checkpoints_v2/converter"
reference_audio = "../OpenVoice/reference.wav"

output_dir = "outputs"
os.makedirs(output_dir, exist_ok=True)


# ---------------------------------------------------------
# Load OpenVoice tone color converter
# ---------------------------------------------------------

tone_color_converter = ToneColorConverter(
    f"{ckpt_converter}/config.json",
    device=device
)

tone_color_converter.load_ckpt(
    f"{ckpt_converter}/checkpoint.pth"
)


# ---------------------------------------------------------
# Extract your speaker embedding from reference.wav
# ---------------------------------------------------------

target_se, audio_name = se_extractor.get_se(
    reference_audio,
    tone_color_converter,
    vad=True
)

print("Speaker embedding extracted:", audio_name)


# ---------------------------------------------------------
# Create base TTS model
# ---------------------------------------------------------

language = "EN_NEWEST"

tts_model = TTS(
    language=language,
    device=device
)

speaker_ids = tts_model.hps.data.spk2id

print("Available speakers:", speaker_ids)


# Pick a base speaker.
speaker_key = list(speaker_ids.keys())[0]
speaker_id = speaker_ids[speaker_key]

print("Using base speaker:", speaker_key)


# ---------------------------------------------------------
# Generate base speech
# ---------------------------------------------------------

text = "Hello, this is a short recording of my natural speaking voice. I am using my normal tone, speed, and pronunciation. The purpose of this recording is to help the system learn the characteristics of my voice. I will continue speaking clearly and naturally, without trying to sound different from the way I normally speak."

tmp_path = f"{output_dir}/base.wav"

tts_model.tts_to_file(
    text,
    speaker_id,
    tmp_path,
    speed=1.0
)


# ---------------------------------------------------------
# Load source speaker embedding
# ---------------------------------------------------------

source_se_path = "../OpenVoice/checkpoints_v2/base_speakers/ses/en-us.pth"

source_se = torch.load(
    source_se_path,
    map_location=device
)


# ---------------------------------------------------------
# Convert base voice into your cloned voice
# ---------------------------------------------------------

output_path = f"{output_dir}/my_voice.wav"

tone_color_converter.convert(
    audio_src_path=tmp_path,
    src_se=source_se,
    tgt_se=target_se,
    output_path=output_path,
    message="@MyShell"
)

print("Done!")
print("Output:", output_path)