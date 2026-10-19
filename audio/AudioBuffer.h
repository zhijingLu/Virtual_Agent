#pragma once

#include <vector>
#include <cstdint>

struct AudioBuffer
{
    // Raw PCM(Pulse Code Modulation) samples.
    // Sample the sound waveform at fixed time intervals, 
    // then directly save the digital amplitude values ​​obtained from each sample.
    // 
    // int16_t means 16-bit signed PCM.
    //sample:The value of the sound waveform at a specific moment in time.
    std::vector<int16_t> samples;

    // Samples per second, e.g. 22050 / 24000 / 44100.
    int sampleRate = 24000;

    // 1 = mono, 2 = stereo（left and right audio channels）.
    int channels = 1;
};