# Virtual Agent

A C++ / OpenGL learning project for understanding how a 3D animated virtual character works.

The project combines a custom animated character, local voice cloning, microphone capture, wake-word detection, and speech recognition. The next stage is to connect recognized speech to a dialogue and response layer.

---

# 1. Project Setup

## Environment

The project is developed with:

- Windows
- Visual Studio
- C++20
- OpenGL
- vcpkg
- x64

Required Visual Studio workload:

- Desktop development with C++

### Implementation

- Visual Studio Solution / Project Settings
- vcpkg.json
- vcpkg-configuration.json

---

## Dependency Management

This project uses **vcpkg Manifest Mode**.

Current main dependencies:

- glad
- glfw3
- glm
- assimp

They are used for:

- GLAD    → OpenGL function loading
- GLFW    → Window creation, input and OpenGL context
- GLM     → Vector, matrix and quaternion mathematics
- Assimp  → 3D model, skeleton and animation loading

### Implementation

- vcpkg.json
- vcpkg-configuration.json

Install the dependencies declared in the manifest using vcpkg and the x64-windows-static triplet.

The project uses:

- x64-windows-static

which means the vcpkg libraries are linked statically.

## Recorded Versions and Python Configuration

| Component | Recorded project configuration |
| --- | --- |
| Windows | Windows 11, x64 |
| C++ | C++20, Visual Studio Desktop development with C++ workload |
| OpenGL | 3.3 Core |
| GLFW | 3.4 |
| vcpkg triplet | x64-windows-static |
| Python | 3.10.22 |
| PyTorch and torchaudio | Matching 2.11.0 releases from the cu130 wheel index |
| Qwen package | qwen-tts 0.1.1 |
| NumPy | 1.26.4 |
| sherpa-onnx | 1.13.8, Windows x64 shared MT Release, no-TTS package |
| Keyword model | kws-zipformer-gigaspeech-3.3M-2024-01-01 |
| TTS model | Qwen/Qwen3-TTS-12Hz-0.6B-Base |
| Whisper model | Use the model selected in server.py; its exact name was not provided |

Versions above describe the recorded development setup. Exact releases for Visual Studio, GLAD, GLM, Assimp, miniaudio, cpp-httplib, Flask, and Whisper have not been supplied.

### Python Setup Procedure

1. Create and activate a Conda environment named virtual-agent with Python 3.10.22, pip, and FFmpeg from conda-forge.
2. Update pip, setuptools, and wheel.
3. Install matching torch and torchaudio 2.11.0 packages from the PyTorch CUDA 13.0 wheel index.
4. Install NumPy 1.26.4 and qwen-tts 0.1.1.
5. Install Flask, openai-whisper, scipy, pandas, scikit-learn, librosa, soundfile, soxr, and sentencepiece.
6. Check dependency consistency and verify that PyTorch can access the GPU and perform a CUDA operation.
7. After both speech endpoints work, save the complete installed package versions in requirements-virtual-agent.txt.

The CUDA version of the PyTorch wheel is distinct from the locally installed CUDA toolkit version. This project uses PyTorch SDPA attention without a separate FlashAttention installation. The Python soxr package and the SoX executable are separate dependencies.

### C++ Audio and Wake-Word Configuration

- Add miniaudio for capture/playback and cpp-httplib for HTTP requests. Compile each header-only implementation in one translation unit only.
- Add sherpa-onnx include and library directories to Visual Studio, link the required C API import libraries, and place the package's runtime DLLs beside the executable.
- Keep the x64 architecture and MSVC runtime settings compatible with the chosen libraries. The shared sherpa-onnx runtime still needs its DLLs even though vcpkg dependencies use a static triplet.
- Extract the keyword model and configure encoder, decoder, joiner, and token-file paths.
- Configure the keyword file using the model's token format. The current example phrase is “hello, lily”.
- Match the recording format, keyword-model input format, and STT input format. Convert or resample where necessary.
- Set the local HTTP host/port consistently in both C++ engines and server.py. The existing service uses 127.0.0.1:5000.
- Set the actual character, shader, voice-prompt, and model paths. Python paths mentioned later are layout examples.

---

# 2. Character Asset Creation Pipeline

The current character was created through a multi-tool pipeline and exported as a single GLB containing the character appearance, skeleton and multiple animation clips.

**Flow:** AI Character Reference → TRELLIS → 3D Character Model → Mixamo → Skeleton + Animation FBX Files → Blender → Character + Skeleton + Multiple Actions → GLB → C++ / OpenGL.

The tools used in this pipeline are:

| Tool | Purpose |
| --- | --- |
| **AI Image Generation** | Created and refined an anime-style character reference, including face, clothing and body proportions. |
| **TRELLIS** | Converted the character image into a textured 3D model. |
| **Mixamo** | Automatically rigged the humanoid character and provided multiple skeletal animation clips. |
| **Blender** | Organized the model, skeleton, materials and animation actions, then exported them together as one GLB file. |

The current GLB contains multiple actions such as:

- idle
- run
- sitting
- standtosit
- talk
- walk

### Implementation

This stage is mainly completed outside the C++ project:

- TRELLIS
- Mixamo
- Blender

The exported GLB is then loaded in:

- Model.cpp
- Animation.cpp

---

