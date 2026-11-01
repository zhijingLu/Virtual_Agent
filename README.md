# Virtual Agent

A C++ / OpenGL virtual character with skeletal animation, voice cloning and LLM-based voice conversation. Python captures microphone audio and runs Whisper, keyword matching, Qwen3-0.6B, and Qwen3-TTS locally. C++ fetches replies, plays audio, and synchronizes character animation.

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
| Dialogue LLM | `Qwen/Qwen3-0.6B`, BF16 on CUDA, thinking disabled |
| LLM runtime | Hugging Face Transformers 4.57.3 (qwen-tts 0.1.1 dependency) |
| Qwen package | `qwen-tts==0.1.1` |
| NumPy | 1.26.4 |
| Microphone input | Python `sounddevice`; 16000 Hz, mono, int16 PCM |
| Activation | Match the word `Lily` in Whisper transcription, case-insensitively |

These are the recorded project versions, rather than a latest-version list. Exact Visual Studio, Whisper, and remaining library versions were not supplied; export the working environment for their exact pins.

### C++ Dependencies

| Library | Purpose |
| --- | --- |
| GLFW 3.4 | Window, input, and OpenGL context |
| GLAD | Load OpenGL functions |
| GLM | Vectors, matrices, and quaternions |
| Assimp | Load models, bones, and animations |
| stb_image | Decode texture images |
| miniaudio | PCM playback on the C++ side |
| cpp-httplib | HTTP communication with Python |
| nlohmann/json | Parse server status and build playback notifications |

### 1.1 Prerequisites and Directory Layout

Install Visual Studio 2022 with **Desktop development with C++**, the MSVC x64/x86 toolset, and a Windows SDK. Also install Git for Windows and Miniconda or Anaconda. Open **Anaconda PowerShell Prompt** for the commands below; if ordinary PowerShell cannot find Conda, use that prompt first.

The commands use these project locations:

| Location | Path / purpose |
| --- | --- |
| Repository root | `D:\project\virtual_agent` |
| Python server | `D:\project\virtual_agent\python\qwen_server` |
| vcpkg tool | `D:\tools\vcpkg` in the fresh-install example below |
| C++ dependencies | `D:\project\virtual_agent\vcpkg_installed\x64-windows-static` |
| Python environment | Named Conda environment `virtual-agent`; actual directory is reported by `conda env list` |

The vcpkg location is a setup choice, not a record of your existing installation. If vcpkg is already installed, run `Get-Command vcpkg` to find it and reuse that installation instead of cloning another copy. A named Conda environment is stored under Conda's configured environment directories, not automatically under the current project folder.

```powershell
Set-Location 'D:\project\virtual_agent'
git --version
conda --version
conda env list
```

### 1.2 Install vcpkg Once

For a fresh setup without an existing vcpkg installation:

```powershell
New-Item -ItemType Directory -Force 'D:\tools' | Out-Null
git clone https://github.com/microsoft/vcpkg.git D:\tools\vcpkg
Set-Location 'D:\tools\vcpkg'
.\bootstrap-vcpkg.bat
.\vcpkg.exe integrate install
.\vcpkg.exe version
```

`bootstrap-vcpkg.bat` prepares the tool. `integrate install` enables user-wide MSBuild integration for that copy of vcpkg. The following commands use its full executable path, so adding vcpkg to PATH is optional. Restart Visual Studio after enabling integration.

### 1.3 Configure Manifest Dependencies

Return to the repository root:

```powershell
Set-Location 'D:\project\virtual_agent'
notepad .\vcpkg.json
```

If `vcpkg.json` already exists, preserve its settings, dependencies, features, and baseline; add missing dependencies rather than overwriting it. For a new project, this is a minimal manifest for the vcpkg-managed libraries used here:

```json
{
    "name": "virtual-agent",
    "version-string": "0.1.0",
    "dependencies": [
        "glfw3",
        "glm",
        "assimp",
        "nlohmann-json"
    ]
}
```

Install from the directory containing the manifest:

```powershell
& 'D:\tools\vcpkg\vcpkg.exe' install --triplet x64-windows-static
```

If reusing an existing vcpkg executable already on PATH, the equivalent command is:

