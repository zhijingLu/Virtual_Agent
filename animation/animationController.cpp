#include "AnimationController.h"


// =====================================================
// Constructor
// =====================================================

AnimationController::AnimationController(
    const std::string& modelPath,
    Model* model
)
    :
    // Animation index 0
    idle(
        modelPath,
        model,
        0
    ),

    // Animation index 1
    run(
        modelPath,
        model,
        1
    ),

    // Animation index 2
    sitting(
        modelPath,
        model,
        2
    ),

    // Animation index 3
    standToSit(
        modelPath,
        model,
        3
    ),

    // Animation index 4
    talk(
        modelPath,
        model,
        4
    ),

    // Animation index 5
    walk(
        modelPath,
        model,
        5
    ),

    // Start animator with idle animation.
    animator(
        &idle
    )

    // Default behavior:
    // idle animation loops forever.
{
    animator.setLoop(true);

}


// =====================================================
// Update
// =====================================================

void AnimationController::update(
    float deltaTime
)
{
    // Update the currently playing animation.
    animator.updateAnimation(
        deltaTime
    );


    // If we are not playing a sequence,
    // nothing else needs to happen.
    if (!sequencePlaying)
    {
        return;
    }


    // In sequence mode,
    // when the current animation finishes,
    // move to the next action.
    if (animator.isFinished())
    {
        playNextState();
    }
}


// =====================================================
// Single action mode
// =====================================================

void AnimationController::playState(
    AnimationState state
)
{
    // Stop sequence mode.
    sequencePlaying = false;
    actionSequence.clear();
    currentActionIndex = 0;
    // Single action should loop forever.
    animator.setLoop( true );
    // Play requested action.
    playRawState( state);
}


// =====================================================
// Sequence mode
// =====================================================

void AnimationController::playSequence(
	const std::vector<StateStep>& statesquence
)
{
    actionSequence = statesquence;

    currentActionIndex = 0;
    currentRepeatCount = 0;

    if (actionSequence.empty())
    {
        sequencePlaying = false;
        return;
    }

    sequencePlaying = true;

    // Sequence uses non-looping playback,
    // because repetition is controlled
    // by the controller.
    animator.setLoop(false);

    playRawState(
        actionSequence[0].state
    );
}


// =====================================================
// Play one action
// =====================================================

void AnimationController::playRawState(
    AnimationState state
)
{
    switch (state)
    {
    case AnimationState::Idle:

        animator.playAnimation(
            &idle
        );

        break;


    case AnimationState::Run:

        animator.playAnimation(
            &run
        );

        break;


    case AnimationState::Sitting:

        animator.playAnimation(
            &sitting
        );

        break;


    case AnimationState::StandToSit:

        animator.playAnimation(
            &standToSit
        );

        break;


    case AnimationState::Talk:

        animator.playAnimation(
            &talk
        );

        break;


    case AnimationState::Walk:

        animator.playAnimation(
            &walk
        );

        break;
    }
}


// =====================================================
// Play next action in sequence
// =====================================================

void AnimationController::playNextState()
{
    const StateStep& currentStep =actionSequence[currentActionIndex];

    // One playback has finished.
    ++currentRepeatCount;


    // -------------------------------------------------
    // Repeat current action
    // -------------------------------------------------

    if (
        currentRepeatCount
        <
        currentStep.repeatCount
        )
    {
        animator.setLoop(false);

        playRawState(
            currentStep.state
        );

        return;
    }


    // -------------------------------------------------
    // Current action finished all repetitions
    // -------------------------------------------------

    currentRepeatCount = 0;

    ++currentActionIndex;


    // -------------------------------------------------
    // Entire sequence finished
    // -------------------------------------------------

    if (currentActionIndex >= actionSequence.size())
    {
        sequencePlaying = false;

        // Keep final action looping.
        animator.setLoop(true);

        playRawState(
            actionSequence.back().state
        );

        return;
    }


    // -------------------------------------------------
    // Start next action
    // -------------------------------------------------

    animator.setLoop(false);

    playRawState(actionSequence[currentActionIndex].state);
   


}


// =====================================================
// Get bone matrices
// =====================================================

const std::vector<glm::mat4>&
AnimationController::getFinalBoneMatrices() const
{
    return animator
        .getFinalBoneMatrices();
}


// =====================================================
// Check sequence status
// =====================================================

bool AnimationController::isSequencePlaying() const
{
    return sequencePlaying;
}