# 3. Rendering and Model Loading

## 3.1 Mesh

A 3D model is composed of meshes.

A mesh mainly contains:

- Vertices
- Indices
- Normals
- Texture Coordinates
- Bone IDs
- Bone Weights

The mesh data is uploaded to the GPU using:

- VAO
- VBO
- EBO

### Implementation

Main files:

- Mesh.h
- Mesh.cpp
- Model.cpp

Relevant responsibilities:

- **Model.cpp:** Reads vertex data from Assimp.
- **→ Reads indices from aiFace.:** Creates Vertex structures.
- **Mesh.cpp:** Creates VAO / VBO / EBO.
- **→ Uploads vertex and index data to the GPU.:** Configures vertex attributes.
- → Draws the mesh with glDrawElements().

---

## 3.2 Material and Texture

Materials describe how the surface of the character looks.

Textures provide image information such as:

- Diffuse / Base Color
- Normal
- Metallic
- Roughness

The current project mainly uses the base-color / diffuse texture.

### Implementation

Main files:

- Model.cpp
- Mesh.cpp
- Fragment Shader

Relevant responsibilities:

- **Model.cpp:** Reads Assimp material information.
- **→ Loads embedded or external textures.:** Creates OpenGL textures.
- **Mesh.cpp:** Binds the texture before drawing.
- **Fragment Shader:** Samples diffuseTexture using TexCoords.

---

## 3.3 Basic Lighting

The current renderer includes:

- Ambient
- Diffuse
- Specular

Lighting depends on:

- Light Position
- Light Color
- Surface Normal
- Camera Position
- Fragment Position

### Implementation

Main file:

- Fragment Shader

The C++ side uploads lighting uniforms from:

- main.cpp

The Fragment Shader calculates:

- Ambient Lighting
- Diffuse Lighting
- Specular Lighting
- Final Surface Color

---

## 3.4 Camera and Model Transform

The renderer uses Model / View / Projection matrices.

- **Model Matrix:** Controls character position, rotation and scale.
- **View Matrix:** Represents the camera.
- **Projection Matrix:** Controls perspective projection.

### Implementation

Main files:

- Camera.h
- Camera.cpp
- main.cpp

Relevant responsibilities:

- **Camera.cpp:** Stores camera position, front and up vectors.
- **→ Builds the view matrix.:** Handles forward / backward / left / right movement.
- **main.cpp:** Builds model and projection matrices.
- → Uploads Model / View / Projection uniforms.

---

# 4. Skeleton and Skinning

## 4.1 Skeleton

An animated character contains a hierarchy of bones.

For example, the hips connect the upper-body chain and legs; the spine connects the head and arms.

The parent-child hierarchy determines how transformations propagate through the character.

### Implementation

Main files:

- Model.cpp
- Animation.cpp
- Animator.cpp

Relevant responsibilities:

- **Model.cpp:** Reads Assimp bone information.
- **→ Builds bone name → bone ID mapping.:** Stores inverse bind / offset matrices.
- **Animation.cpp:** Copies the node hierarchy from Assimp.
- **Animator.cpp:** Recursively propagates local transforms through the hierarchy.

---

## 4.2 Bone Weights

A vertex can be influenced by multiple bones.

For example, one vertex may receive weights of 0.7, 0.2, and 0.1 from three different bones.

The project stores up to four bone influences per vertex.

### Implementation

Main files:

- Model.h
- Model.cpp
- Mesh.cpp

Each vertex stores four integer bone IDs and four floating-point bone weights.

Responsibilities:

- **Model.cpp:** Reads aiBone::mWeights.
- → Converts bone → vertex data into vertex → bone influence data.
- **Mesh.cpp:** Uploads bone IDs as integer vertex attributes.
- → Uploads bone weights as floating-point vertex attributes.

---

## 4.3 Skinning

Skinning connects the mesh to the skeleton.

**Flow:** Bone Transform → Vertex Transformation → Mesh Deformation.

### Implementation

CPU side:

- Animator.cpp

GPU side:

- Vertex Shader

The CPU calculates final bone matrices:

**Matrix relationship:** global inverse transform × current global bone transform × inverse bind / offset matrix.

The GPU then combines bone influences for every vertex.

**Flow:** Final Bone Matrix × Original Vertex → Multiply by Bone Weight → Sum all Bone Influences → Skinned Vertex Position.

---

## 4.4 GPU Skinning

The CPU calculates the current bone transformations and sends them to the vertex shader.

**Flow:** CPU → Final Bone Matrices → Vertex Shader → GPU Skinning → Animated Vertices.

### Implementation

Main files:

- Animator.cpp
- main.cpp
- Vertex Shader

Responsibilities:

- **Animator.cpp:** Produces finalBoneMatrices.
- **main.cpp:** Uploads finalBonesMatrices[] uniforms to the shader.
- **Vertex Shader:** Reads aBoneIDs and aWeights.
- → Applies weighted bone transformations.

---

# 5. Animation Data

## 5.1 Animation Clip

A model can contain multiple animation clips.

Current examples:

- Idle
- Walk
- Run
- Talk
- Sitting
- StandToSit

Assimp exposes the model’s animation collection; the loader selects the requested clip by index.

A specific animation can be selected by its index.

### Implementation

