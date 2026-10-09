#include "Animator.h"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>



Animator::Animator(
    Animation* animation
)
{
    currentAnimation =
        animation;


    // Prepare bone matrix array.
    finalBoneMatrices.resize(
        MAX_BONES,
        glm::mat4(1.0f)
    );
}

void Animator::updateAnimation(
    float deltaTime
)
{
    if (!currentAnimation)
    {
        return;
    }


    // Convert real-world seconds into animation ticks.
    currentTime +=
        currentAnimation->getTicksPerSecond()
        *
        deltaTime;


    float duration =
        currentAnimation->getDuration();


    // Loop the animation.
    if (duration > 0.0f)
    {
        currentTime =
            std::fmod(
                currentTime,
                duration
            );
    }


    // Start recursively calculating bone transforms
    // from the root node.
    calculateBoneTransform(
        currentAnimation->getRootNode(),
        glm::mat4(1.0f)
    );
}

void Animator::calculateBoneTransform(
    const AssimpNodeData& node,
    const glm::mat4& parentTransform
)
{
    std::string nodeName =
        node.name;


    // Start with the node's original
    // transformation from the model.
    glm::mat4 nodeTransform =
        node.transformation;


    // Check whether this node has animation data.
    const BoneAnimation* boneAnimation =
        currentAnimation
        ->findBoneAnimation(
            nodeName
        );


    if (boneAnimation)
    {
        // Replace the original local transform
        // with the interpolated animation transform.
        nodeTransform =
            calculateLocalTransform(
                *boneAnimation
            );
    }


    // Child transform must include
    // all transformations from its parents.
    glm::mat4 globalTransform =
        parentTransform *
        nodeTransform;


    const auto& boneMapping =
        currentAnimation
        ->getBoneMapping();


    // Check whether this node is an actual skinning bone.
    auto iterator =
        boneMapping.find(
            nodeName
        );


    if (
        iterator !=
        boneMapping.end()
        )
    {
        int boneID =
            iterator->second;


        const auto& boneInfo =
            currentAnimation
            ->getBoneInfo();


        if (
            boneID >= 0 &&
            boneID <
            static_cast<int>(
                finalBoneMatrices.size()
                ) &&
            boneID <
            static_cast<int>(
                boneInfo.size()
                )
            )
        {
            // Final skinning matrix:
            //
            // remove root transform
            // *
            // current animated global transform
            // *
            // bind-pose offset matrix
            finalBoneMatrices[boneID] =
                currentAnimation
                ->getGlobalInverseTransform()
                *
                globalTransform
                *
                boneInfo[boneID].offset;
        }
    }


    // Recursively calculate all children.
    for (
        const AssimpNodeData& child :
        node.children
        )
    {
        calculateBoneTransform(
            child,
            globalTransform
        );
    }
}

glm::mat4 Animator::calculateLocalTransform(
    const BoneAnimation& animation
)
{
    // Current interpolated translation.
    glm::vec3 position =
        interpolatePosition(
            animation
        );


    // Current interpolated rotation.
    glm::quat rotation =
        interpolateRotation(
            animation
        );


    // Current interpolated scale.
    glm::vec3 scale =
        interpolateScale(
            animation
        );


    glm::mat4 translationMatrix =
        glm::translate(
            glm::mat4(1.0f),
            position
        );


    glm::mat4 rotationMatrix =
        glm::mat4_cast(
            rotation
        );


    glm::mat4 scaleMatrix =
        glm::scale(
            glm::mat4(1.0f),
            scale
        );


    // Local animation transformation.
    return
        translationMatrix *
        rotationMatrix *
        scaleMatrix;
}

glm::vec3 Animator::interpolatePosition(
    const BoneAnimation& animation
)
{
    // Only one keyframe:
    // no interpolation is needed.
    if (animation.positions.size() == 1)
    {
        return
            animation.positions[0].position;
    }


    if (animation.positions.empty())
    {
        return glm::vec3(0.0f);
    }


    int index =
        findPositionIndex(
            animation
        );


    int nextIndex =
        index + 1;


    float factor =
        calculateFactor(
            animation.positions[index]
            .timeStamp,

            animation.positions[nextIndex]
            .timeStamp
        );


    // Linear interpolation.
    return glm::mix(
        animation.positions[index]
        .position,

        animation.positions[nextIndex]
        .position,

        factor
    );
}

glm::quat Animator::interpolateRotation(
    const BoneAnimation& animation
)
{
    if (animation.rotations.size() == 1)
    {
        return glm::normalize(
            animation.rotations[0]
            .orientation
        );
    }


    if (animation.rotations.empty())
    {
        return glm::quat(
            1.0f,
            0.0f,
            0.0f,
            0.0f
        );
    }


    int index =
        findRotationIndex(
            animation
        );


    int nextIndex =
        index + 1;


    float factor =
        calculateFactor(
            animation.rotations[index]
            .timeStamp,

            animation.rotations[nextIndex]
            .timeStamp
        );


    // Spherical interpolation is used
    // for quaternion rotations.
    glm::quat rotation =
        glm::slerp(
            animation.rotations[index]
            .orientation,

            animation.rotations[nextIndex]
            .orientation,

            factor
        );


    return glm::normalize(
        rotation
    );
}

glm::vec3 Animator::interpolateScale(
    const BoneAnimation& animation
)
{
    if (animation.scales.size() == 1)
    {
        return
            animation.scales[0].scale;
    }


    if (animation.scales.empty())
    {
        return glm::vec3(1.0f);
    }


    int index =
        findScaleIndex(
            animation
        );


    int nextIndex =
        index + 1;


    float factor =
        calculateFactor(
            animation.scales[index]
            .timeStamp,

            animation.scales[nextIndex]
            .timeStamp
        );


    return glm::mix(
        animation.scales[index]
        .scale,

        animation.scales[nextIndex]
        .scale,

        factor
    );
}

int Animator::findPositionIndex(
    const BoneAnimation& animation
) const
{
    for (
        int i = 0;
        i <
        static_cast<int>(
            animation.positions.size()
            ) - 1;
            ++i
        )
    {
        if (
            currentTime <
            animation.positions[i + 1]
            .timeStamp
            )
        {
            return i;
        }
    }


    return
        static_cast<int>(
            animation.positions.size()
            ) - 2;
}

int Animator::findRotationIndex(
    const BoneAnimation& animation
) const
{
    for (
        int i = 0;
        i <
        static_cast<int>(
            animation.rotations.size()
            ) - 1;
            ++i
        )
    {
        if (
            currentTime <
            animation.rotations[i + 1]
            .timeStamp
            )
        {
            return i;
        }
    }


    return
        static_cast<int>(
            animation.rotations.size()
            ) - 2;
}

int Animator::findScaleIndex(
    const BoneAnimation& animation
) const
{
    for (
        int i = 0;
        i <
        static_cast<int>(
            animation.scales.size()
            ) - 1;
            ++i
        )
    {
        if (
            currentTime <
            animation.scales[i + 1]
            .timeStamp
            )
        {
            return i;
        }
    }


    return
        static_cast<int>(
            animation.scales.size()
            ) - 2;
}

float Animator::calculateFactor(
    float lastTime,
    float nextTime
) const
{
    float delta =
        nextTime -
        lastTime;


    if (delta <= 0.0f)
    {
        return 0.0f;
    }


    return
        (currentTime - lastTime)
        /
        delta;
}

void Animator::playAnimation(
    Animation* animation
)
{
    currentAnimation =
        animation;

    currentTime = 0.0f;
}


const std::vector<glm::mat4>&
Animator::getFinalBoneMatrices() const
{
    return finalBoneMatrices;
}