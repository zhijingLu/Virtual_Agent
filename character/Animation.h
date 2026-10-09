#pragma once

#include <string>
#include <vector>
#include <map>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <assimp/scene.h>

#include "Model.h"

//read keyframe animation
// -----------------------------------------------------
// A position keyframe.
// -----------------------------------------------------

struct PositionKey
{
    // Bone/node position at this keyframe.
    glm::vec3 position;

    // Time of this keyframe.
    float timeStamp;
};


// -----------------------------------------------------
// A rotation keyframe.
// -----------------------------------------------------

struct RotationKey
{
    // Bone/node rotation at this keyframe.
    glm::quat orientation;

    // Time of this keyframe.
    float timeStamp;
};


// -----------------------------------------------------
// A scale keyframe.
// -----------------------------------------------------

struct ScaleKey
{
    // Bone/node scale at this keyframe.
    glm::vec3 scale;

    // Time of this keyframe.
    float timeStamp;
};


// -----------------------------------------------------
// Animation data for one bone/node.
// -----------------------------------------------------

struct BoneAnimation
{
    // Name of the animated node/bone.
    std::string boneName;

    // Position keyframes.
    std::vector<PositionKey> positions;

    // Rotation keyframes.
    std::vector<RotationKey> rotations;

    // Scale keyframes.
    std::vector<ScaleKey> scales;
};
// -----------------------------------------------------
// One node in the imported skeleton / scene hierarchy.
// -----------------------------------------------------

struct AssimpNodeData
{
    // Local transformation of this node
    // relative to its parent.
    glm::mat4 transformation =
        glm::mat4(1.0f);

    // Node / bone name.
    std::string name;

    // Child nodes.
    std::vector<AssimpNodeData> children;
};

class Animation
{
public:

    // Load animation data from a model file.
    Animation(
        const std::string& animationPath,
        Model* model,
        unsigned int animationIndex
    );


    // Duration of the animation in Assimp ticks.
    float getDuration() const;


    // Number of animation ticks per second.
    float getTicksPerSecond() const;


    // Get all bone animation channels.
    const std::vector<BoneAnimation>&
        getBoneAnimations() const;

public:

    // Find animation keyframes belonging to a node/bone.
    const BoneAnimation* findBoneAnimation(
        const std::string& name
    ) const;


    // Return the root of the skeleton hierarchy.
    const AssimpNodeData&
        getRootNode() const;


    // Return name -> bone ID mapping.
    const std::map<std::string, int>&
        getBoneMapping() const;


    // Return bone information.
    const std::vector<BoneInfo>&
        getBoneInfo() const;


    // Transformation used to remove the original
    // root-node transform when calculating skinning.
    const glm::mat4&
        getGlobalInverseTransform() const;


private:

    // Total duration of the animation.
    float duration = 0.0f;

    // Conversion rate from real seconds
    // to animation ticks.
    float ticksPerSecond = 0.0f;


    // Animation channels.
    //
    // Each channel usually corresponds
    // to one skeleton node/bone.
    std::vector<BoneAnimation> boneAnimations;


    // Read animation data from Assimp.
    void readAnimation(
        const std::string& animationPath,
        Model* model,
        unsigned int animationIndex
    );


    // Convert Assimp vector to GLM vector.
    glm::vec3 convertVector(
        const aiVector3D& vector
    );


    // Convert Assimp quaternion to GLM quaternion.
    glm::quat convertQuaternion(
        const aiQuaternion& quaternion
    );

    // Root node of the imported scene hierarchy.
    AssimpNodeData rootNode;


    // Bone information copied from Model.
    std::map<std::string, int> boneMapping;

    std::vector<BoneInfo> boneInfo;


    // Inverse of the imported scene's root transform.
    glm::mat4 globalInverseTransform =glm::mat4(1.0f);


    // Recursively copy the Assimp node hierarchy.
    void readHierarchyData(AssimpNodeData& destination,const aiNode* source);


    // Convert Assimp matrix to GLM.
    glm::mat4 convertMatrix(const aiMatrix4x4& matrix);
};