Main files:

- Animation.h
- Animation.cpp

---

## 5.2 Animation Channel

An animation contains multiple channels.

Usually one channel corresponds to one animated node or bone.

Each channel contains:

- Position Keyframes
- Rotation Keyframes
- Scale Keyframes

### Implementation

Main files:

- Animation.h
- Animation.cpp

---

## 5.3 Animation Hierarchy

Animation channels contain keyframes, but the skeleton hierarchy is also required to calculate global transforms.

**Flow:** Root → Parent Bone → Child Bone → Child Bone.

### Implementation

Main file:

- Animation.cpp

The hierarchy is stored in the project’s AssimpNodeData representation.

---

# 6. Animator

The `Animator` is responsible for low-level animation playback and bone transformation calculation.

Its main responsibilities are:

- Animation Time
- Loop / Non-loop Playback
- Finished State
- Keyframe Sampling
- Bone Hierarchy Calculation
- Animation Blending
- Final Bone Matrices

### Implementation

Main files:

- Animator.h
- Animator.cpp

---

## 6.1 Animation Time

Real frame time is converted to animation time using:

- Animation Time += Ticks Per Second × Delta Time

### Implementation

---

## 6.2 Loop and Finished State

The Animator supports both looping and non-looping playback.

- **loop = true:** Animation repeats continuously.
- **loop = false:** Animation plays once and stops at the final frame.

### Implementation

This allows the high-level controller to decide whether an action should repeat continuously or be part of a sequence.

---

## 6.3 Keyframe Interpolation

The current animation time normally lies between two keyframes.

Position and scale use:

- Linear Interpolation

Rotation uses:

- Quaternion SLERP

### Implementation

Main file:

- Animator.cpp

Interpolation functions:

---

## 6.4 Bone Hierarchy Calculation

Every node has a local transform relative to its parent.

The global transform is calculated recursively:

**Matrix relationship:** global transform = parent global transform × current local transform.

The root traversal starts with an identity matrix because it has no parent transform.

### Implementation

Main file:

- Animator.cpp

---

# 7. Animation Controller

The `AnimationController` provides a higher-level interface for controlling character actions.

The important separation is:

- **AnimationController:** Decides WHAT should be played.
- **Animator:** Decides HOW the animation is played.

### Implementation

Main files:

- AnimationController.h
- AnimationController.cpp

---

## 7.1 Animation State

Character actions are represented by named states: Idle, Walk, Run, Talk, StandToSit, and Sitting.

### Implementation

Main file:

- AnimationController.h

---

## 7.2 Single-State Playback

A single action can be selected and continuously looped.

Example playback:

**Flow:** Walk → Walk → Walk → ....

### Implementation

Main file:

- AnimationController.cpp

The controller enables looping before starting the selected animation.

---

## 7.3 Animation Sequence

The controller supports ordered animation sequences.

Example:

**Flow:** Idle → Walk → Idle → Talk.

### Implementation

Main file:

- AnimationController.cpp

During sequence playback, looping is disabled for each individual step.

Each animation plays to completion before the controller advances to the next state.

---

## 7.4 Per-State Repeat Count

Each state in a sequence can define how many times it should play.

Each sequence step stores an animation state and a repeat count, which defaults to one.

Example playback:

**Flow:** Idle × 2 → Walk × 4 → Talk × 1.

### Implementation

Main files:

- AnimationController.h
- AnimationController.cpp

The logic is:

After each completion, the controller increases the repeat count. It replays the current state if more repetitions remain; otherwise it advances to the next sequence step.

---

# 8. Animation Blending

Animations are blended instead of switching instantly.

Without blending:

- Idle → Walk

With blending:

**Flow:** Idle → Idle + Walk → Walk.

### Implementation

Main file:

- Animator.cpp

---

## 8.1 Previous and Current Animation

When a new animation begins, the Animator stores:

- Previous Animation
- Previous Animation Time
- Current Animation
- Current Animation Time

The previous animation is not copied.

Instead, the Animator stores:

The Animator retains references to both clips and their playback times, then samples the required poses when needed.

### Implementation

Main file:

- Animator.cpp

---

## 8.2 Local Bone Transform Sampling

For blending, position, rotation, and scale are sampled separately and stored as a local pose for each bone.

Sampling process:

**Flow:** use the clip and playback time → find surrounding keyframes → interpolate position, rotation, and scale → build the local bone pose.

### Implementation

Main file:

- Animator.cpp

---

## 8.3 Blend Factor

The blend factor changes gradually from:

- 0.0 → Previous animation pose
- 0.5 → Mixed pose
- 1.0 → Current animation pose

It is calculated using:

The blend factor is elapsed transition time divided by transition duration, clamped to the transition range.

### Implementation

Main file:

- Animator.cpp

The default transition duration is 0.2 seconds.

---

## 8.4 Blending Position, Rotation and Scale

The two local bone transforms are blended separately.

- Position → glm::mix
- Rotation → glm::slerp
- Scale    → glm::mix

### Implementation

Main file:

- Animator.cpp

The blended Position / Rotation / Scale are then converted back into a local transformation matrix.

The local matrix is built as:

**Matrix relationship:** translation × rotation × scale.

---

# 9. Current Architecture

The current animation architecture is approximately:

