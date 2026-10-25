#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

class WakeWordDetector
{
public:
    WakeWordDetector(
        const std::string& encoderPath,
        const std::string& decoderPath,
        const std::string& joinerPath,
        const std::string& tokensPath,
        const std::string& keywordsPath
    );

    ~WakeWordDetector();

    bool process(
        const int16_t* samples,
        size_t sampleCount,
        int sampleRate
    );

private:
    struct Impl;
    Impl* impl;
};