# Virtual Agent

A C++ / OpenGL learning project for understanding how a 3D animated virtual character works.

The current goal is to learn the complete character rendering and animation pipeline step by step, and later extend it toward an interactive AI virtual agent.

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

```text
Desktop development with C++
```

### Implementation

```text
Visual Studio Solution / Project Settings
vcpkg.json
vcpkg-configuration.json
```

---

## Dependency Management

This project uses **vcpkg Manifest Mode**.

Current main dependencies:

```text
glad
glfw3
glm
assimp
```

They are used for:

```text
GLAD    → OpenGL function loading
GLFW    → Window creation, input and OpenGL context
GLM     → Vector, matrix and quaternion mathematics
Assimp  → 3D model, skeleton and animation loading
```

### Implementation

```text
vcpkg.json
vcpkg-configuration.json
```

Install dependencies with:

```powershell
vcpkg install --triplet x64-windows-static
```

The project uses:

```text
x64-windows-static
```

which means the vcpkg libraries are linked statically.

---

# 2. Character Asset Creation Pipeline

The current character was created through a multi-tool pipeline and exported as a single GLB containing the character appearance, skeleton and multiple animation clips.

```text
AI Character Reference
        ↓
TRELLIS
        ↓
3D Character Model
        ↓
Mixamo
        ↓
Skeleton + Animation FBX Files
        ↓
Blender
        ↓
Character + Skeleton + Multiple Actions
        ↓
GLB
        ↓
C++ / OpenGL
```

The tools used in this pipeline are:

| Tool | Purpose |
| --- | --- |
| **AI Image Generation** | Created and refined an anime-style character reference, including face, clothing and body proportions. |
| **TRELLIS** | Converted the character image into a textured 3D model. |
| **Mixamo** | Automatically rigged the humanoid character and provided multiple skeletal animation clips. |
| **Blender** | Organized the model, skeleton, materials and animation actions, then exported them together as one GLB file. |

The current GLB contains multiple actions such as:

```text
idle
run
sitting
standtosit
talk
walk
```

### Implementation

This stage is mainly completed outside the C++ project:

```text
TRELLIS
Mixamo
Blender
```

The exported GLB is then loaded in:

```text
Model.cpp
Animation.cpp
```

---

# 3. Rendering and Model Loading

## 3.1 Mesh

A 3D model is composed of meshes.

A mesh mainly contains:

```text
Vertices
Indices
Normals
Texture Coordinates
Bone IDs
Bone Weights
```

The mesh data is uploaded to the GPU using:

```text
VAO
VBO
EBO
```

### Implementation

Main files:

```text
Mesh.h
Mesh.cpp
Model.cpp
```

Relevant responsibilities:

```text
Model.cpp
→ Reads vertex data from Assimp.
→ Reads indices from aiFace.
→ Creates Vertex structures.

Mesh.cpp
→ Creates VAO / VBO / EBO.
→ Uploads vertex and index data to the GPU.
→ Configures vertex attributes.
→ Draws the mesh with glDrawElements().
```

---

## 3.2 Material and Texture

Materials describe how the surface of the character looks.

Textures provide image information such as:

```text
Diffuse / Base Color
Normal
Metallic
Roughness
```

The current project mainly uses the base-color / diffuse texture.

### Implementation

Main files:

```text
Model.cpp
Mesh.cpp
Fragment Shader
```

Relevant responsibilities:

```text
Model.cpp
→ Reads Assimp material information.
→ Loads embedded or external textures.
→ Creates OpenGL textures.

Mesh.cpp
→ Binds the texture before drawing.

Fragment Shader
→ Samples diffuseTexture using TexCoords.
```

Relevant functions include:

```cpp
loadMaterialTexture()
createTextureFromData()
Mesh::draw()
```

---

## 3.3 Basic Lighting

The current renderer includes:

```text
Ambient
Diffuse
Specular
```

Lighting depends on:

```text
Light Position
Light Color
Surface Normal
Camera Position
Fragment Position
```

### Implementation

Main file:

```text
Fragment Shader
```

The C++ side uploads lighting uniforms from:

```text
main.cpp
```

Typical uniforms:

```cpp
lightPos
lightColor
viewPos
```

The Fragment Shader calculates:

```text
Ambient Lighting
Diffuse Lighting
Specular Lighting
Final Surface Color
```

---

## 3.4 Camera and Model Transform

The renderer uses Model / View / Projection matrices.