The model loader supplies mesh and bone data. Animation supplies clips and the node hierarchy. AnimationController selects playback behavior; Animator produces bone matrices; the vertex shader deforms and renders the character.

---

# 10. File Responsibilities

## Model.h / Model.cpp

- Load GLB / GLTF model
- Load Mesh
- Load Material / Texture
- Read Bones
- Create Bone Mapping
- Read Bone Weights
- Store BoneInfo

Important concepts:

- aiScene
- aiNode
- aiMesh
- aiBone
- aiMaterial

---

## Mesh.h / Mesh.cpp

- VAO / VBO / EBO
- Vertex Attributes
- Texture Binding
- Bone ID Upload
- Bone Weight Upload
- Mesh Rendering

---

## Animation.h / Animation.cpp

- Load one animation clip
- Select animation by index
- Read animation channels
- Read Position / Rotation / Scale keyframes
- Copy skeleton hierarchy
- Store duration and ticks per second

---

## Animator.h / Animator.cpp

- Animation Time
- Loop / Non-loop Playback
- Finished Detection
- Keyframe Sampling
- Interpolation
- Bone Hierarchy Calculation
- Animation Blending
- Final Bone Matrices

---

## AnimationController.h / AnimationController.cpp

- Semantic Animation States
- Single-State Playback
- Animation Sequence
- Sequence Progression
- Per-State Repeat Count
- High-Level Animation Control

---

## Camera.h / Camera.cpp

- Camera Position
- Camera Direction
- View Matrix
- Camera Movement

---

## Vertex Shader

- Receive Vertex Position
- Receive Normal
- Receive TexCoords
- Receive Bone IDs
- Receive Bone Weights
- Receive finalBonesMatrices[]
- Perform GPU Skinning
- Calculate World Position
- Calculate Final gl_Position

---

## Fragment Shader

- Sample Character Texture
- Ambient Lighting
- Diffuse Lighting
- Specular Lighting
- Final Pixel Color

---

## main.cpp

- Create Window
- Initialize OpenGL
- Create Shader
- Load Character Model
- Create AnimationController
- Handle Input
- Update AnimationController
- Build Model / View / Projection Matrices
- Upload Lighting Uniforms
- Upload Final Bone Matrices
- Draw Character

---

# 11. Speech System — Qwen3-TTS Voice Cloning

The C++ character uses a local Python speech service powered by
`Qwen/Qwen3-TTS-12Hz-0.6B-Base`. This Base model supports reference-audio
voice cloning; the CustomVoice model is for preset speakers and is not used here.

There are two separate stages:

1. **Voice preparation (once per voice):** `test_qwen.py` extracts a voice prompt
   from `reference.wav` and its exact transcript, saves `character_voice.pt`,
   and generates `output_clone.wav` for listening.
2. **Runtime:** `server.py` loads the model and saved voice prompt once at startup.
   Each C++ request supplies new English text. The server returns signed 16-bit
   mono PCM in memory for miniaudio playback.

`character_voice.pt` contains voice-conditioning data, not the complete TTS model
or a playable recording. New sentences still require model inference. Normal runtime does not need to read the original reference WAV or write temporary WAV files.

## 11.1 Python Environment and Packages

The combined TTS/STT environment is documented in Section 1. Use the same environment for voice preparation and the server. It includes Qwen3-TTS, Whisper, PyTorch, torchaudio, Flask, NumPy, soundfile, and soxr.

The model and voice prompt are loaded at startup rather than for every request. Initial model downloads need network access; later runs can reuse cached weights.

## 11.2 Suggested Python File Layout

The examples below use `python/qwen_server/`. If your files are elsewhere,
adjust the terminal path; `Path(__file__)` resolves assets relative to each script.

| File | Responsibility |
| --- | --- |
| `python/qwen_server/test_qwen.py` | Extract and save the voice prompt; generate a preview |
| `python/qwen_server/reference.wav` | Clean source recording used during preparation |
| `python/qwen_server/character_voice.pt` | Saved model ID and CPU voice-prompt tensors |
| `python/qwen_server/output_clone.wav` | Preview for checking the cloned voice |
| `python/qwen_server/server.py` | Load the saved voice and serve C++ synthesis requests |
| `requirements-virtual-agent.txt` | Optional snapshot of the working Python packages |

## 11.3 Prepare the Reference Recording

Use a clean, single-speaker English recording of about 5–10 seconds as a starting
point. Avoid music, overlapping voices, clipping and long silences. The transcript
must match what was actually spoken, including any changes made while recording.

A reference transcript should reproduce the recording exactly. Example:

> Hello, this is my natural voice. I'm speaking clearly and calmly, just as I would in a normal conversation.

In this project `x_vector_only_mode=False`: the prompt retains reference speech
information and its transcript, not just a speaker embedding. A longer reference
can increase the context processed during generation. Compare short references
for both similarity and latency rather than assuming longer is better.

## 11.4 Create character_voice.pt — test_qwen.py

Keep test_qwen.py beside reference.wav, set the reference transcript to match the recording, run the preparation script, and listen to output_clone.wav.

The `.pt` file is generated locally, not downloaded as a ready-made character
voice. Re-run this script only when changing the reference or model, or rebuilding
the prompt after an incompatible package update. Restart the server after
replacing the voice file. This dictionary format is a project convention, not a
guaranteed version-independent Qwen interchange format; keep the working package
versions with it.

