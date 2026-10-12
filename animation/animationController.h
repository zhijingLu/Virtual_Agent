#pragma once

#include <string>

#include "Animation.h"
#include "Animator.h"
#include "../character/Model.h"
#include <vector>

enum class AnimationState
{
    Idle,
    Walk,
    Run,
    Talk,
    StandToSit,
    Sitting
};
struct StateStep
{
    AnimationState state;

    int repeatCount = 1;
};


class AnimationController
{
public:

    // Load all animation clips from the same model file.
    AnimationController(
        const std::string& modelPath,
        Model* model
    );


    // Update current animation every frame.
    void update(
        float deltaTime
    );
   // -------------------------------------------------
   // Single action mode
   // -------------------------------------------------
   //
   // The selected action will loop forever.
   //
   // Example:
   //
   // controller.playAction(
   //     CharacterAction::Walk
   // );
   //
    void playState(
		 AnimationState state
    );
    // -------------------------------------------------
    // Sequence mode
    // -------------------------------------------------
    //
    // Every action plays once.
    //
    // Example:
    //
    // Idle -> Walk -> Idle -> Talk
    //
    void playSequence(
		const std::vector<StateStep>& stateSequence
    );


    // Access Animator bone matrices.
    const std::vector<glm::mat4>&
        getFinalBoneMatrices() const;
    
     // Return whether a sequence is currently running.
    bool isSequencePlaying() const;


private:

    // All animation clips.
    Animation idle;
    Animation run;
    Animation sitting;
    Animation standToSit;
    Animation talk;
    Animation walk;


    // The actual animation player.
    Animator animator;

    // Play one action without changing
    // the controller mode.
    void playRawState(
        AnimationState state
    );


    // Start the next action
    // in the current sequence.
    void playNextState();
    // =================================================
    // Sequence control
    // =================================================

    std::vector<StateStep>
        actionSequence;


    size_t currentActionIndex = 0;


    bool sequencePlaying = false;
    int currentRepeatCount = 0;
};