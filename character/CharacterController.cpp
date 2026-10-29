#include "CharacterController.h"
#include <utility>


CharacterController::CharacterController(
    AnimationController* animationController
   
)
    : animationController(animationController),
    currentState(CharacterState::Idle)
{
}

void CharacterController::setState(
    CharacterState state
)
{
    if (currentState == state)
        return;

    currentState = state;

    switch (state)
    {
    case CharacterState::Idle:
        animationController->playState(
            AnimationState::Idle
        );
        break;

    case CharacterState::Walk:
        animationController->playState(
            AnimationState::Walk
        );
        break;

    case CharacterState::Run:
        animationController->playState(
            AnimationState::Run
        );
        break;

    case CharacterState::Talk:
        animationController->playState(
            AnimationState::Talk
        );
        break;

    case CharacterState::Sitting:
        animationController->playState(
            AnimationState::Sitting
        );
        break;
    
    case CharacterState::StandToSit:
		animationController->playState(
			AnimationState::StandToSit
		);
		break;
    }
}

void CharacterController::speak(
    const std::string& text
)
{
    if (ttsRunning)
        return;
    // Previous TTS thread may have finished,
   // but it still needs to be joined.
    if (ttsThread.joinable())
    {
		// Wait for the previous TTS thread to finish before starting a new one.
        ttsThread.join();
    }


    ttsRunning = true;
    ttsThread = std::thread(
        [this, text]()
        {
            AudioBuffer audio =
                tts.synthesize(text);

            if (!audio.samples.empty())
            {
                std::lock_guard<std::mutex> lock(audioMutex);

                pendingAudio = std::move(audio);
            }

            ttsRunning = false;
        });
}


void CharacterController::speakAudio(AudioBuffer audio)
{
    if (audio.samples.empty())
        return;

    std::lock_guard<std::mutex> lock(audioMutex);

    pendingAudio = std::move(audio);
}

void CharacterController::update(
    float deltaTime
)
{
    {
        std::lock_guard<std::mutex> lock(
            audioMutex
        );

        if (pendingAudio.has_value())
        {
            player.play(*pendingAudio);

            pendingAudio.reset();

            speechActive = true;

            setState(CharacterState::Talk);
        }
    }
    if (speechActive &&!player.isPlaying())
    {
        speechActive = false;

        setState( CharacterState::Idle );
    }

    animationController->update(
        deltaTime
    );
}

CharacterState CharacterController::getState() const
{
    return currentState;
}

CharacterController::~CharacterController()
{
    if (ttsThread.joinable())
    {
        ttsThread.join();
    }
}