```text
Model Matrix
→ Controls character position, rotation and scale.

View Matrix
→ Represents the camera.

Projection Matrix
→ Controls perspective projection.
```

### Implementation

Main files:

```text
Camera.h
Camera.cpp
main.cpp
```

Relevant responsibilities:

```text
Camera.cpp
→ Stores camera position, front and up vectors.
→ Builds the view matrix.
→ Handles forward / backward / left / right movement.

main.cpp
→ Builds model and projection matrices.
→ Uploads Model / View / Projection uniforms.
```

---

# 4. Skeleton and Skinning

## 4.1 Skeleton

An animated character contains a hierarchy of bones.

Example:

```text
Root
 └── Hips
      ├── Spine
      │    ├── Head
      │    ├── Left Arm
      │    └── Right Arm
      ├── Left Leg
      └── Right Leg
```

The parent-child hierarchy determines how transformations propagate through the character.

### Implementation

Main files:

```text
Model.cpp
Animation.cpp
Animator.cpp
```

Relevant responsibilities:

```text
Model.cpp
→ Reads Assimp bone information.
→ Builds bone name → bone ID mapping.
→ Stores inverse bind / offset matrices.

Animation.cpp
→ Copies the node hierarchy from Assimp.

Animator.cpp
→ Recursively propagates local transforms through the hierarchy.
```

---

## 4.2 Bone Weights

A vertex can be influenced by multiple bones.

Example:

```text
Vertex
 ├── Bone A → 0.7
 ├── Bone B → 0.2
 └── Bone C → 0.1
```

The project stores up to four bone influences per vertex.

### Implementation

Main files:

```text
Model.h
Model.cpp
Mesh.cpp
```

The `Vertex` structure contains:

```cpp
glm::ivec4 boneIDs;
glm::vec4 weights;
```

Relevant function:

```cpp
extractBoneWeightForVertices()
```

Responsibilities:

```text
Model.cpp
→ Reads aiBone::mWeights.
→ Converts bone → vertex data into vertex → bone influence data.

Mesh.cpp
→ Uploads bone IDs as integer vertex attributes.
→ Uploads bone weights as floating-point vertex attributes.
```

---

## 4.3 Skinning

Skinning connects the mesh to the skeleton.

```text
Bone Transform
      ↓
Vertex Transformation
      ↓
Mesh Deformation
```

### Implementation

CPU side:

```text
Animator.cpp
```

GPU side:

```text
Vertex Shader
```

The CPU calculates final bone matrices:

```text
Global Inverse Transform
×
Current Global Bone Transform
×
Inverse Bind / Offset Matrix
```

The GPU then combines bone influences for every vertex.

Conceptually:

```text
Final Bone Matrix × Original Vertex
            ↓
Multiply by Bone Weight
            ↓
Sum all Bone Influences
            ↓
Skinned Vertex Position
```

---

## 4.4 GPU Skinning

The CPU calculates the current bone transformations and sends them to the vertex shader.

```text
CPU
 ↓
Final Bone Matrices
 ↓
Vertex Shader
 ↓
GPU Skinning
 ↓
Animated Vertices
```

### Implementation

Main files:

```text
Animator.cpp
main.cpp
Vertex Shader
```

Responsibilities:

```text
Animator.cpp
→ Produces finalBoneMatrices.

main.cpp
→ Uploads finalBonesMatrices[] uniforms to the shader.

Vertex Shader
→ Reads aBoneIDs and aWeights.
→ Applies weighted bone transformations.
```

---

# 5. Animation Data

## 5.1 Animation Clip

A model can contain multiple animation clips.

Current examples:

```text
Idle
Walk
Run
Talk
Sitting
StandToSit
```

Assimp loads these animations from:

```cpp
scene->mAnimations
```

A specific animation can be selected by its index.

### Implementation

Main files:

```text
Animation.h
Animation.cpp
```

Relevant constructor:

```cpp
Animation(
    const std::string& animationPath,
    Model* model,
    unsigned int animationIndex
);
```

---

## 5.2 Animation Channel

An animation contains multiple channels.

Usually one channel corresponds to one animated node or bone.

Each channel contains:

```text
Position Keyframes
Rotation Keyframes
Scale Keyframes
```

### Implementation

Main files:

```text
Animation.h
Animation.cpp
```

Relevant structures:

```cpp
PositionKey
RotationKey
ScaleKey
BoneAnimation
```

Assimp source type:

