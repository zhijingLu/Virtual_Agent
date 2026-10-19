#pragma once

#include <string>
#include "AudioBuffer.h"

class TTSEngine
{
public:
    TTSEngine();
    ~TTSEngine();

    // Convert text into PCM audio.
    AudioBuffer synthesize(const std::string& text);

private:
    struct Impl;
    Impl* impl;
};