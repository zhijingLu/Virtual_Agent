#pragma once

#include <string>
#include "AudioBuffer.h"

class STTEngine
{
public:
    STTEngine();
    ~STTEngine();
	// Transcribe the given audio buffer and return the recognized text.
    std::string transcribe(const AudioBuffer& audio);


private:
    struct Impl;
    Impl* impl;
};