```cpp
aiNodeAnim
```

---

## 5.3 Animation Hierarchy

Animation channels contain keyframes, but the skeleton hierarchy is also required to calculate global transforms.

```text
Root
 ↓
Parent Bone
 ↓
Child Bone
 ↓
Child Bone
```

### Implementation

Main file:

```text
Animation.cpp
```

Relevant function:

```cpp
readHierarchyData()
```

The hierarchy is stored in:

```cpp
AssimpNodeData
```

---

# 6. Animator

The `Animator` is responsible for low-level animation playback and bone transformation calculation.

Its main responsibilities are:

```text
Animation Time
Loop / Non-loop Playback
Finished State
Keyframe Sampling
Bone Hierarchy Calculation
Animation Blending
Final Bone Matrices
```

### Implementation

Main files:

```text
Animator.h
Animator.cpp
```

---

## 6.1 Animation Time

Real frame time is converted to animation time using:

```text
Animation Time += Ticks Per Second × Delta Time
```

### Implementation

Relevant function:

```cpp
Animator::updateAnimation()
```

---

## 6.2 Loop and Finished State

The Animator supports both looping and non-looping playback.

```text
loop = true
→ Animation repeats continuously.

loop = false
→ Animation plays once and stops at the final frame.
```

### Implementation

Relevant functions:

```cpp
Animator::setLoop()
Animator::isFinished()
Animator::updateAnimation()
Animator::playAnimation()
```

This allows the high-level controller to decide whether an action should repeat continuously or be part of a sequence.

---

## 6.3 Keyframe Interpolation

The current animation time normally lies between two keyframes.

Position and scale use:

```text
Linear Interpolation
```

Rotation uses:

```text
Quaternion SLERP
```

### Implementation

Main file:

```text
Animator.cpp
```

Relevant functions:

```cpp
Animator::sampleBoneTransform()
Animator::getPositionIndex()
Animator::getRotationIndex()
Animator::getScaleIndex()
```

Interpolation functions:

```cpp
glm::mix()
glm::slerp()
```

---

## 6.4 Bone Hierarchy Calculation

Every node has a local transform relative to its parent.

The global transform is calculated recursively:

```text
Global Transform
=
Parent Global Transform
×
Current Local Transform
```

The root starts with the identity matrix:

```cpp
glm::mat4(1.0f)
```

because it has no parent transform.

### Implementation

Main file:

```text
Animator.cpp
```

Relevant function:

```cpp
Animator::calculateBoneTransform()
```

---

# 7. Animation Controller

The `AnimationController` provides a higher-level interface for controlling character actions.

The important separation is:

```text
AnimationController
→ Decides WHAT should be played.

Animator
→ Decides HOW the animation is played.
```

### Implementation

Main files:

```text
AnimationController.h
AnimationController.cpp
```

---

## 7.1 Animation State

Character actions are represented by an enum.

Example:

```cpp
enum class AnimationState
{
    Idle,
    Walk,
    Run,
    Talk,
    StandToSit,
    Sitting
};
```

### Implementation

Main file:

```text
AnimationController.h
```

---

## 7.2 Single-State Playback

A single action can be selected and continuously looped.

Example:

```cpp
animationController.playState(
    AnimationState::Walk
);
```

Result:

```text
Walk
 ↓
Walk
 ↓
Walk
 ↓
...
```

### Implementation

Main file:

```text
AnimationController.cpp
```

Relevant functions:

```cpp
AnimationController::playState()
AnimationController::playRawState()
```

The controller configures:

```cpp
animator.setLoop(true);
```

before starting the selected animation.

---

## 7.3 Animation Sequence

The controller supports ordered animation sequences.

Example:

```text
Idle
  ↓
Walk
  ↓
Idle
  ↓
Talk
```

### Implementation

Main file:

```text
AnimationController.cpp
```

Relevant functions:

```cpp
AnimationController::playSequence()
AnimationController::update()
AnimationController::playNextState()
AnimationController::playRawState()
```

During sequence playback:

```cpp
animator.setLoop(false);
```

Each animation plays to completion before the controller advances to the next state.

---

## 7.4 Per-State Repeat Count

Each state in a sequence can define how many times it should play.

The sequence step is represented as:

```cpp
struct StateStep
{
    AnimationState state;
    int repeatCount = 1;
};
```

Example:

```cpp
animationController.playSequence(
    {
        { AnimationState::Idle, 2 },
        { AnimationState::Walk, 4 },
        { AnimationState::Talk, 1 }
    }
);
```

Result:

```text
Idle × 2
   ↓
Walk × 4
   ↓
Talk × 1
```

### Implementation

Main files:

```text
AnimationController.h
AnimationController.cpp
```

Relevant members:

```cpp
std::vector<StateStep> actionSequence;
size_t currentActionIndex;
int currentRepeatCount;
bool sequencePlaying;
```

Relevant function:

```cpp
AnimationController::playNextState()
```

The logic is:

```text
Current Animation Finished
          ↓
Increase Repeat Count
          ↓
Reached repeatCount?
     ↓             ↓
    No            Yes
     ↓             ↓
Replay State   Next State
```

---

# 8. Animation Blending

Animations are blended instead of switching instantly.

Without blending:

```text
Idle → Walk
```

With blending:

```text
Idle
  ↓
Idle + Walk
  ↓
Walk
```

### Implementation

Main file:

```text
Animator.cpp
```

Relevant functions:

```cpp
Animator::playAnimation()
Animator::updateAnimation()
Animator::calculateBoneTransform()
Animator::sampleBoneTransform()
Animator::blendTransforms()
Animator::buildTransformMatrix()
```

---

## 8.1 Previous and Current Animation

When a new animation begins, the Animator stores:

```text
Previous Animation
Previous Animation Time
Current Animation
Current Animation Time
```

Conceptually:

```cpp
previousAnimation = currentAnimation;
previousTime = currentTime;

currentAnimation = animation;
currentTime = 0.0f;
```

The previous animation is not copied.

Instead, the Animator stores:

```text
Animation Pointer
+
Animation Time
```

and samples the required pose when needed.

### Implementation

Main file:

```text
Animator.cpp
```

Relevant function:

```cpp
Animator::playAnimation()
```

---

## 8.2 Local Bone Transform Sampling

For blending, Position / Rotation / Scale are sampled separately.

```cpp
struct LocalBoneTransform
{
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
};
```

Sampling process:

```text
Bone Animation
+
Animation Time
       ↓
Find Surrounding Keyframes
       ↓
Interpolate Position
Interpolate Rotation
Interpolate Scale
       ↓
Local Bone Transform
```

### Implementation

Main file:

```text
Animator.cpp
```

Relevant function:

```cpp
Animator::sampleBoneTransform()
```

---

## 8.3 Blend Factor

The blend factor changes gradually from:

```text
0.0 → Previous animation pose
0.5 → Mixed pose
1.0 → Current animation pose
```

It is calculated using:

```text
blendFactor
=
blendTime / blendDuration
```

### Implementation

Main file:

```text
Animator.cpp
```

Relevant variables:

```cpp
bool blending;
float blendTime;
float blendDuration;
```

The default transition duration is defined in:

```cpp
void playAnimation(
    Animation* animation,
    float blendDuration = 0.2f
);
```

Therefore:

```cpp
animator.playAnimation(&walk);
```

uses a default blending duration of:

```text
0.2 seconds
```

---

## 8.4 Blending Position, Rotation and Scale

The two local bone transforms are blended separately.

```text
Position → glm::mix
Rotation → glm::slerp
Scale    → glm::mix
```

### Implementation

Main file:

```text
Animator.cpp
```

Relevant function:

```cpp
Animator::blendTransforms()
```

The blended Position / Rotation / Scale are then converted back into a local transformation matrix.

Relevant function:

```cpp
Animator::buildTransformMatrix()
```

The local matrix is built as:

```text
Translation
×
Rotation
×
Scale
```

---

# 9. Current Architecture

The current animation architecture is approximately:

```text
                Character GLB
                     │
          ┌──────────┴──────────┐
          │                     │
        Model              Animation Clips
          │                     │
     Mesh / Bones                ↓
     Bone Weights            Animation
                                │
                     Channels / Keyframes
                                │
                                ↓
                     AnimationController
                       │              │
                       │              │
                  Single State     Sequence
                                      │
                               Repeat Count
                                      │
                       └───────┬──────┘
                               ↓
                            Animator
                               │
                    ┌──────────┴──────────┐
                    │                     │
              Animation Time          Blending
                    │                     │
                    └──────────┬──────────┘
                               ↓
                     Bone Transform
                               ↓
                    Final Bone Matrices
                               ↓
                         Vertex Shader
                               ↓
                         GPU Skinning
                               ↓
                      Animated Character
```