## 11.5 Use the Saved Voice — server.py

The server reads the model ID from `character_voice.pt`, reconstructs
`VoiceClonePromptItem` objects and loads the model once. It reuses both for every
request. C++ does not load the `.pt` file directly.

The lock prevents concurrent model generations. Disabling the reloader avoids
loading the GPU model twice during Flask development startup. Flask is the local
HTTP communication layer; Qwen3-TTS performs synthesis.

## 11.6 HTTP Contract and C++ Playback

Endpoint: `POST http://127.0.0.1:5000/tts`

Request content type: `application/json`. The request contains a text field with the sentence to synthesize.

| Response field | Meaning |
| --- | --- |
| HTTP status `200` | Successful synthesis |
| Body | Raw little-endian signed 16-bit PCM; no WAV header |
| `Content-Type` | `application/octet-stream` |
| `X-Sample-Rate` | Actual sample rate returned by Qwen; do not hard-code an earlier backend's rate |
| `X-Channels` | `1` for mono |
| HTTP status `400` | Invalid JSON or missing, non-string or empty text |

`TTSEngine.cpp` must check HTTP success before interpreting the body as PCM and
read the sample-rate/channel headers. `AudioPlayer.cpp` must configure its input
buffer using that metadata and retain sample memory during playback. The endpoint
and binary contract stay the same, so an existing client that follows them can
continue to work. Set the HTTP read timeout long enough for model inference.

## 11.7 Latency and Troubleshooting

This implementation returns the complete sentence after synthesis; it does not
stream audio. A saved prompt removes repeated reference-feature extraction but
does not remove the cost of generating new speech.

- RTF = generation seconds / generated audio seconds. RTF above 1 means synthesis
  takes longer than playback. The measured development runs were about 2.1–2.6;
  these are observations, not a performance guarantee.
- First-run model loading and first-request GPU initialization can add delay.
- Compare the same short sentence after warm-up when testing different references.
- A shorter clean reference may reduce context cost; regenerate the `.pt` file
  and restart the server to apply it.
- `Setting pad_token_id to eos_token_id` is an informational generation message.
- `No module named soxr`: run `python -m pip install soxr` in the active environment.
- If CUDA is unavailable or a GPU-kernel compatibility error occurs, verify the
  Python interpreter, NVIDIA driver and CUDA-enabled PyTorch installation.
- If the audio sounds too fast, slow or pitched, check the C++ sample-rate settings.
- Use a background C++ request worker to keep rendering responsive during inference;
  this does not by itself make the model generate faster.

