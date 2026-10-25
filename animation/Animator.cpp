#include "Animator.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>


// =====================================================
// Constructor
// =====================================================

Animator::Animator(
    Animation* animation
)
{
    currentAnimation =animation;
    previousAnimation =nullptr;
    currentTime = 0.0f;
    previousTime =  0.0f;
    loop =true;
    finished = false;
    //Animation Blending
    blending = false;
    blendTime = 0.0f;
    blendDuration = 0.5f;

    // Allocate fixed-size matrix array
    // for GPU skinning.
    //
    // All bones initially use identity matrices.
    //
    finalBoneMatrices.resize(
        MAX_BONES,
        glm::mat4(1.0f)
    );
}


// =====================================================
// Update animation
// =====================================================

void Animator::updateAnimation(
    float deltaTime
)
{
    // No animation is currently loaded.
    if (!currentAnimation)
    {
        return;
    }


    // -------------------------------------------------
    // Update current animation time
    // -------------------------------------------------

    // If a non-looping animation has already finished,
    // keep its final pose.
    //
    // The AnimationController can check:
    //
    // animator.isFinished()
    //
    // and then switch to the next action.
    //
    if (!finished)
    {
        currentTime +=
            currentAnimation
            ->getTicksPerSecond()
            *
            deltaTime;


        float duration =
            currentAnimation
            ->getDuration();


        if (duration > 0.0f)
        {
            // -----------------------------------------
            // Looping animation
            // -----------------------------------------

            if (loop)
            {
                currentTime =
                    std::fmod(
                        currentTime,
                        duration
                    );
            }
            // -----------------------------------------
            // Non-looping animation
            // -----------------------------------------
            else
            {
                if (currentTime >= duration)
                {
                    // Clamp to the final animation time.
                    currentTime =duration;

                    finished =true;
                }
            }
        }
    }

    // -------------------------------------------------
    // Update blending timer
    // -------------------------------------------------
    if (blending)
    {
        blendTime +=deltaTime;
    }

    // -------------------------------------------------
    // Calculate current skeleton pose
    // -------------------------------------------------

    calculateBoneTransform(
        currentAnimation
        ->getRootNode(),
        glm::mat4(1.0f)
    );


    // -------------------------------------------------
    // End blending
    // -------------------------------------------------

    if (
        blending
        &&
        blendTime >= blendDuration
        )
    {
        blending = false;

        previousAnimation =nullptr;

        previousTime =0.0f;
    }
}


// =====================================================
// Change animation
// =====================================================

void Animator::playAnimation(
    Animation* animation,
    float newBlendDuration
)
{
    if (!animation)
    {
        return;
    }


    // -------------------------------------------------
    // Save current animation as previous animation
    // -------------------------------------------------
    //
    // The current pose will be used as the starting
    // point of the transition.
    //

    if (currentAnimation && newBlendDuration > 0.0f)
    {
        previousAnimation =currentAnimation;

        previousTime = currentTime;
        blending = true;
        blendTime = 0.0f;
        blendDuration = newBlendDuration;
    }

    else
    {
        // No blending requested.
        previousAnimation =  nullptr;

        blending =false;

        blendTime = 0.0f;
    }


    // -------------------------------------------------
    // Start new animation
    // -------------------------------------------------

    currentAnimation = animation;


    // New animation always starts at the beginning.
    currentTime = 0.0f;


    // New animation is not finished.
    finished = false;
}


// =====================================================
// Loop control
// =====================================================

void Animator::setLoop(
    bool shouldLoop
)
{
    loop = shouldLoop;
    // If we enable looping again,
    // Animator must continue updating.
    if (loop)
    {
        finished = false;
    }
}


// =====================================================
// Finished state
// =====================================================

bool Animator::isFinished() const
{
    return finished;
}


// =====================================================
// Final bone matrices
// =====================================================

const std::vector<glm::mat4>&
Animator::getFinalBoneMatrices() const
{
    return finalBoneMatrices;
}


