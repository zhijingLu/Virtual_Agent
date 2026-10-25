#pragma once

#include <string>

#include "../animation/AnimationController.h"
#include "../audio/TTSEngine.h"
#include "../audio/AudioPlayer.h"
#include <atomic>
#include <mutex>
#include <optional>
#include <thread>
enum class CharacterState
{
    Idle,
    Walk,
    Run,
    Talk,
    StandToSit,
    Sitting
};

class CharacterController
{
public:
    CharacterController(
        AnimationController* animationController);
    ~CharacterController();
    void update(float deltaTime);

    void setState(CharacterState state);

    void speak(const std::string& text);

    CharacterState getState() const;

private:
    AnimationController* animationController;

    TTSEngine tts;
    AudioPlayer player;

    CharacterState currentState;

    bool speechActive = false;

    std::thread ttsThread;
    std::atomic<bool> ttsRunning{ false };

    std::mutex audioMutex;
    std::optional<AudioBuffer> pendingAudio;
};