---

# 10. File Responsibilities

## Model.h / Model.cpp

```text
Load GLB / GLTF model
Load Mesh
Load Material / Texture
Read Bones
Create Bone Mapping
Read Bone Weights
Store BoneInfo
```

Important concepts:

```text
aiScene
aiNode
aiMesh
aiBone
aiMaterial
```

---

## Mesh.h / Mesh.cpp

```text
VAO / VBO / EBO
Vertex Attributes
Texture Binding
Bone ID Upload
Bone Weight Upload
Mesh Rendering
```

---

## Animation.h / Animation.cpp

```text
Load one animation clip
Select animation by index
Read animation channels
Read Position / Rotation / Scale keyframes
Copy skeleton hierarchy
Store duration and ticks per second
```

---

## Animator.h / Animator.cpp

```text
Animation Time
Loop / Non-loop Playback
Finished Detection
Keyframe Sampling
Interpolation
Bone Hierarchy Calculation
Animation Blending
Final Bone Matrices
```

---

## AnimationController.h / AnimationController.cpp

```text
Semantic Animation States
Single-State Playback
Animation Sequence
Sequence Progression
Per-State Repeat Count
High-Level Animation Control
```

---

## Camera.h / Camera.cpp

```text
Camera Position
Camera Direction
View Matrix
Camera Movement
```

---

## Vertex Shader

```text
Receive Vertex Position
Receive Normal
Receive TexCoords
Receive Bone IDs
Receive Bone Weights
Receive finalBonesMatrices[]
Perform GPU Skinning
Calculate World Position
Calculate Final gl_Position
```

---

## Fragment Shader

```text
Sample Character Texture
Ambient Lighting
Diffuse Lighting
Specular Lighting
Final Pixel Color
```

---

## main.cpp

```text
Create Window
Initialize OpenGL
Create Shader
Load Character Model
Create AnimationController
Handle Input
Update AnimationController
Build Model / View / Projection Matrices
Upload Lighting Uniforms
Upload Final Bone Matrices
Draw Character
```

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
or a playable recording. New sentences still require model inference. Normal
runtime does not need to read the original reference WAV or write temporary WAV files.

## 11.1 Python Environment and Packages

The following is a recommended clean setup. Use the same environment for voice
preparation and the server. If an existing environment already runs the tested
model successfully, it can be reused; its name does not affect the model.

```bat
conda create -n qwen3-tts python=3.10 -y
conda activate qwen3-tts
python -m pip install --upgrade pip setuptools wheel
```