// =====================================================
// Calculate bone hierarchy
// =====================================================

void Animator::calculateBoneTransform(
    const AssimpNodeData& node,
    const glm::mat4& parentTransform
)
{
    if (!currentAnimation)
    {
        return;
    }


    const std::string& nodeName = node.name;


    // Default transformation stored in the model.
    //
    // This is used when this node does not have
    // an animation channel.
    //
    glm::mat4 nodeTransform = node.transformation;


    // =================================================
    // Current animation channel
    // =================================================

    const BoneAnimation* currentBoneAnimation =
        currentAnimation
        ->findBoneAnimation( nodeName);


    // =================================================
    // Blending
    // =================================================

    if (blending && previousAnimation)
    {
        // Find the same bone/node
        // in the previous animation.
        const BoneAnimation* previousBoneAnimation =
            previousAnimation
            ->findBoneAnimation(nodeName );


        // ---------------------------------------------
        // Both animations contain this bone
        // ---------------------------------------------

        if (
            previousBoneAnimation
            &&
            currentBoneAnimation
            )
        {
            // Sample previous pose.
            LocalBoneTransform previousTransform =
                sampleBoneTransform(
                    *previousBoneAnimation,
                    previousTime
                );


            // Sample new pose.
            LocalBoneTransform currentTransform =
                sampleBoneTransform(
                    *currentBoneAnimation,
                    currentTime
                );


            // Calculate blending factor.
            //
            // Example:
            //
            // blendTime     = 0.1
            // blendDuration = 0.2
            //
            // factor = 0.5
            //
            float factor =
                1.0f;


            if (blendDuration > 0.0f)
            {
                factor =
                    blendTime
                    /
                    blendDuration;
            }


            factor =
                std::clamp(
                    factor,
                    0.0f,
                    1.0f
                );


            // Blend Position / Rotation / Scale.
            LocalBoneTransform blendedTransform =
                blendTransforms(
                    previousTransform,
                    currentTransform,
                    factor
                );


            // Convert blended TRS back into matrix.
            nodeTransform =
                buildTransformMatrix(
                    blendedTransform
                );
        }


        // ---------------------------------------------
        // Only current animation contains this bone
        // ---------------------------------------------

        else if (currentBoneAnimation)
        {
            LocalBoneTransform currentTransform =
                sampleBoneTransform(
                    *currentBoneAnimation,
                    currentTime
                );


            nodeTransform =
                buildTransformMatrix(
                    currentTransform
                );
        }


        // ---------------------------------------------
        // Only previous animation contains this bone
        // ---------------------------------------------
        //
        // Keep previous pose during the transition.
        //
        else if (previousBoneAnimation)
        {
            LocalBoneTransform previousTransform =
                sampleBoneTransform(
                    *previousBoneAnimation,
                    previousTime
                );


            nodeTransform =
                buildTransformMatrix(
                    previousTransform
                );
        }
    }


    // =================================================
    // Normal playback without blending
    // =================================================

    else if (currentBoneAnimation)
    {
        LocalBoneTransform currentTransform =
            sampleBoneTransform(
                *currentBoneAnimation,
                currentTime
            );


        nodeTransform =
            buildTransformMatrix(
                currentTransform
            );
    }


    // =================================================
    // Local -> Global
    // =================================================

    // Parent global transform
    // *
    // current local transform
    //
    // gives this node's global transform.
    //
    glm::mat4 globalTransform =
        parentTransform
        *
        nodeTransform;


    // =================================================
    // Skinning bone
    // =================================================

    const auto& boneMapping =
        currentAnimation
        ->getBoneMapping();


    const auto& boneInfo =
        currentAnimation
        ->getBoneInfo();


    auto boneIterator =
        boneMapping.find(
            nodeName
        );


    if (
        boneIterator
        !=
        boneMapping.end()
        )
    {
        int boneID =
            boneIterator->second;


        if (
            boneID >= 0
            &&
            boneID <
            static_cast<int>(
                finalBoneMatrices.size()
                )
            &&
            boneID <
            static_cast<int>(
                boneInfo.size()
                )
            )
        {
            // -------------------------------------------------
            // Final skinning matrix
            // -------------------------------------------------
            //
            // globalInverseTransform
            // *
            // current global bone transform
            // *
            // inverse bind matrix
            //
            finalBoneMatrices[boneID] =
                currentAnimation
                ->getGlobalInverseTransform()
                *
                globalTransform
                *
                boneInfo[boneID]
                .offset;
        }
    }


    // =================================================
    // Recursively process children
    // =================================================

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


// =====================================================
// Sample one bone animation
// =====================================================

LocalBoneTransform Animator::sampleBoneTransform(
    const BoneAnimation& boneAnimation,
    float animationTime
) const
{
    LocalBoneTransform result;


    // =================================================
    // Position
    // =================================================

    if (!boneAnimation.positions.empty())
    {
        // Only one keyframe.
        if (boneAnimation.positions.size()== 1)
        {
            result.position = boneAnimation.positions[0].position;
        }

        else
        {
            int index =
                getPositionIndex(boneAnimation,animationTime);

            int nextIndex = index + 1;


            const PositionKey& currentKey = boneAnimation.positions[index];

            const PositionKey& nextKey = boneAnimation.positions[nextIndex];
            float deltaTime = nextKey.timeStamp-currentKey.timeStamp;

            float factor = 0.0f;

            if (deltaTime > 0.0f)
            {
                factor =
                    (
                        animationTime
                        -
                        currentKey.timeStamp
                        )
                    /
                    deltaTime;
            }


            factor =
                std::clamp(
                    factor,
                    0.0f,
                    1.0f
                );


            result.position =
                glm::mix(
                    currentKey.position,
                    nextKey.position,
                    factor
                );
        }
    }


    // =================================================
    // Rotation
    // =================================================

    if (
        !boneAnimation
        .rotations
        .empty()
        )
    {
        if (
            boneAnimation
            .rotations
            .size()
            ==
            1
            )
        {
            result.rotation =
                boneAnimation
                .rotations[0]
                .orientation;
        }

        else
        {
            int index =
                getRotationIndex(
                    boneAnimation,
                    animationTime
                );


            int nextIndex =
                index + 1;


            const RotationKey& currentKey =
                boneAnimation
                .rotations[index];


            const RotationKey& nextKey =
                boneAnimation
                .rotations[nextIndex];


            float deltaTime =
                nextKey.timeStamp
                -
                currentKey.timeStamp;


            float factor =
                0.0f;


            if (deltaTime > 0.0f)
            {
                factor =
                    (
                        animationTime
                        -
                        currentKey.timeStamp
                        )
                    /
                    deltaTime;
            }


            factor =
                std::clamp(
                    factor,
                    0.0f,
                    1.0f
                );


            result.rotation =
                glm::slerp(
                    currentKey.orientation,
                    nextKey.orientation,
                    factor
                );


            result.rotation =
                glm::normalize(
                    result.rotation
                );
        }
    }


    // =================================================
    // Scale
    // =================================================

    if (
        !boneAnimation
        .scales
        .empty()
        )
    {
        if (
            boneAnimation
            .scales
            .size()
            ==
            1
            )
        {
            result.scale =
                boneAnimation
                .scales[0]
                .scale;
        }

        else
        {
            int index =
                getScaleIndex(
                    boneAnimation,
                    animationTime
                );


            int nextIndex =
                index + 1;


            const ScaleKey& currentKey =
                boneAnimation
                .scales[index];


            const ScaleKey& nextKey =
                boneAnimation
                .scales[nextIndex];


            float deltaTime =
                nextKey.timeStamp
                -
                currentKey.timeStamp;


            float factor =
                0.0f;


            if (deltaTime > 0.0f)
            {
                factor =
                    (
                        animationTime
                        -
                        currentKey.timeStamp
                        )
                    /
                    deltaTime;
            }


            factor =
                std::clamp(
                    factor,
                    0.0f,
                    1.0f
                );


            result.scale =
                glm::mix(
                    currentKey.scale,
                    nextKey.scale,
                    factor
                );
        }
    }


    return result;
}


// =====================================================
// Build matrix from Position / Rotation / Scale
// =====================================================

glm::mat4 Animator::buildTransformMatrix(
    const LocalBoneTransform& transform
) const
{
    // Translation matrix.
    glm::mat4 translationMatrix =
        glm::translate(
            glm::mat4(1.0f),
            transform.position
        );


    // Quaternion -> rotation matrix.
    glm::mat4 rotationMatrix =
        glm::mat4_cast(
            transform.rotation
        );


    // Scale matrix.
    glm::mat4 scaleMatrix =
        glm::scale(
            glm::mat4(1.0f),
            transform.scale
        );


    // Local transform:
    //
    // Translation
    // *
    // Rotation
    // *
    // Scale
    //
    return
        translationMatrix
        *
        rotationMatrix
        *
        scaleMatrix;
}


// =====================================================
// Blend two local bone transformations
// =====================================================

LocalBoneTransform Animator::blendTransforms(
    const LocalBoneTransform& previous,
    const LocalBoneTransform& current,
    float factor
) const
{
    LocalBoneTransform result;


    // -------------------------------------------------
    // Position
    // -------------------------------------------------
    //
    // Linear interpolation.
    //
    result.position =
        glm::mix(
            previous.position,
            current.position,
            factor
        );


    // -------------------------------------------------
    // Rotation
    // -------------------------------------------------
    //
    // Spherical interpolation between quaternions.
    //
    result.rotation =
        glm::slerp(
            previous.rotation,
            current.rotation,
            factor
        );


    result.rotation =
        glm::normalize(
            result.rotation
        );


    // -------------------------------------------------
    // Scale
    // -------------------------------------------------

    result.scale =
        glm::mix(
            previous.scale,
            current.scale,
            factor
        );


    return result;
}


// =====================================================
// Position keyframe lookup
// =====================================================

int Animator::getPositionIndex(
    const BoneAnimation& boneAnimation,
    float animationTime
) const
{
    int count =
        static_cast<int>(
            boneAnimation
            .positions
            .size()
            );


    for (
        int i = 0;
        i < count - 1;
        ++i
        )
    {
        if (
            animationTime
            <
            boneAnimation
            .positions[i + 1]
            .timeStamp
            )
        {
            return i;
        }
    }


    // Return the final valid interpolation pair.
    return
        std::max(
            0,
            count - 2
        );
}


// =====================================================
// Rotation keyframe lookup
// =====================================================

int Animator::getRotationIndex(
    const BoneAnimation& boneAnimation,
    float animationTime
) const
{
    int count =
        static_cast<int>(
            boneAnimation
            .rotations
            .size()
            );


    for (
        int i = 0;
        i < count - 1;
        ++i
        )
    {
        if (
            animationTime
            <
            boneAnimation
            .rotations[i + 1]
            .timeStamp
            )
        {
            return i;
        }
    }


    return
        std::max(
            0,
            count - 2
        );
}


// =====================================================
// Scale keyframe lookup
// =====================================================

int Animator::getScaleIndex(
    const BoneAnimation& boneAnimation,
    float animationTime
) const
{
    int count =
        static_cast<int>(
            boneAnimation
            .scales
            .size()
            );


    for (
        int i = 0;
        i < count - 1;
        ++i
        )
    {
        if (
            animationTime
            <
            boneAnimation
            .scales[i + 1]
            .timeStamp
            )
        {
            return i;
        }
    }


    return
        std::max(
            0,
            count - 2
        );
}