```powershell
vcpkg install --triplet x64-windows-static
```

Run one of these alternatives. Manifest mode reads package names from `vcpkg.json`; do not run `vcpkg install nlohmann-json:x64-windows-static` here.

Verify the installed headers:

```powershell
Test-Path .\vcpkg_installed\x64-windows-static\include\GLFW\glfw3.h
Test-Path .\vcpkg_installed\x64-windows-static\include\glm\glm.hpp
Test-Path .\vcpkg_installed\x64-windows-static\include\assimp\Importer.hpp
Test-Path .\vcpkg_installed\x64-windows-static\include\nlohmann\json.hpp
```

All four checks should return `True`. Exact C++ library versions depend on the retained manifest baseline / vcpkg checkout; keep those files or the checkout commit to reproduce an existing build.

### 1.4 Header and Source Dependencies

Keep the project's existing GLAD, stb_image, miniaudio, and cpp-httplib files. The minimal manifest above does not install these separately maintained files.

For a fresh checkout, verify that it includes the following assets; otherwise obtain the missing files from the corresponding official project:

| Dependency | Required setup |
| --- | --- |
| GLAD | Existing code uses GLAD 1 API: `glad/glad.h` and `gladLoadGLLoader`. Generate OpenGL 3.3 Core C bindings with GLAD 1, add `glad.c` to the VS project, and add its `include` directory. GLAD 2 bindings are not a drop-in replacement. |
| stb_image | Supply `stb_image.h`; define `STB_IMAGE_IMPLEMENTATION` in exactly one `.cpp` |
| miniaudio | Supply `miniaudio.h`; define `MINIAUDIO_IMPLEMENTATION` in exactly one `.cpp` |
| cpp-httplib | Supply `external/httplib/httplib.h`; add `external/httplib` to compiler include directories |

