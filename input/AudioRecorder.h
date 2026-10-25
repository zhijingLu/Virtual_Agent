#pragma once

#include <functional>

#include "../audio/AudioBuffer.h"
#include "miniaudio.h"
class AudioRecorder
{
public:
    using AudioCallback =
        std::function<void(const AudioBuffer&)>;

    AudioRecorder();
    ~AudioRecorder();

    bool start(
        AudioCallback callback
    );

    void stop();

private:
    struct Impl;
    Impl* impl;
    static void captureCallback(
        ma_device* device,
        void* output,
        const void* input,
        ma_uint32 frameCount
    );
};