#pragma once

#include "../audio/AudioBuffer.h"

class ChatEngine
{
public:
    ChatEngine();
    ~ChatEngine();

    // This object owns an HTTP client and must not be copied.
    ChatEngine(const ChatEngine&) = delete;
    ChatEngine& operator=(const ChatEngine&) = delete;

    // Send recorded audio and receive the spoken reply.
    AudioBuffer chat(const AudioBuffer& audio);

private:
    struct Impl;
    Impl* impl;
};