Official sources: [GLAD 1 generator](https://glad.dav1d.de/), [stb](https://github.com/nothings/stb), [miniaudio](https://github.com/mackron/miniaudio), and [cpp-httplib](https://github.com/yhirose/cpp-httplib). Preserve the versions that already build in this repository; their exact revisions were not supplied.

### 1.5 Configure Visual Studio / MSBuild

Open the existing solution, then right-click the C++ project and open **Properties**. Apply shared settings to **All Configurations / x64**.

| Property | Value |
| --- | --- |
| General → C++ Language Standard, or C/C++ → Language | ISO C++20 (`/std:c++20`) |
| vcpkg → Use Vcpkg | Yes |
| vcpkg → Use Vcpkg Manifest | Yes |
| vcpkg → Vcpkg Triplet | `x64-windows-static` |
| C/C++ → Code Generation → Runtime Library, Debug | Multi-threaded Debug (`/MTd`) |
| C/C++ → Code Generation → Runtime Library, Release | Multi-threaded (`/MT`) |
| Debugging → Working Directory | `D:\project\virtual_agent` |
| Linker → Input → Additional Dependencies | Retain existing entries; include `opengl32.lib` |

The working directory must contain `assets\Lu.glb` and `config\vertex.glsl` / `fragment.glsl`, matching the relative paths in `main.cpp`.

If the vcpkg property UI does not expose the settings, unload the project and edit its `.vcxproj`. Add this property group before the `Microsoft.Cpp.props` import, without removing existing configuration groups:

```xml
<PropertyGroup Label="Vcpkg">
  <VcpkgEnabled>true</VcpkgEnabled>
  <VcpkgEnableManifest>true</VcpkgEnableManifest>
  <VcpkgTriplet>x64-windows-static</VcpkgTriplet>
</PropertyGroup>
```

Reload the project. With integration enabled, manifest headers and libraries should be included automatically. If `<nlohmann/json.hpp>` remains unresolved after the file existence check succeeds, verify that VS uses the same vcpkg installation and triplet. As a fallback, add this actual path to **C/C++ → General → Additional Include Directories**, preserving inherited entries:

```text
D:\project\virtual_agent\vcpkg_installed\x64-windows-static\include
```

Also retain the existing GLAD, miniaudio, stb_image, and httplib include directories. Adding an include path alone does not configure library linking; use MSBuild integration for the vcpkg libraries.

Add `ConversationClient.cpp` to the project through **Add → Existing Item** if it is not already compiled. C++ does not start microphone capture or load sherpa-onnx in the current flow.

### 1.6 Create or Reuse the Conda Environment

Use **Anaconda PowerShell Prompt**. If `virtual-agent` already exists, skip environment creation and activate it directly; do not recreate a working environment just to add sounddevice.

Fresh environment only:

```powershell
Set-Location 'D:\project\virtual_agent\python\qwen_server'
conda create -n virtual-agent python=3.10.22 pip ffmpeg -c conda-forge -y
conda activate virtual-agent
```

Existing environment:

```powershell
Set-Location 'D:\project\virtual_agent\python\qwen_server'
conda activate virtual-agent
```

Confirm which Python will receive the packages:

```powershell
conda env list
(Get-Command python).Source
python -c "import sys; print(sys.executable); print(sys.version)"
```

The active environment should be `virtual-agent`. Its physical path depends on your Conda installation; do not assume it is `D:\project\virtual_agent`. Use `python -m pip` to install into that active interpreter.

Install the complete Python stack below after activating `virtual-agent`. Run these commands in order. Conda installs the interpreter, FFmpeg and microphone backend; pip installs the AI and HTTP packages into that same environment.

```powershell
Set-Location 'D:\project\virtual_agent\python\qwen_server'
conda activate virtual-agent
conda install -c conda-forge ffmpeg python-sounddevice -y
python -m pip install --upgrade pip setuptools wheel
python -m pip install torch==2.11.0 torchaudio==2.11.0 --index-url https://download.pytorch.org/whl/cu130
python -m pip install numpy==1.26.4
python -m pip install qwen-tts==0.1.1 transformers==4.57.3 accelerate==1.12.0
python -m pip install flask openai-whisper scipy pandas scikit-learn librosa soundfile soxr sentencepiece
python -m pip check
```

The PyTorch/CUDA versions above retain this project's recorded configuration. This document has not re-tested a fresh Windows installation. If the index reports no matching wheel, check the available Python/platform wheels before changing the project's version pair.

| Package / command | Purpose in this project |
| --- | --- |
| `python=3.10.22`, `pip` | Interpreter and package installer, created by the earlier Conda command |
| Conda `ffmpeg` | Audio-file decoding for Whisper and reference-audio workflows |
| Conda `python-sounddevice` | Provides the `sounddevice` import and microphone capture backend |
| `torch`, `torchaudio` | GPU tensor inference and audio operations |
| `numpy==1.26.4` | PCM arrays and numeric processing |
| `qwen-tts==0.1.1` | Qwen3-TTS model and voice-clone prompt support |
| `transformers==4.57.3` | Load the Qwen dialogue model; version required by qwen-tts 0.1.1 |
| `accelerate==1.12.0` | Model loading/runtime dependency required by qwen-tts 0.1.1 |
| `flask` | `/status`, `/reply/<reply_id>`, and `/playback` HTTP endpoints |
| `openai-whisper` | Provides `import whisper` for speech transcription |
| `soundfile`, `librosa`, `soxr`, `scipy` | Audio reading, preprocessing and resampling |
| `sentencepiece` | Tokenizer support |
| `pandas`, `scikit-learn` | Retained scientific-stack dependencies from the working setup |

Installing qwen-tts also resolves its dependencies, including `einops`, `onnxruntime`, `gradio`, and the Python `sox` wrapper. The wrapper and the separate SoX executable are different; the current PCM conversation path does not explicitly invoke the SoX command. Do not install FlashAttention for this setup; the workers use PyTorch SDPA.

If all previous AI packages already work and only today's microphone dependency is missing, run only:

```powershell
conda activate virtual-agent
conda install -c conda-forge python-sounddevice -y
```

A pip alternative is `python -m pip install sounddevice`; choose one microphone installation method. `Lock`, `uuid4`, `queue`, `threading`, `logging`, `re`, and `pathlib` belong to Python's standard library and need no pip installation. Import names and distribution names can differ: install `openai-whisper` for `import whisper`, and `python-sounddevice` through Conda for `import sounddevice`.

Do not install sherpa-onnx or its keyword model for the current Python microphone + Whisper keyword flow. The C++ nlohmann/json dependency is installed with vcpkg, independently of Conda.

### 1.7 Verify Python, CUDA, and Microphone Input

Run from the active environment:

```powershell
python -c "import torch; print(torch.__version__); print(torch.version.cuda); print(torch.cuda.is_available())"
python -c "import whisper, sounddevice, flask, transformers, qwen_tts, torchaudio; print('Imports OK'); print(transformers.__version__)"
python -m sounddevice
ffmpeg -version
```

Whisper currently requires CUDA, so `torch.cuda.is_available()` must print `True`. The PyTorch wheels provide their CUDA runtime; installing a separate CUDA toolkit is not normally required for these prebuilt wheels. A compatible NVIDIA driver is required.

If the wrong microphone is selected, use the input device index shown by `python -m sounddevice`:

```python
microphone_worker = MicrophoneWorker(
    whisper_worker=whisper_worker,
    on_text=handle_microphone_text,
    device=1,
    speech_threshold=0.015,
)
```

`1` is an example; replace it with your input device index. Keep `device=None` to use the default. Enable Windows microphone access for desktop apps if recording is blocked.

FFmpeg supports audio-file decoding. `soxr` is a resampling library and is different from the SoX executable. Current attention uses PyTorch SDPA without a separate FlashAttention installation.

After successful testing, save the environment from the server directory:

```powershell
Set-Location 'D:\project\virtual_agent\python\qwen_server'
python -m pip freeze > requirements-virtual-agent.txt
conda env export > environment-virtual-agent.yml
```

The Conda export records the environment location as well as packages; on another machine, review or remove a machine-specific `prefix:` line before recreating it. To restore a pip snapshot into an appropriately prepared environment, install the matching CUDA PyTorch pair first, then run:

```powershell
python -m pip install -r .\requirements-virtual-agent.txt
```


### 1.8 Models and Local Configuration

| Item | Configuration |
| --- | --- |
| Qwen3-TTS | `Qwen/Qwen3-TTS-12Hz-0.6B-Base`; downloaded on first load and cached locally |
| Character voice | Generate `character_voice.pt` using `test_qwen.py` |
| Whisper | `base`, configured in `whisper_worker.py`, running on CUDA |
| Dialogue model | `Qwen/Qwen3-0.6B`; downloaded on first load and cached locally |
| Character instructions | Configure identity, language, and reply length through `system_prompt` in `llm_worker.py` |
| Microphone | Default input on the Python server computer; configure `device` if needed |
| Keyword | `WAKE_WORD_PATTERN` in `server.py`; current example: `Lily` |
| Speech segmentation | RMS threshold `0.015`, 800 ms silence, 300 ms pre-roll, 200 ms minimum above-threshold audio, 15 s maximum recording |
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

**Flow:** server microphone → utterance segmentation → Whisper → keyword check → LLM → TTS → HTTP reply → C++ playback → Talk / Idle.

| Module / file | What it does |
| --- | --- |
| `microphone_worker.py` | Capture microphone audio using sounddevice; collect utterances and submit them to Whisper |
| `whisper_worker.py` | Load Whisper once and transcribe queued int16 recordings on CUDA |
| `server.py` | Match the activation keyword, coordinate LLM / TTS, retain reply audio, and expose status and playback endpoints |
| `llm_worker.py` | Process queued text with Qwen3-0.6B and return a short answer |
| `tts_worker.py` / `qwen_tts_worker.py` | Generate reply audio with Qwen3-TTS; use the filename imported by server.py |
| `test_qwen.py` | Create `character_voice.pt` and a preview WAV |
| `ConversationClient.h/.cpp` | Poll the server on a background thread, download replies, and queue playback notifications |
| `AudioBuffer.h` | Store int16 PCM samples, sample rate, and channel count |
| `CharacterController.cpp` | Store received audio, start playback in `update()`, and synchronize Talk / Idle |
| `AudioPlayer.cpp` | Play PCM with miniaudio and retain sample memory during playback |
| `main.cpp` | Start the network client, retrieve replies, update the character, and report playback state |

### Server Microphone and Speech Recognition

`MicrophoneWorker` owns the input stream, bounded audio queue, and processing thread. The sounddevice callback copies 20 ms blocks of 16000 Hz mono int16 PCM into the queue; it does not run inference.

The processing thread uses an RMS threshold to detect sound, retains pre-roll audio, and ends a recording after sustained silence or the maximum duration. This is basic energy-based segmentation, not a dedicated neural VAD. Adjust `speech_threshold` for the microphone and room; a lower value also admits more background noise.

A complete recording is submitted through `whisper_worker.submit(pcm, sample_rate, channels)`. Whisper converts int16 PCM to normalized float32 audio; stereo inputs are mixed to mono. The existing worker requires 16000 Hz audio. The model stays loaded between requests.

`MicrophoneWorker` uses `running` and a queue sentinel (`None`) to stop its thread. Each Whisper request has a separate `finished` Event indicating recognition completion. Stop the microphone worker before stopping Whisper.

### Keyword Activation and Reply Generation

`handle_microphone_text()` checks the transcription using `r"\blily\b"` with `re.IGNORECASE`. Text after the keyword is passed to the LLM. For example, “Hello Lily, tell me a joke” submits “tell me a joke”. A keyword without a following question does not submit a conversation; waiting for the next utterance is not implemented.

The microphone callback handler runs on the microphone processing thread and waits for the existing LLM and TTS workers in order. No new thread is created per keyword. The LLM uses `Qwen/Qwen3-0.6B`, BF16 on CUDA, SDPA attention, and `enable_thinking=False`. The current request contains system instructions and one user message, without conversation history.

### Text to Speech — Qwen3-TTS

**Voice preparation:** `reference.wav` + matching transcript → `test_qwen.py` → `character_voice.pt` + preview WAV.

Use a clean reference recording of roughly 5–10 seconds. Generate the prompt once for the selected voice and keep it accessible to `server.py`. The `.pt` file stores voice-conditioning data, not the full model.

At runtime, Python submits the LLM reply directly to the TTS worker. The generated reply is retained as raw signed 16-bit little-endian PCM with its sample rate and channel count. C++ no longer requests synthesis through `TTSEngine` or `CharacterController::speak(text)` in this flow.

### Server State and HTTP Endpoints

| State | Meaning |
| --- | --- |
| `idle` | Accept a new keyword-triggered question |
| `processing` | Generate the LLM reply and TTS audio |
| `ready` | Reply is available for C++ to download |
| `playing` | C++ has reported playback started |

`conversation_state` stores the status, pending reply, and latest error. A `threading.Lock` protects related state reads and writes across microphone processing and Flask request threads. Inference runs outside the lock. `uuid4().hex` provides an ID for each reply.

| Endpoint | Purpose |
| --- | --- |
| `GET /status` | Return state, reply ID, user text, reply text, and error |
| `GET /reply/<reply_id>` | Return raw PCM with `X-Reply-Id`, `X-Sample-Rate`, `X-Channels`, and `X-Audio-Format: pcm_s16le` |
| `POST /playback` | Accept `reply_id` and `state`: `started`, `finished`, or `failed` |

Playback completion or failure clears the pending reply and restores `idle`. Downloading audio alone does not complete a conversation. The current flow replaces the earlier audio-upload `/chat` pipeline; `/stt` and `/tts` are not required by this client.

Required server imports include `from threading import Lock` and `from uuid import uuid4`; both are standard-library imports. Normal HTTP access logs can be hidden with `logging.getLogger("werkzeug").setLevel(logging.WARNING)`, while warnings, errors, and explicit prints remain visible.

### C++ Reply Playback

`ConversationClient` uses cpp-httplib for HTTP and nlohmann/json for status and playback messages. Binary PCM does not pass through JSON. Its network thread polls about every 200 ms, decodes little-endian int16 audio into `ConversationReply::audio`, and delivers each reply ID once. Playback notifications are queued for background delivery and retried on transient failures.

| C++ step | Work completed |
| --- | --- |
| Startup | Create one `ConversationClient` in `main()` and call `start()` |
| `tryGetReply(ConversationReply&)` | Move a downloaded reply into the main thread without blocking rendering |
| `CharacterController::speakAudio()` | Move `reply.audio` into `pendingAudio` |
| `CharacterController::update()` | Start pending playback, enter Talk, and return to Idle when playback stops |
| `CharacterController::isSpeaking()` | Return `speechActive` |
| Main loop | Check playback after `update()`; send `started`, `finished`, or `failed` with the matching reply ID |
| Shutdown | Stop the network client before destroying the window |

Call character `update()` once per frame. `speakAudio()` queues audio rather than immediately playing it, so checking `isSpeaking()` before `update()` is too early. Keep HTTP requests off the rendering thread.

Remove the local TTS path from CharacterController: `speak(text)`, the `TTSEngine` member, TTS thread, `ttsRunning`, and the thread-joining destructor logic. Keep `speakAudio()`, `pendingAudio`, and playback / animation updates. C++ `AudioRecorder`, `WakeWordDetector`, `SpeechRecognizer`, `STTEngine`, `TTSEngine`, and `ChatEngine` are no longer started or used in the current conversation path; their source files may remain as earlier implementations.

### Current Limitations

- Whisper keyword matching can still miss or mis-transcribe a name. It is checked after utterance transcription, not immediately when the keyword is spoken.
- The microphone processing thread waits during recognition and reply generation; queued audio from that period is discarded afterward.
- During reply availability and playback, the current microphone worker can still capture and transcribe audio; the server ignores recognized text while busy. Actual input suspension and echo cancellation are not implemented. Headphones help during testing.
- A server-side microphone captures the server computer's input. A remote deployment would require client-side audio capture and transport.
- Playback feedback assumes `AudioPlayer::isPlaying()` reflects playback promptly. The current client stop routine does not guarantee queued notifications are delivered; a disconnected client can leave the server waiting in `ready` or `playing` until reset or restart.

### Current Latency

The full reply is synthesized before it becomes available for download; streaming playback is not implemented. One measured TTS run took 11.55 seconds for 5.36 seconds of audio (RTF 2.15), excluding STT and LLM time. Shorter replies reduce the amount of audio to generate. Worker threads keep the application responsive but do not automatically accelerate model inference; the models and renderer share GPU resources.

## 6. Run the Project

1. Install manifest dependencies, including `nlohmann-json`, and configure asset paths.
2. Install sounddevice in the `virtual-agent` environment and verify the selected input with `python -m sounddevice`.
3. Generate `character_voice.pt` if it does not exist.
4. Start Python from the server directory:

   ```powershell
   Set-Location 'D:\project\virtual_agent\python\qwen_server'
   conda activate virtual-agent
   python server.py
   ```

5. Wait for the models and microphone to start, then launch the C++ application.
6. Say the keyword and question in one utterance, for example: “Lily, tell me a short joke.” Pause for about 0.8 seconds to finish the recording.
7. Check the server transcription and reply. Confirm C++ downloads the audio, plays it with Talk animation, returns to Idle, and reports playback finished.

If normal speech does not trigger recording, lower `speech_threshold`, for example from `0.015` to `0.008`. If JSON headers are not found, verify the manifest installation directory and Visual Studio include configuration.

## 7. Current Status

Implemented: custom GLB character, rendering, skeletal animation, animation sequences and blending, Qwen3-TTS voice cloning, PCM playback, Python microphone capture and basic utterance segmentation, Whisper transcription with keyword activation, server-side LLM / TTS processing, and C++ reply retrieval with playback feedback and Talk / Idle synchronization.

The conversation path now keeps audio input and AI processing in Python, with character rendering, playback, and animation in C++.

Next: suspend microphone processing during character playback, improve utterance detection and activation reliability, handle disconnected clients, reduce speech latency, add conversation history, and add facial animation / lip synchronization.

## 8. Setup References

- [Microsoft: vcpkg with Visual Studio / MSBuild](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-msbuild)
- [Microsoft: manifest mode](https://learn.microsoft.com/en-us/vcpkg/consume/manifest-mode)
- [Microsoft: MSBuild integration settings](https://learn.microsoft.com/en-us/vcpkg/users/buildsystems/msbuild-integration)

- [Qwen3-TTS package dependencies](https://github.com/QwenLM/Qwen3-TTS/blob/main/pyproject.toml)
