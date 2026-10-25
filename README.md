# Virtual Agent

A C++ / OpenGL virtual character with skeletal animation, voice cloning, wake-word detection, and speech recognition. Python provides a local Qwen3-TTS and Whisper service.

## 1. Requirements and Setup

### Development Environment

| Component | Project configuration |
| --- | --- |
| OS | Windows 11, x64 |
| IDE | Visual Studio with **Desktop development with C++** |
| Language | C++20 |
| Graphics | OpenGL 3.3 Core |
| C++ package manager | vcpkg manifest mode, `x64-windows-static` |
| Python | Python 3.10; development environment used 3.10.22 |
| GPU | NVIDIA RTX 5060 Laptop GPU |
| PyTorch / torchaudio | 2.11.0, CUDA 13.0 wheels (`cu130`) |
| Qwen package | `qwen-tts==0.1.1` |
| NumPy | 1.26.4 |
| Wake-word runtime | sherpa-onnx 1.13.8, Windows x64 shared MT Release, no-TTS package  |
| Wake-word model | `kws-zipformer-gigaspeech-3.3M-2024-01-01` |
```
curl.exe -fL -o sherpa-onnx.tar.bz2 "https://github.com/k2-fsa/sherpa-onnx/releases/download/v1.13.8/sherpa-onnx-v1.13.8-win-x64-shared-MT-Release-no-tts.tar.bz2"

tar -xf sherpa-onnx.tar.bz2

curl.exe -fL -o kws-model.tar.bz2 "https://github.com/k2-fsa/sherpa-onnx/releases/download/kws-models/sherpa-onnx-kws-zipformer-gigaspeech-3.3M-2024-01-01.tar.bz2"

tar -xf kws-model.tar.bz2
```

These are the recorded project versions, rather than a latest-version list. Exact Visual Studio, Whisper, and remaining library versions were not supplied; retain the working environment's versions.

### C++ Dependencies

| Library | Purpose |
| --- | --- |
| GLFW 3.4 | Window, input, and OpenGL context |
| GLAD | Load OpenGL functions |
| GLM | Vectors, matrices, and quaternions |
| Assimp | Load models, bones, and animations |
| stb_image | Decode texture images |
| miniaudio | Microphone recording and PCM playback |
| cpp-httplib | HTTP communication with Python |
| sherpa-onnx | Local wake-word detection |

From the directory containing `vcpkg.json`, install the manifest dependencies:

```powershell
vcpkg install --triplet x64-windows-static
```

In Visual Studio:

1. Select **x64** and configure vcpkg to use `x64-windows-static`.
2. Add the include directories for the header-only libraries. The existing HTTP layout uses `external/httplib/httplib.h`, with `$(ProjectDir)external\httplib` as an include directory.
3. Add sherpa-onnx's `include` and `lib` directories, and link the import libraries required by its C API.
4. Copy the runtime DLLs from the sherpa-onnx package beside the executable.
5. Set the character GLB, shaders, and wake-word model paths to their actual locations.

Keep each header-only library's implementation macro in one translation unit only. Use a compatible MSVC runtime setting for the selected library builds.

### Python Environment

Run in a Conda-enabled terminal:

```bat
conda create -n virtual-agent python=3.10.22 pip ffmpeg -c conda-forge -y
conda activate virtual-agent
python -m pip install --upgrade pip setuptools wheel
python -m pip install torch==2.11.0 torchaudio==2.11.0 --index-url https://download.pytorch.org/whl/cu130
python -m pip install numpy==1.26.4
python -m pip install scipy pandas scikit-learn librosa soundfile soxr sentencepiece
python -m pip install qwen-tts==0.1.1 flask openai-whisper
python -m pip check
```

FFmpeg supports audio-file decoding. The C++ / Python runtime transfers raw PCM directly. `soxr` is a Python resampling library and is different from the SoX command-line program. This setup uses PyTorch SDPA attention without a separate FlashAttention installation.

Check GPU access:

```bat
python -c "import torch; print(torch.__version__); print(torch.version.cuda); print(torch.cuda.is_available())"
```

After successfully running both TTS and STT, record the complete package versions:

```bat
python -m pip freeze > requirements-virtual-agent.txt
```

To restore that snapshot, install the matching PyTorch pair from the CUDA wheel index above first, then run:

```bat
python -m pip install -r requirements-virtual-agent.txt
```

### Models and Local Configuration

| Item | Configuration |
| --- | --- |
| Qwen3-TTS | `Qwen/Qwen3-TTS-12Hz-0.6B-Base`; downloaded on first load and cached locally |
| Character voice | Generate `character_voice.pt` using `test_qwen.py` |
| Whisper | Use the model name configured in `server.py`; its exact name was not supplied |
| Wake-word model | Extract the sherpa-onnx model and configure encoder, decoder, joiner, and token-file paths |
| Wake words | Configure the keyword file using the model's token format; current example phrase: “hello, lily” |
| HTTP service | Match the C++ clients and Flask server; existing service address: `127.0.0.1:5000` |

Model downloads require network access initially. C++ communicates with the Python service locally.

## 2. Character Asset Creation

**Flow:** AI reference image → TRELLIS → Mixamo → Blender → GLB.

| Tool | Work completed |
| --- | --- |
| AI image generation | Create the character's appearance reference |
| TRELLIS | Convert the image into a textured 3D model |
| Mixamo | Add a humanoid rig and download animation clips |
| Blender | Combine the character, one main armature, and multiple animation actions; export a GLB |

