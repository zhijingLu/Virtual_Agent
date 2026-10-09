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

---

## Dependency Management

This project uses **vcpkg Manifest Mode**.

The repository already contains:

```text
vcpkg.json
vcpkg-configuration.json
```

The required libraries are defined inside `vcpkg.json`.

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
GLFW    → Window and OpenGL context
GLM     → Vector / matrix mathematics
Assimp  → 3D model, skeleton and animation loading
```

---

## Install Dependencies

Clone the repository:

```powershell
git clone https://github.com/zhijingLu/Virtual_Agent.git
```

Enter the project directory:

```powershell
cd Virtual_Agent
```

Install the dependencies:

```powershell
vcpkg install --triplet x64-windows-static
```

The project uses:

```text
x64-windows-static
```

which means the vcpkg libraries are linked statically.

After installation, dependencies are stored under:

```text
vcpkg_installed/
```

---

## Visual Studio

Open:

```text
virtual_agent.slnx
```

Use:

```text
Debug | x64
```

or:

```text
Release | x64
```

The project already enables vcpkg Manifest Mode and uses the static vcpkg libraries.

Then build with:

```text
Ctrl + Shift + B
```

and run with:

```text
Local Windows Debugger
```

---

# 2. Learning Progress

The current project follows the basic 3D character pipeline:

```text
3D Model
   ↓
Mesh
   ↓
Material / Texture
   ↓
Skeleton
   ↓
Bones
   ↓
Bone Weights
   ↓
Skinning
   ↓
Animation Clip
   ↓
Animation Channels
   ↓
Keyframe Interpolation
   ↓
GPU Skinning
   ↓
Animated Character
```

---

## Currently Learned

### 1. Mesh

A 3D model is composed of meshes.

A mesh mainly contains:

```text
Vertices
Indices
Normals
Texture Coordinates
```

The mesh data is uploaded to the GPU using:

```text
VAO
VBO
EBO
```

---

### 2. Material and Texture

Materials describe how the surface of the model looks.

Textures provide image information such as:

```text
Diffuse / Base Color
Normal
Metallic
Roughness
```

---

### 3. Basic Lighting

The current renderer includes basic lighting concepts:

```text
Ambient
Diffuse
Specular
```

Lighting mainly depends on:

```text
Light Position
Surface Normal
Camera Position
Fragment Position
```

---

### 4. Skeleton

An animated character contains a hierarchy of bones.

For example:

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

The parent-child hierarchy determines how bone transformations propagate through the character.

---

### 5. Bone Weights

A vertex can be influenced by multiple bones.

For example:

```text
Vertex
 ├── Bone A → 0.7
 ├── Bone B → 0.2
 └── Bone C → 0.1
```

The weights determine how strongly each bone affects the vertex.

---

### 6. Skinning

Skinning connects the mesh with the skeleton.

When the skeleton moves:

```text
Bone Transform
      ↓
Vertex Transformation
      ↓
Mesh Deformation
```

This allows the character body to move together with the skeleton.

---

### 7. Animation Clip

A model can contain multiple animations, for example:

```text
Idle
Walk
Run
Wave
Talk
```

Assimp loads these animations from:

```cpp
scene->mAnimations
```

A specific animation can be selected by its index.

---

### 8. Animation Channel

An animation contains multiple channels.

Usually one animation channel corresponds to one bone.

Each channel contains keyframes for:

```text
Position Keyframes
Rotation Keyframes
Scale Keyframes
```

---

### 9. Keyframe Interpolation

The current animation time normally lies between two keyframes.

Therefore the transformation has to be interpolated.

Position and scale use linear interpolation:

```text
Linear Interpolation
```

Rotation uses:

```text
Quaternion SLERP
```

This produces smooth animation between keyframes.

---

### 10. GPU Skinning

The CPU calculates the current bone transformations.

The final bone matrices are sent to the vertex shader:

```text
CPU
 ↓
Bone Matrices
 ↓
Vertex Shader
 ↓
GPU Skinning
 ↓
Animated Vertices
```

The GPU then calculates the final animated position of every vertex.

---

# 3. Current Animation Framework

The current code structure is approximately:

```text
Model
 │
 ├── Mesh
 │
 ├── Bone Information
 │
 └── Animation
        │
        ├── Animation Channels
        │
        └── Keyframes

Animator
 │
 ├── Current Animation
 ├── Current Time
 └── Final Bone Matrices
        │
        ↓
      Shader
        │
        ↓
   GPU Skinning
```

Main responsibility of each component:

```text
Model
→ Loads the 3D model and mesh data.

Mesh
→ Stores and renders vertex data.

Animation
→ Loads one animation clip and its bone channels.

Animator
→ Updates animation time and calculates bone matrices.

Shader
→ Performs GPU skinning and rendering.
```

---

# 4. Next Learning Steps

The next topics will extend the current skeletal animation system.

## Animation Switching

Learn how to switch between different animation clips.

Example:

```text
Idle
  ↓
Walk
  ↓
Run
```

---

## Animation Blending

Instead of instantly switching:

```text
Idle → Walk
```

blend the two animations gradually:

```text
Idle
  ↓
Idle + Walk
  ↓
Walk
```

This makes character movement smoother.

---

## Animation State Machine

Control animation based on character state.

For example:

```text
          Walk
         ↗    ↘
      Idle    Run
         ↘    ↗
          Jump
```

---

## Blendshape / Morph Target

Skeletal animation mainly controls the body.

Blendshapes are commonly used for facial animation.

For example:

```text
Neutral Face
     ↓
Smile
Blink
Mouth Open
Eyebrow Raise
```

---

## Facial Animation

Learn how to control:

```text
Eyes
Eyebrows
Mouth
Facial Expressions
```

using blendshapes.

---

## Lip Synchronization

Connect speech with mouth animation.

Pipeline:

```text
Speech
  ↓
Phoneme / Viseme
  ↓
Blendshape
  ↓
Mouth Movement
```

---

# 5. Long-Term Direction

After the graphics and animation foundation is complete, the project can gradually evolve into:

```text
3D Character
     ↓
Animation System
     ↓
Facial Animation
     ↓
Speech
     ↓
Perception
     ↓
AI / LLM
     ↓
Interactive Virtual Agent
```

The current focus is still on understanding the **3D character and animation system from the C++ implementation level**.
