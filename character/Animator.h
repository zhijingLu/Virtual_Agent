//Based on the current time, calculate the position, rotation, and scale for each bone in this frame.

#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "Animation.h"


// Maximum number of bone matrices sent to the shader.
const int MAX_BONES = 100;


class Animator
{
public:

    // Create an Animator for one animation.
    Animator(
        Animation* animation
    );


    // Advance animation time and calculate
    // all bone matrices for the current frame.
    void updateAnimation(
        float deltaTime
    );


    // Change the currently playing animation.
    void playAnimation(
        Animation* animation
    );


    // Return final bone matrices that will
    // be sent to the vertex shader.
    const std::vector<glm::mat4>&
        getFinalBoneMatrices() const;


private:

    // Animation currently being played.
    Animation* currentAnimation =
        nullptr;


    // Current animation time in Assimp ticks.
    float currentTime = 0.0f;


    // Final skinning matrices.
    std::vector<glm::mat4>
        finalBoneMatrices;


    // Recursively calculate the transformation
    // of every node/bone in the hierarchy.
    void calculateBoneTransform(
        const AssimpNodeData& node,
        const glm::mat4& parentTransform
    );


    // Calculate current local transformation
    // of one animated node.
    glm::mat4 calculateLocalTransform(
        const BoneAnimation& animation
    );


    // Interpolate position at currentTime.
    glm::vec3 interpolatePosition(
        const BoneAnimation& animation
    );


    // Interpolate rotation at currentTime.
    glm::quat interpolateRotation(
        const BoneAnimation& animation
    );


    // Interpolate scale at currentTime.
    glm::vec3 interpolateScale(
        const BoneAnimation& animation
    );


    // Find two neighboring position keyframes.
    int findPositionIndex(
        const BoneAnimation& animation
    ) const;


    // Find two neighboring rotation keyframes.
    int findRotationIndex(
        const BoneAnimation& animation
    ) const;


    // Find two neighboring scale keyframes.
    int findScaleIndex(
        const BoneAnimation& animation
    ) const;


    // Calculate interpolation factor between
    // two keyframe timestamps.
    float calculateFactor(
        float lastTime,
        float nextTime
    ) const;
};