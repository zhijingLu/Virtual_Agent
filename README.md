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

# 11. Current Progress

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

---

# 12. Next Learning Steps

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

# 13. Long-Term Direction

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