Keep the character mesh attached to the main armature, check skeleton scale, retain the required actions, and export the intended objects. Current actions include Idle, Walk, Run, Talk, StandToSit, and Sitting.

## 3. Rendering

**Flow:** GLB → Assimp → mesh / textures / bones → OpenGL buffers → shaders → character on screen.

| Module / file | What it does |
| --- | --- |
| `Model.cpp` | Load meshes, materials, textures, bone IDs, weights, and offset matrices |
| `Mesh.cpp` | Upload vertex/index data through VAO, VBO, and EBO; bind textures and draw |
| `Camera.cpp` | Control camera movement and build the view matrix |
| Vertex shader | Apply weighted bone matrices and model/view/projection transforms |
| Fragment shader | Sample the character texture and calculate ambient, diffuse, and specular lighting |
| `main.cpp` | Initialize the window and OpenGL; update and draw each frame |

## 4. Animation

**Flow:** requested state → AnimationController → Animator → bone matrices → GPU skinning.

| Module / file | What it does |
| --- | --- |
| `Animation.cpp` | Load a selected animation clip, keyframes, duration, and node hierarchy |
| `Animator.cpp` | Advance playback, interpolate keyframes, calculate bone transforms, and blend animations |
| `AnimationController.cpp` | Select states, play sequences, track completion, and manage repeat counts |

Implemented features:

- Looping and one-time playback.
- State switching and ordered animation sequences.
- Per-state repeat counts.
- Position/scale interpolation and quaternion rotation interpolation.
- Animation blending, with a default transition of 0.2 seconds.
- GPU skinning with up to four bone influences per vertex.

## 5. Audio and Speech Modules

The file names below identify modules; keep their directory prefixes consistent with the actual project layout.

| Module / file | What it does |
| --- | --- |
| `AudioBuffer.h` | Store `int16_t` PCM samples, sample rate, and channel count |
| `AudioRecorder.cpp` | Capture microphone audio with miniaudio and deliver chunks through a callback |
| `WakeWordDetector.cpp` | Feed audio to sherpa-onnx and report a detected wake word |
| `SpeechRecognizer.cpp` | Maintain recent audio, collect a triggered recording, and pass completed audio to the STT worker |
| `STTEngine.cpp` | Send PCM to Python's `/stt` endpoint and return recognized text |
| `TTSEngine.cpp` | Send text to `/tts` and build an `AudioBuffer` from the PCM response |
| `AudioPlayer.cpp` | Play PCM with miniaudio and retain sample memory during playback |
| `server.py` | Load Qwen3-TTS, the saved voice prompt, and Whisper; serve TTS and STT requests |
| `test_qwen.py` | Create the reusable voice prompt and a preview WAV |

### Microphone and Wake-Word Detection

**Flow:** microphone → AudioRecorder callback → SpeechRecognizer → WakeWordDetector → start recording.

`AudioRecorder::start()` stores a callback. Whenever a new audio chunk arrives, the callback calls `speechRecognizer.processAudioChunk(chunk)`.

While waiting, `SpeechRecognizer` keeps recent audio in `rollingBuffer`. When `notifyWakeWordDetected()` runs, it copies that history into `recordedSamples`. Subsequent chunks are appended until the configured recording-end condition is met. The completed audio is queued for transcription.

The rolling buffer preserves audio around the trigger; subsequent chunks capture the rest of the spoken command. The trigger chunk should be included only once.

### Speech to Text — Whisper

**Flow:** recorded PCM → STTEngine → `POST /stt` → Whisper → recognized text.

The Python endpoint interprets signed 16-bit PCM and converts it into normalized floating-point samples. Whisper's input must be mono at 16 kHz; configure recording accordingly or resample before transcription. Keep the client and server consistent on sample rate, channels, and the response format.

Transcription runs through a worker so that waiting for the server does not block rendering.

### Text to Speech — Qwen3-TTS

**Voice preparation:** `reference.wav` + matching transcript → `test_qwen.py` → `character_voice.pt` + preview WAV.

Use a clean reference recording of roughly 5–10 seconds. Run `test_qwen.py` once for the chosen voice, listen to the preview, and keep the generated `.pt` file accessible to `server.py`. This file stores voice-conditioning data, not the complete model.

**Runtime flow:** text → TTSEngine → `POST /tts` → Qwen3-TTS → PCM → AudioPlayer.

The server loads the model and voice prompt once, then reuses them. The TTS response contains raw signed 16-bit little-endian PCM, with `X-Sample-Rate` and `X-Channels` headers. `TTSEngine` reads that metadata; `AudioPlayer` uses it for playback.

Speech synthesis is requested asynchronously. During playback, the character uses Talk; after playback ends, it returns to Idle.

## 6. Run the Project

1. Finish the C++ dependency setup and configure asset/model paths.
2. Generate `character_voice.pt` if it does not exist.
3. Activate the Python environment and start the server from its directory:

   ```bat
   conda activate virtual-agent
   python server.py
   ```

4. Keep the server running and launch the C++ application from Visual Studio.
5. Use the configured speech trigger to test TTS. Say the configured wake phrase followed by a command to test recording and Whisper transcription.

## 7. Current Status

Implemented: custom GLB character, rendering, skeletal animation, animation sequences and blending, Qwen3-TTS voice cloning, PCM playback, microphone recording, wake-word detection, and Whisper STT.

Next: connect recognized text to a dialogue/response layer, then add facial animation and lip synchronization.