Sources: [Qwen3-TTS official repository](https://github.com/QwenLM/Qwen3-TTS),
[PyTorch installation guide](https://pytorch.org/get-started/locally/).

---

## 11.8 AudioBuffer

AudioBuffer stores signed 16-bit PCM samples, sample rate, and channel count.

Responsibilities:

- **samples:** Raw signed 16-bit PCM samples.
- **sampleRate:** Number of audio frames per second.
- **channels:** 1 = mono
- → 2 = stereo

---

## 11.9 C++ TTSEngine

Main files:

- audio/TTSEngine.h
- audio/TTSEngine.cpp

The C++ `TTSEngine` does not run the neural network directly.

Its responsibilities are:

**Flow:** Receive std::string text → Send HTTP POST request → Receive PCM bytes → Read sample rate / channel metadata → Create AudioBuffer.

The HTTP client is implemented using:

- cpp-httplib

The project uses the header-only file:

- external/httplib/httplib.h

---

## 11.10 cpp-httplib

`cpp-httplib` is used by the C++ TTS client to communicate with the local Python server.

PCM playback activates Talk, and the application returns to Idle after playback ends.

Visual Studio include path:

- $(ProjectDir)external\httplib

The TTS client sends:

- POST http://127.0.0.1:5000/tts

and receives raw PCM data.

---

## 11.11 AudioPlayer

Main files:

- audio/AudioPlayer.h
- audio/AudioPlayer.cpp

`AudioPlayer` uses:

- miniaudio

to play PCM audio directly from memory.

The internal implementation follows the PImpl pattern:

**Flow:** AudioPlayer → Impl* → ma_engine → ma_audio_buffer → ma_sound.

Responsibilities:

- **ma_engine:** Global miniaudio playback engine.
- **ma_audio_buffer:** Wraps PCM samples stored in memory.
- **ma_sound:** Playable sound object created from the PCM buffer.

The player keeps its own sample storage in currentSamples.

This is required because miniaudio playback is asynchronous.

Without an owned copy:

**Flow:** AudioBuffer destroyed → PCM memory released → miniaudio still reads old pointer → Access violation.

With `currentSamples`:

**Flow:** AudioPlayer::Impl → owns PCM memory → miniaudio can safely read it while playing.

---

## 11.12 Speech Integration in main.cpp

The speech system is integrated into the main character application.

During testing, a key can trigger speech:

**Flow:** Press T → Generate speech → Play PCM → Switch animation to Talk.

Speech synthesis is requested through a background worker so that the render loop can continue while Python generates audio.

The worker receives the text, waits for the HTTP response, and makes the completed AudioBuffer available for playback. The application updates the Talk/Idle animation from actual playback state. Generation and playback are separate phases: waiting for synthesis does not mean the character is already speaking.

---

## 11.13 Speech System Runtime Flow

The complete current runtime pipeline is:

**Flow:** User / C++ Trigger → Text → TTSEngine::synthesize() → cpp-httplib → HTTP POST /tts → Python Flask Server → Python Generates Speech PCM Using the Character's Voice → HTTP Binary Response → C++ AudioBuffer → AudioPlayer → ma_audio_buffer → ma_sound → miniaudio → Speaker.

Speech and character animation can be triggered together:

PCM playback activates Talk, and the application returns to Idle after playback ends.

---

## 11.14 Speech-Related File Responsibilities

### Python

- python/qwen_server/server.py

Responsibilities:

- Flask HTTP server
- Load Qwen3-TTS model
- Receive text
- Generate speech
- Convert float32 PCM to int16 PCM
- Return raw audio bytes
- Return sample-rate/channel metadata

### C++

- audio/TTSEngine.h
- audio/TTSEngine.cpp

Responsibilities:

- Send text to Python server
- Receive binary PCM response
- Parse audio metadata
- Build AudioBuffer

- audio/AudioBuffer.h

Responsibilities:

- Store PCM samples
- Store sample rate
- Store channel count

- audio/AudioPlayer.h
- audio/AudioPlayer.cpp

Responsibilities:

- Initialize miniaudio
- Own PCM memory during playback
- Create ma_audio_buffer
- Create ma_sound
- Start audio playback

- main.cpp

Responsibilities:

- Create TTSEngine
- Create AudioPlayer
- Trigger speech
- Trigger Talk animation
- Continue rendering the character

---

## 11.15 Running the Speech System

Activate the Conda environment used for both TTS and STT.

Start server.py from its actual directory.

Keep this terminal running.

Then start the C++ Virtual Agent from Visual Studio.

Runtime:

- **Terminal 1:** Python TTS Server
- **Visual Studio:** C++ Virtual Agent

The C++ process sends text to the local Python server through:

- 127.0.0.1:5000

No external network connection is required for the C++ ↔ Python communication.

---

# 12. Microphone Recording — AudioRecorder

## 12.1 Purpose

AudioRecorder captures microphone input with miniaudio and delivers small PCM chunks to the application. It handles the recording device; interpretation of those chunks belongs to SpeechRecognizer and WakeWordDetector.

### Implementation

Main files: AudioRecorder.h and AudioRecorder.cpp. Directory prefixes depend on the actual project layout.

## 12.2 Recording Lifecycle

**Flow:** configure capture device → register callback → start microphone → receive chunks → stop device → release resources.

The capture configuration defines the sample format, sample rate, and channel count. The device's physical format is not automatically the same as the application's requested PCM format.

AudioRecorder converts or packages incoming data into the project's AudioBuffer representation before delivering it to the next component.

## 12.3 Callback Registration

When the application starts recording, it supplies a callback that forwards each AudioBuffer to SpeechRecognizer's processAudioChunk operation.

AudioRecorder stores that callback in its internal implementation. Moving the callback into internal storage preserves it after the start operation returns. Saving a callback does not execute it: the recorder invokes it when audio arrives.

The callback captures SpeechRecognizer by reference, so that object must remain alive while recording can invoke it. Stop capture before destroying the recognizer.

## 12.4 Capture and Processing Boundaries

The microphone stays active while the application waits for a wake word. Continuous capture provides the audio needed by both the keyword detector and rolling buffer.

The capture path should remain short. Network requests and Whisper inference belong to a separate processing worker rather than the device callback.

---

# 13. Wake-Word Detection — WakeWordDetector

## 13.1 Purpose

WakeWordDetector uses sherpa-onnx to detect the configured keyword in incoming microphone audio. Its output is a trigger event, rather than a full transcription of the user's sentence.

### Implementation

Main files: WakeWordDetector.h and WakeWordDetector.cpp.

## 13.2 Model and Runtime

The recorded setup uses sherpa-onnx 1.13.8 and the kws-zipformer-gigaspeech-3.3M-2024-01-01 model. The native C++ runtime loads the model locally; keyword detection does not require a request to the Flask service.

Model configuration includes the encoder, decoder, joiner, token list, keyword file, and the model's expected audio format. Keep these files available at the configured paths.

## 13.3 Detection Flow

**Flow:** incoming chunk → format conversion if needed → keyword stream → decode available audio → inspect result → notify SpeechRecognizer.

Signed 16-bit application audio can be converted to normalized floating-point samples for the detector. Input sample rate must match the model or be resampled appropriately.

The detector processes successive chunks as a continuous stream. After a detection, stream/result handling must follow the implementation's reset policy so that an old result does not repeatedly trigger recording.

## 13.4 Wake Word and Command

The wake phrase activates command recording. SpeechRecognizer collects the surrounding audio and subsequent speech; Whisper later transcribes the completed segment.

Whether the wake phrase remains in the transcription depends on the captured segment and any explicit text filtering. Keyword detection alone does not remove it from PCM.

---

# 14. Recording Coordination — SpeechRecognizer

## 14.1 Purpose

SpeechRecognizer connects microphone chunks, keyword detection, triggered recording, and the STT queue. It coordinates when audio becomes a transcription request; STTEngine performs the HTTP exchange.

### Implementation

Main files: SpeechRecognizer.h and SpeechRecognizer.cpp.

## 14.2 Rolling Buffer

The rollingBuffer stores a bounded window of recent microphone audio. Older samples are discarded as newer samples arrive.

Its purpose is to preserve speech near the trigger, because the wake-word result arrives after some of the relevant sound has already been captured. It is recent history, rather than an unlimited archive of the microphone stream.

The configured history length, sample rate, and channel count determine its sample capacity.

## 14.3 Wake-Word Notification

When notifyWakeWordDetected is called, the recognizer begins the triggered recording and copies the rolling buffer into recordedSamples according to its state policy.

This copy supplies the audio that existed at detection time. It does not automatically collect future chunks.

## 14.4 Appending New Chunks

While recording is active, processAudioChunk appends newly arriving samples to recordedSamples. This collects the rest of the command after the wake word.

| Audio source | When it is copied | Why it is needed |
| --- | --- | --- |
| rollingBuffer | At wake-word detection | Preserve recent audio around and before the trigger |
| Incoming chunk samples | During active recording | Continue collecting the spoken command |

The chunk that triggers detection must be counted once. If it has already entered the copied rolling buffer, the same samples should not also be appended as new audio. The exact branch order must be checked in the source implementation.

## 14.5 Completing a Recording

Once the configured recording-end condition is reached, the recognizer packages the collected samples and metadata into an AudioBuffer and places it in the transcription queue.

The supplied README and snippets do not identify the exact duration or stopping algorithm. This document therefore does not assume a particular silence detector, VAD, or fixed recording timeout.

**Flow:** wait for keyword → copy recent history → append subsequent chunks → reach recording-end condition → enqueue completed audio → resume waiting according to the state policy.

## 14.6 Queue and Worker

The worker takes a completed recording from the queue and calls STTEngine. Queue transfer separates audio capture from the slower HTTP and inference operations.

Moving a completed AudioBuffer into the queue transfers ownership of its samples without requiring another full vector copy. The recording path then prepares storage for the next segment.

Shared queue access requires synchronization. Any mutex-protected queue should release its lock before the worker waits for an HTTP response.

---

# 15. Speech to Text — STTEngine and Whisper

## 15.1 C++ STTEngine

STTEngine sends recorded audio to the local Python service and returns the recognized text. It does not execute the Whisper neural network inside C++.

### Implementation

Main files: STTEngine.h and STTEngine.cpp.

**Flow:** completed AudioBuffer → HTTP POST to /stt → Python transcription → response parsing → recognized text.

The input is provided as a read-only AudioBuffer reference. The client must preserve the audio format expected by the server and handle HTTP failures before treating a response as a successful transcript.

## 15.2 Whisper in server.py

The Python server now provides both TTS and STT. Whisper loads using the selected model configuration and transcribes completed microphone recordings.

The STT path receives signed 16-bit PCM bytes, interprets their byte order consistently, converts samples to floating point, and normalizes their amplitudes by 32768. Whisper's waveform input must be mono at 16 kHz; recording or server-side preprocessing must provide that format.

The exact Whisper model name, release, response schema, and metadata transport were not included in the supplied source material. Use the current server.py and STTEngine implementation as the authority for these settings.

## 15.3 STT Request and Response

| Part | Meaning |
| --- | --- |
| Endpoint | POST /stt on the local Flask service |
| Request audio | Raw signed 16-bit PCM, interpreted consistently by both sides |
| Sample rate and channels | Agreed in configuration or conveyed using the implemented request metadata |
| Preprocessing | Convert to normalized floating-point mono audio at the required sample rate |
| Response | Recognized text in the format parsed by STTEngine |

Raw PCM has no WAV header and does not describe its own sample rate. A mismatch can distort timing, reduce recognition quality, or produce invalid input.

## 15.4 Worker and Result Handling

Whisper runs after a complete audio segment is queued. Rendering and microphone capture can continue while transcription is pending.

The recognized text is the output of this stage. Connecting it to an LLM, command executor, or automatic spoken reply is a later integration step unless separately implemented.

---

# 16. Shared Speech Runtime and Integration

## 16.1 Local Server

server.py hosts the two speech endpoints:

| Endpoint | Input | Processing | Output |
| --- | --- | --- | --- |
| /tts | Text | Qwen3-TTS with the saved voice prompt | PCM plus audio metadata |
| /stt | Recorded PCM | Whisper | Recognized text |

Load models once at startup and reuse them. Avoid duplicate initialization from a development reloader. TTS and STT share GPU resources when both use CUDA; a C++ worker alone does not guarantee simultaneous model inference or lower inference latency.

## 16.2 Audio Ownership and Threading

| Area | Responsibility |
| --- | --- |
| Rendering thread | Update character state, animation, and drawing |
| Capture callback | Receive microphone chunks and forward them for bounded processing |
| STT worker | Consume completed recordings and wait for transcription |
| TTS worker | Request generated speech without blocking rendering |
| Playback storage | Keep generated samples alive until playback ends |

Synchronization protects data shared between these areas. Callback targets, queued buffers, and playback samples must remain valid for as long as their consumers need them.

At shutdown, stop capture before destroying its callback targets, signal workers to finish, and wait for worker completion before releasing the objects they use. This describes the intended lifecycle; exact shutdown mechanics depend on the source.

## 16.3 Application Startup

1. Activate the configured Python environment and start server.py.
2. Allow Qwen3-TTS, the voice prompt, and Whisper to load.
3. Launch the C++ application with the configured model and runtime paths.
4. Initialize rendering, animation, playback, keyword detection, and speech coordination.
5. Start microphone capture with its forwarding callback.
6. Keep updating animation and rendering while speech workers handle requests.

## 16.4 Runtime Paths

**Speech input:** microphone → AudioRecorder → SpeechRecognizer and WakeWordDetector → completed recording queue → STTEngine → Whisper → text.

**Speech output:** supplied text → TTSEngine → Qwen3-TTS → AudioBuffer → AudioPlayer → sound.

**Animation:** playback starts → Talk state; playback finishes → Idle state. The existing animation blending smooths these transitions, but does not provide phoneme-level lip synchronization.

The input and output paths are implemented building blocks. Automatic conversation requires an additional response-generation connection between recognized text and TTS.

## 16.5 Configuration and Verification

| Check | Expected observation |
| --- | --- |
| Asset loading | Character, textures, and animation clips load correctly |
| Animation | States, sequences, repeat counts, and blending work |
| TTS | A supplied sentence produces the chosen voice and correct playback speed |
| Recording | Microphone chunks arrive with consistent PCM metadata |
| Keyword detection | The configured wake phrase activates command recording |
| Recording boundary | Recent history and subsequent chunks form one segment without duplicating the trigger chunk |
| STT | Whisper returns text from the completed segment |
| Responsiveness | Rendering continues during speech requests |

These are verification steps for the local project, not checks executed against the C++ source during this documentation update.

---

# 17. Current Progress

The following major milestones have now been completed:

- Created a custom anime-style character reference.
- Converted the character reference into a textured 3D model using TRELLIS.
- Added a humanoid skeleton using Mixamo.
- Downloaded multiple Mixamo animation clips.
- Organized the model, skeleton and animation actions in Blender.
- Exported the character and multiple animations into one GLB.
- Loaded the custom character in the C++ / OpenGL renderer.
- Loaded multiple animation clips with Assimp.
- Implemented mesh, texture and basic lighting.
- Implemented skeleton and bone mapping.
- Implemented bone weights.
- Implemented skeletal animation.
- Implemented keyframe interpolation.
- Implemented GPU skinning.
- Added runtime switching between animation clips.
- Added an `AnimationController`.
- Added continuously looping single-state playback.
- Added ordered animation sequences.
- Added configurable repetition counts for every sequence state.
- Added animation completion detection.
- Added animation blending between clips.
- Added a Python Qwen3-TTS voice-cloning speech backend.
- Saved a reusable character voice prompt in `character_voice.pt`.
- Added C++ `TTSEngine` HTTP communication.
- Added in-memory PCM audio transfer.
- Added miniaudio-based PCM playback.
- Integrated speech triggering with the character Talk animation and return to Idle.
- Moved speech requests away from the render loop.
- Added AudioRecorder with microphone callbacks.
- Added sherpa-onnx WakeWordDetector.
- Added SpeechRecognizer with rolling history and triggered recording.
- Added an audio queue and STT worker.
- Added STTEngine and the Whisper /stt endpoint in server.py.

---

# 18. Next Learning Steps

The basic body-animation framework is now functional.

The next major topics are dialogue integration, facial animation, and higher-level character behavior.

## Dialogue Integration

Pass recognized text to a response-generation or command layer, then send the resulting reply to TTSEngine. Define turn handling and the behavior of microphone input during character playback.

## Blendshape / Morph Target

Skeletal animation mainly controls the body.

Blendshapes are commonly used for facial animation.

Examples:

Possible shapes include a neutral face, smile, blink, open mouth, and raised eyebrows.

Possible future implementation files:

- Model.cpp
- Mesh.cpp
- Vertex Shader
- FacialAnimationController.cpp

---

## Facial Animation

Future controls may include:

- Eyes
- Eyebrows
- Mouth
- Facial Expressions

using blendshapes / morph targets.

---

## Lip Synchronization

Future pipeline:

**Flow:** Speech → Phoneme / Viseme → Blendshape → Mouth Movement.

Possible future components:

- Speech System
- LipSync Controller
- Facial Animation Controller

---

## Character Behavior

Higher-level logic can eventually generate action sequences.

Example:

**Flow:** AI Decision → Animation Sequence → Walk × 3 → Talk × 1 → Idle.

The future AI layer should request semantic actions such as:

- Walk
- Talk
- Sit
- Idle

instead of directly manipulating:

- Animation Index
- Animation Time
- Bone Matrices

---

# 19. Long-Term Direction

The project can gradually evolve into:

**Flow:** 3D Character → Animation System → Animation Controller → Facial Animation → Speech / Lip Sync → Perception → AI / LLM → Interactive Virtual Agent.

The current foundation covers character rendering, body animation, speech generation, and wake-word-triggered speech recognition. The next goal is a connected interaction loop with response generation and coordinated character behavior.
