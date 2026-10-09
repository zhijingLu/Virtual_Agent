#include "Animation.h"

#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>


// =====================================================
// Constructor
// =====================================================

Animation::Animation(
    const std::string& animationPath,
    Model* model,
    unsigned int animationIndex
)
{
    readAnimation(
        animationPath,
        model,
        animationIndex
    );
}


// =====================================================
// Read animation from model file
// =====================================================

void Animation::readAnimation(
    const std::string& animationPath,
    Model* model,
    unsigned int animationIndex
)
{
    Assimp::Importer importer;


    // Load the model again using Assimp.
    //
    // For now, Animation loads the same model file
    // independently from Model.
    const aiScene* scene =
        importer.ReadFile(
            animationPath,
            aiProcess_Triangulate
        );


    // Check whether the file loaded successfully.
    if (
        !scene ||
        !scene->mRootNode
        )
    {
        std::cerr
            << "Failed to load animation file: "
            << importer.GetErrorString()
            << std::endl;

        return;
    }

    std::cout
        << "Number of animations: "
        << scene->mNumAnimations
        << std::endl;

    for (
        unsigned int i = 0;
        i < scene->mNumAnimations;
        ++i
        )
    {
        std::cout
            << "Animation "
            << i
            << ": "
            << scene->mAnimations[i]->mName.C_Str()
            << std::endl;
    }
    // Check whether this model actually contains
    // at least one animation.
    if (scene->mNumAnimations == 0)
    {
        std::cerr
            << "No animation found in model."
            << std::endl;

        return;
    }

    if (animationIndex >= scene->mNumAnimations)
    {
        std::cerr
            << "Invalid animation index."
            << std::endl;

        return;
    }

    aiAnimation* animation =scene->mAnimations[ animationIndex ];
    


    // Store total animation duration.
    duration =
        static_cast<float>(
            animation->mDuration
            );


    // Store animation speed.
    //
    // Some files may store 0 here,
    // so we use 25 as a fallback.
    if (animation->mTicksPerSecond != 0.0)
    {
        ticksPerSecond =
            static_cast<float>(
                animation->mTicksPerSecond
                );
    }
    else
    {
        ticksPerSecond = 25.0f;
    }


    // Reserve memory for animation channels.
    boneAnimations.reserve(
        animation->mNumChannels
    );


    // -------------------------------------------------
    // Read every animation channel.
    // -------------------------------------------------

    for (
        unsigned int channelIndex = 0;
        channelIndex < animation->mNumChannels;
        ++channelIndex
        )
    {
        // One aiNodeAnim usually represents
        // animation data for one skeleton node.
        aiNodeAnim* channel =
            animation->mChannels[
                channelIndex
            ];


        BoneAnimation boneAnimation;


        // Store bone/node name.
        boneAnimation.boneName =
            channel->mNodeName.C_Str();


        // =============================================
        // Position keyframes
        // =============================================

        boneAnimation.positions.reserve(
            channel->mNumPositionKeys
        );


        for (
            unsigned int i = 0;
            i < channel->mNumPositionKeys;
            ++i
            )
        {
            PositionKey key;


            // Convert Assimp position to GLM.
            key.position =
                convertVector(
                    channel
                    ->mPositionKeys[i]
                    .mValue
                );


            // Store keyframe time.
            key.timeStamp =
                static_cast<float>(
                    channel
                    ->mPositionKeys[i]
                    .mTime
                    );


            boneAnimation.positions.push_back(
                key
            );
        }


        // =============================================
        // Rotation keyframes
        // =============================================

        boneAnimation.rotations.reserve(
            channel->mNumRotationKeys
        );


        for (
            unsigned int i = 0;
            i < channel->mNumRotationKeys;
            ++i
            )
        {
            RotationKey key;


            // Convert Assimp quaternion to GLM.
            key.orientation =
                convertQuaternion(
                    channel
                    ->mRotationKeys[i]
                    .mValue
                );


            // Store keyframe time.
            key.timeStamp =
                static_cast<float>(
                    channel
                    ->mRotationKeys[i]
                    .mTime
                    );


            boneAnimation.rotations.push_back(
                key
            );
        }


        // =============================================
        // Scale keyframes
        // =============================================

        boneAnimation.scales.reserve(
            channel->mNumScalingKeys
        );


        for (
            unsigned int i = 0;
            i < channel->mNumScalingKeys;
            ++i
            )
        {
            ScaleKey key;


            // Convert Assimp scale to GLM.
            key.scale =
                convertVector(
                    channel
                    ->mScalingKeys[i]
                    .mValue
                );


            // Store keyframe time.
            key.timeStamp =
                static_cast<float>(
                    channel
                    ->mScalingKeys[i]
                    .mTime
                    );


            boneAnimation.scales.push_back(
                key
            );
        }


        // Store this bone's animation channel.
        boneAnimations.push_back(
            boneAnimation
        );
    }

    // Copy skeleton / scene hierarchy.
    readHierarchyData(
        rootNode,
        scene->mRootNode
    );


    // Store inverse root transformation.
    //
    // Later:
    // finalBoneMatrix =globalInverse *globalBoneTransform *boneOffset
    globalInverseTransform =
        glm::inverse(
            convertMatrix(
                scene->mRootNode->mTransformation
            )
        );


    // Copy bone information from Model.
    if (model)
    {
        boneMapping =
            model->getBoneMapping();

        boneInfo =
            model->getBoneInfo();
    }

    std::cout
        << "Animation duration: "
        << duration
        << std::endl;


    std::cout
        << "Ticks per second: "
        << ticksPerSecond
        << std::endl;


    std::cout
        << "Animation channels: "
        << boneAnimations.size()
        << std::endl;
}


