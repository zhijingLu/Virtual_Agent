#pragma once

#include <vector>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Animation.h"


// =====================================================
// Local bone transformation
// =====================================================
//
// We keep Position / Rotation / Scale separately
// because animation blending should not directly
// interpolate two transformation matrices.
//
// Position -> linear interpolation
// Rotation -> quaternion interpolation
// Scale    -> linear interpolation
//
struct LocalBoneTransform
{
    glm::vec3 position =
        glm::vec3(0.0f);

    glm::quat rotation =
        glm::quat(
            1.0f,
            0.0f,
            0.0f,
            0.0f
        );

    glm::vec3 scale =
        glm::vec3(1.0f);
};


// =====================================================
// Animator
// =====================================================

class Animator
{
public:

    // Create Animator with an initial animation.
    Animator(
        Animation* animation
    );


    // Update animation every frame.
    void updateAnimation(
        float deltaTime
    );


    // Switch to another animation.
    //
    // blendDuration:
    // how long the transition from the previous
    // animation to the new animation should take.
    //
    // Example:
    //
    // animator.playAnimation(&walk);
    //
    // default transition time = 0.2 seconds.
    //
    void playAnimation(
        Animation* animation,
        float blendDuration = 0.2f
    );


    // true:
    // current animation repeats forever.
    //
    // false:
    // current animation plays once and stops
    // at the final frame.
    //
    void setLoop(
        bool shouldLoop
    );


    // Returns true when a non-looping animation
    // reaches its end.
    bool isFinished() const;


    // Final bone matrices used by GPU skinning.
    const std::vector<glm::mat4>&
        getFinalBoneMatrices() const;


private:

    // =================================================
    // Skeleton calculation
    // =================================================

    // Recursively calculate the transformation
    // of every node/bone in the hierarchy.
    void calculateBoneTransform(
        const AssimpNodeData& node,
        const glm::mat4& parentTransform
    );


    // =================================================
    // Animation sampling
    // =================================================

    // Read Position / Rotation / Scale
    // from one animation channel at a specific time.
    LocalBoneTransform sampleBoneTransform(
        const BoneAnimation& boneAnimation,
        float animationTime
    ) const;


    // Build transformation matrix:
    //
    // Translation * Rotation * Scale
    //
    glm::mat4 buildTransformMatrix(
        const LocalBoneTransform& transform
    ) const;


    // Blend two local bone transformations.
    //
    // factor:
    //
    // 0.0 -> 100% previous animation
    // 0.5 -> 50% previous + 50% current
    // 1.0 -> 100% current animation
    //
    LocalBoneTransform blendTransforms(
        const LocalBoneTransform& previous,
        const LocalBoneTransform& current,
        float factor
    ) const;


    // =================================================
    // Keyframe lookup
    // =================================================

    int getPositionIndex(
        const BoneAnimation& boneAnimation,
        float animationTime
    ) const;


    int getRotationIndex(
        const BoneAnimation& boneAnimation,
        float animationTime
    ) const;


    int getScaleIndex(
        const BoneAnimation& boneAnimation,
        float animationTime
    ) const;


private:

    // =================================================
    // Current animation
    // =================================================

    Animation* currentAnimation =
        nullptr;


    // Current animation time.
    //
    // Unit:
    // animation ticks.
    //
    float currentTime =
        0.0f;


    // =================================================
    // Previous animation
    // =================================================
    //
    // Used only during blending.
    //

    Animation* previousAnimation =nullptr;


    // Time of the previous animation when
    // the transition started.
    //
    // We freeze the previous pose at this time
    // while blending toward the new animation.
    //
    float previousTime =0.0f;


    // =================================================
    // Playback state
    // =================================================

    // true:
    // animation loops.
    //
    // false:
    // animation stops after one playback.
    //
    bool loop = true;


    // true when a non-looping animation
    // reaches the end.
    //
    bool finished = false;


    // =================================================
    // Blending state
    // =================================================

    // Are we currently transitioning between
    // two animations?
    bool blending = false;


    // Time already spent blending.
    //
    // Unit:
    // seconds.
    //
    float blendTime = 0.0f;


    // Total duration of the transition.
    //
    // Unit:
    // seconds.
    //
    float blendDuration = 0.2f;


    // =================================================
    // GPU skinning matrices
    // =================================================

    std::vector<glm::mat4>
        finalBoneMatrices;


    static constexpr int MAX_BONES =
        100;
};