Install a CUDA-enabled PyTorch build suitable for the NVIDIA RTX 5060 and its
installed driver. Use the [official PyTorch installer](https://pytorch.org/get-started/locally/)
to select Windows, Pip, Python and a supported CUDA build. Install `torch` and
`torchaudio` together from the same CUDA wheel index with matching versions.
Do not infer the PyTorch package version from the CUDA version.

Then install the speech and HTTP packages:

```bat
python -m pip install -U qwen-tts flask soundfile numpy soxr
python -m pip check
```

| Package | Purpose |
| --- | --- |
| `torch` | GPU tensor operations and model inference |
| `torchaudio` | Audio operations used by the speech stack; match the PyTorch release |
| `qwen-tts` | Qwen3-TTS model loading, voice prompt extraction and speech generation |
| `flask` | Local HTTP endpoint called by C++ |
| `soundfile` | Write the preview WAV |
| `numpy` | Convert generated floating-point samples into PCM |
| `soxr` | Audio resampling dependency; prevents the missing-soxr import error |

The Qwen package installs its declared dependencies, including Transformers.
This setup uses PyTorch SDPA attention and does not require a separate
FlashAttention installation. The model weights are downloaded automatically on
first load; later runs normally reuse the local cache. The first download needs
network access.

Verify the active interpreter and imports:

```bat
where python
python -c "import soxr; from qwen_tts import Qwen3TTSModel, VoiceClonePromptItem; print('Import OK')"
python -c "import torch; print('Torch:', torch.__version__); print('CUDA:', torch.version.cuda); print('Available:', torch.cuda.is_available()); print('GPU:', torch.cuda.get_device_name(0) if torch.cuda.is_available() else 'Unavailable')"
```

Confirm an actual GPU operation also works:

```bat
python -c "import torch; x=torch.randn(256,256,device='cuda'); y=x@x; torch.cuda.synchronize(); print('CUDA computation OK')"
```

For a reproducible snapshot after a successful run:

```bat
python -m pip freeze > requirements-qwen.txt
```

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
| `requirements-qwen.txt` | Optional snapshot of the working Python packages |

## 11.3 Prepare the Reference Recording

Use a clean, single-speaker English recording of about 5–10 seconds as a starting
point. Avoid music, overlapping voices, clipping and long silences. The transcript
must match what was actually spoken, including any changes made while recording.

Example reference text:

> Hello, this is my natural voice. I'm speaking clearly and calmly, just as I would in a normal conversation.

In this project `x_vector_only_mode=False`: the prompt retains reference speech
information and its transcript, not just a speaker embedding. A longer reference
can increase the context processed during generation. Compare short references
for both similarity and latency rather than assuming longer is better.

## 11.4 Create character_voice.pt — test_qwen.py

Save this as `test_qwen.py` beside `reference.wav`. Change `ref_text` if your
recording says something different.
Run once, then listen to `output_clone.wav`:

```bat
conda activate qwen3-tts
cd /d "D:\project\virtual_agent\python\qwen_server"
python test_qwen.py
```

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

Request content type: `application/json`.

```json
{"text": "Hello! How are you today?"}
```

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

The C++ audio data is represented by:

```cpp
struct AudioBuffer
{
    std::vector<int16_t> samples;
    int sampleRate = 0;
    int channels = 0;
};
```

Responsibilities:

```text
samples
→ Raw signed 16-bit PCM samples.

sampleRate
→ Number of audio frames per second.

channels
→ 1 = mono
→ 2 = stereo
```

---

## 11.9 C++ TTSEngine

Main files:

```text
audio/TTSEngine.h
audio/TTSEngine.cpp
```

The C++ `TTSEngine` does not run the neural network directly.

Its responsibilities are:

```text
Receive std::string text
        ↓
Send HTTP POST request
        ↓
Receive PCM bytes
        ↓
Read sample rate / channel metadata
        ↓
Create AudioBuffer
```

Main interface:

```cpp
AudioBuffer synthesize(const std::string& text);
```

Conceptually:

```cpp
AudioBuffer audio =
    tts.synthesize(
        "Hello, this is my virtual character."
    );
```

The HTTP client is implemented using:

```text
cpp-httplib
```

The project uses the header-only file:

```text
external/httplib/httplib.h
```

---

## 11.10 cpp-httplib

`cpp-httplib` is used by the C++ TTS client to communicate with the local Python server.

Example installation layout:

```text
external/
└── httplib/
    └── httplib.h
```

Visual Studio include path:

```text
$(ProjectDir)external\httplib
```

Usage:

```cpp
#include "httplib.h"
```

The TTS client sends:

```text
POST http://127.0.0.1:5000/tts
```

and receives raw PCM data.

---

## 11.11 AudioPlayer

Main files:

```text
audio/AudioPlayer.h
audio/AudioPlayer.cpp
```

`AudioPlayer` uses:

```text
miniaudio
```

to play PCM audio directly from memory.

The internal implementation follows the PImpl pattern:

```text
AudioPlayer
    ↓
Impl*
    ↓
ma_engine
ma_audio_buffer
ma_sound
```

Responsibilities:

```text
ma_engine
→ Global miniaudio playback engine.

ma_audio_buffer
→ Wraps PCM samples stored in memory.

ma_sound
→ Playable sound object created from the PCM buffer.
```

The player keeps its own copy of the current PCM samples:

```cpp
std::vector<int16_t> currentSamples;
```

This is required because miniaudio playback is asynchronous.

Without an owned copy:

```text
AudioBuffer destroyed
        ↓
PCM memory released
        ↓
miniaudio still reads old pointer
        ↓
Access violation
```

With `currentSamples`:

```text
AudioPlayer::Impl
        ↓
owns PCM memory
        ↓
miniaudio can safely read it while playing
```

---

## 11.12 Speech Integration in main.cpp

The speech system is integrated into the main character application.

Main includes:

```cpp
#include "audio/TTSEngine.h"
#include "audio/AudioPlayer.h"
```

Create the speech components once:

```cpp
TTSEngine tts;
AudioPlayer audioPlayer;
```

During testing, a key can trigger speech:

```text
Press T
   ↓
Generate speech
   ↓
Play PCM
   ↓
Switch animation to Talk
```
The TTS call is currently synchronous.

This means:

```text
Render Loop
   ↓
TTS Request
   ↓
Wait for Python inference
   ↓
Receive PCM
   ↓
Continue Rendering
```

A future improvement is to move TTS inference to a background thread.

---

## 11.13 Speech System Runtime Flow

The complete current runtime pipeline is:

```text
User / C++ Trigger
        ↓
Text
        ↓
TTSEngine::synthesize()
        ↓
cpp-httplib
        ↓
HTTP POST /tts
        ↓
Python Flask Server
        ↓
Python Generates Speech PCM Using the Character's Voice
        ↓
HTTP Binary Response
        ↓
C++ AudioBuffer
        ↓
AudioPlayer
        ↓
ma_audio_buffer
        ↓
ma_sound
        ↓
miniaudio
        ↓
Speaker
```

Speech and character animation can be triggered together:

```text
Speech Trigger
    ├──→ TTS → Audio Playback
    │
    └──→ AnimationController → Talk Animation
```

---

## 11.14 Speech-Related File Responsibilities

### Python

```text
python/qwen_server/server.py
```

Responsibilities:

```text
Flask HTTP server
Load Qwen3-TTS model
Receive text
Generate speech
Convert float32 PCM to int16 PCM
Return raw audio bytes
Return sample-rate/channel metadata
```

### C++

```text
audio/TTSEngine.h
audio/TTSEngine.cpp
```

Responsibilities:

```text
Send text to Python server
Receive binary PCM response
Parse audio metadata
Build AudioBuffer
```

```text
audio/AudioBuffer.h
```

Responsibilities:

```text
Store PCM samples
Store sample rate
Store channel count
```

```text
audio/AudioPlayer.h
audio/AudioPlayer.cpp
```

Responsibilities:

```text
Initialize miniaudio
Own PCM memory during playback
Create ma_audio_buffer
Create ma_sound
Start audio playback
```

```text
main.cpp
```

Responsibilities:

```text
Create TTSEngine
Create AudioPlayer
Trigger speech
Trigger Talk animation
Continue rendering the character
```

---

## 11.15 Running the Speech System

Start the Python environment:

```bat
conda activate qwen3-tts
```

Start the TTS server:

```bat
cd /d D:\project\virtual_agent\python\qwen_server
python server.py
```

Keep this terminal running.

Then start the C++ Virtual Agent from Visual Studio.

Runtime:

```text
Terminal 1
→ Python TTS Server

Visual Studio
→ C++ Virtual Agent
```

The C++ process sends text to the local Python server through:

```text
127.0.0.1:5000
```

No external network connection is required for the C++ ↔ Python communication.

---


# 12. Current Progress

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
- Integrated speech triggering with the character `Talk` animation.

---

# 13. Next Learning Steps

The basic body-animation framework is now functional.

The next major topics focus on facial animation and higher-level character behavior.

## Blendshape / Morph Target

Skeletal animation mainly controls the body.

Blendshapes are commonly used for facial animation.

Examples:

```text
Neutral Face
     ↓
Smile
Blink
Mouth Open
Eyebrow Raise
```

Possible future implementation files:

```text
Model.cpp
Mesh.cpp
Vertex Shader
FacialAnimationController.cpp
```

---

## Facial Animation

Future controls may include:

```text
Eyes
Eyebrows
Mouth
Facial Expressions
```

using blendshapes / morph targets.

---

## Lip Synchronization

Future pipeline:

```text
Speech
  ↓
Phoneme / Viseme
  ↓
Blendshape
  ↓
Mouth Movement
```

Possible future components:

```text
Speech System
LipSync Controller
Facial Animation Controller
```

---

## Character Behavior

Higher-level logic can eventually generate action sequences.

Example:

```text
AI Decision
    ↓
Animation Sequence
    ↓
Walk × 3
    ↓
Talk × 1
    ↓
Idle
```

The future AI layer should request semantic actions such as:

```text
Walk
Talk
Sit
Idle
```

instead of directly manipulating:

```text
Animation Index
Animation Time
Bone Matrices
```

---

# 14. Long-Term Direction

The project can gradually evolve into:

```text
3D Character
     ↓
Animation System
     ↓
Animation Controller
     ↓
Facial Animation
     ↓
Speech / Lip Sync
     ↓
Perception
     ↓
AI / LLM
     ↓
Interactive Virtual Agent
```

The current focus remains on understanding and implementing the **3D character and animation system at the C++ implementation level**.