// =====================================================
// Convert aiVector3D to glm::vec3
// =====================================================

glm::vec3 Animation::convertVector(
    const aiVector3D& vector
)
{
    return glm::vec3(
        vector.x,
        vector.y,
        vector.z
    );
}


// =====================================================
// Convert aiQuaternion to glm::quat
// =====================================================

glm::quat Animation::convertQuaternion(
    const aiQuaternion& quaternion
)
{
    // glm::quat constructor order:
    //
    // w, x, y, z
    return glm::quat(
        quaternion.w,
        quaternion.x,
        quaternion.y,
        quaternion.z
    );
}


// =====================================================
// Get animation duration
// =====================================================

float Animation::getDuration() const
{
    return duration;
}


// =====================================================
// Get ticks per second
// =====================================================

float Animation::getTicksPerSecond() const
{
    return ticksPerSecond;
}


// =====================================================
// Get all bone animation channels
// =====================================================

const std::vector<BoneAnimation>&
Animation::getBoneAnimations() const
{
    return boneAnimations;
}


// =====================================================
// Read Assimp node hierarchy
// =====================================================

void Animation::readHierarchyData(
    AssimpNodeData& destination,
    const aiNode* source
)
{
    if (!source)
    {
        return;
    }


    // Copy node name.
    destination.name =
        source->mName.C_Str();


    // Copy local node transformation.
    destination.transformation =
        convertMatrix(
            source->mTransformation
        );


    // Prepare space for child nodes.
    destination.children.resize(
        source->mNumChildren
    );


    // Recursively copy every child.
    for (
        unsigned int i = 0;
        i < source->mNumChildren;
        ++i
        )
    {
        readHierarchyData(
            destination.children[i],
            source->mChildren[i]
        );
    }
}


// =====================================================
// Convert Assimp matrix to GLM
// =====================================================

glm::mat4 Animation::convertMatrix(
    const aiMatrix4x4& from
)
{
    glm::mat4 to;


    // GLM indexing is [column][row].
    to[0][0] = from.a1;
    to[1][0] = from.a2;
    to[2][0] = from.a3;
    to[3][0] = from.a4;

    to[0][1] = from.b1;
    to[1][1] = from.b2;
    to[2][1] = from.b3;
    to[3][1] = from.b4;

    to[0][2] = from.c1;
    to[1][2] = from.c2;
    to[2][2] = from.c3;
    to[3][2] = from.c4;

    to[0][3] = from.d1;
    to[1][3] = from.d2;
    to[2][3] = from.d3;
    to[3][3] = from.d4;


    return to;
}

const BoneAnimation*
Animation::findBoneAnimation(
    const std::string& name
) const
{
    for (
        const BoneAnimation& animation :
        boneAnimations
        )
    {
        if (animation.boneName == name)
        {
            return &animation;
        }
    }

    return nullptr;
}


const AssimpNodeData&
Animation::getRootNode() const
{
    return rootNode;
}


const std::map<std::string, int>&
Animation::getBoneMapping() const
{
    return boneMapping;
}


const std::vector<BoneInfo>&
Animation::getBoneInfo() const
{
    return boneInfo;
}


const glm::mat4&
Animation::getGlobalInverseTransform() const
{
    return globalInverseTransform;
}