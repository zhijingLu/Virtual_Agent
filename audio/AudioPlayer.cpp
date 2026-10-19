
#include "miniaudio.h"


#include "AudioPlayer.h"
#include <iostream>

// Complete definition of AudioPlayer::Impl.
//
// In AudioPlayer.h we only wrote:
//     struct Impl;
//
// Here the compiler finally learns what members
// actually exist inside Impl.
//
// ma_engine is the audio engine provided by miniaudio.
// initialized records whether the engine was
// successfully initialized.
struct AudioPlayer::Impl
{
    ma_engine engine{};

    bool initialized = false;

    // miniaudio audio buffer.
    ma_audio_buffer audioBuffer{};
    // Keep a copy of PCM samples alive while miniaudio is playing.
    std::vector<int16_t> currentSamples;

	// Sound object used to play the buffer. like a player that plays the audio buffer.
    ma_sound sound{};

    bool bufferInitialized = false;
    bool soundInitialized = false;
};

AudioPlayer::AudioPlayer()
{
    // Create the actual implementation object on the heap.
    //
    // The relationship is:
    //
    // AudioPlayer
    //     |
    //     v
    //   impl
    //     |
    //     v
    // Impl object
    impl = new Impl();

    // Initialize the miniaudio engine.
    //
    // &impl->engine means:
    // get the address of the engine member stored inside Impl
    // and pass that address to miniaudio.
    if (ma_engine_init(nullptr, &impl->engine) == MA_SUCCESS)
    {
        impl->initialized = true;
    }
    else
    {
        std::cerr << "Failed to initialize audio engine.\n";
    }
}

AudioPlayer::~AudioPlayer()
{
    // Release sound resources first.
    if (impl->soundInitialized)
    {
        ma_sound_uninit(&impl->sound);
    }

    // Release the memory audio buffer.
    if (impl->bufferInitialized)
    {
        ma_audio_buffer_uninit(&impl->audioBuffer);
    }
    // If the engine was initialized successfully,
    // release the miniaudio engine resources.
    if (impl->initialized)
    {
        ma_engine_uninit(&impl->engine);
    }

    // Impl was created with new,
    // so it must be released with delete.
    delete impl;
}

bool AudioPlayer::play(const AudioBuffer& buffer)
{
    // Audio cannot be played if the engine
    // was not initialized successfully.
    if (!impl->initialized)
        return false;
    if (buffer.samples.empty())
        return false;

    // If an old sound exists, release it first.
    if (impl->soundInitialized)
    {
        ma_sound_uninit(&impl->sound);
        impl->soundInitialized = false;
    }

    // If an old audio buffer exists, release it first.
    if (impl->bufferInitialized)
    {
        ma_audio_buffer_uninit(&impl->audioBuffer);
        impl->bufferInitialized = false;
    }
    // IMPORTANT:
    // Keep our own copy of the PCM samples.
    // miniaudio will keep reading this memory asynchronously.
    impl->currentSamples = buffer.samples;
    // Configure the PCM format.
    ma_audio_buffer_config bufferConfig =
        ma_audio_buffer_config_init(
            ma_format_s16,
            static_cast<ma_uint32>(buffer.channels),
            // miniaudio expects the number of PCM frames,
            // not the total number of individual samples.
            static_cast<ma_uint64>(
                impl->currentSamples.size() / buffer.channels
                ), 
            impl->currentSamples.data(), // Pointer to the PCM samples stored in memory.
            nullptr
        );
    bufferConfig.sampleRate = static_cast<ma_uint32>(buffer.sampleRate);

    // Initialize the miniaudio memory buffer.
    ma_result result =
        ma_audio_buffer_init(
            &bufferConfig,
            &impl->audioBuffer
        );

    if (result != MA_SUCCESS)
    {
        std::cerr << "Failed to initialize audio buffer.\n";
        return false;
    }

    impl->bufferInitialized = true;
    // Set the sample rate directly in the buffer configuration.
    

    // Create a sound object from the memory buffer.
    result = ma_sound_init_from_data_source(
        &impl->engine,
        &impl->audioBuffer,
        0,
        nullptr,
        &impl->sound
    );

    if (result != MA_SUCCESS)
    {
        std::cerr << "Failed to initialize sound.\n";
        return false;
    }

    impl->soundInitialized = true;

    // Start playback.
    result = ma_sound_start(&impl->sound);

    if (result != MA_SUCCESS)
    {
        std::cerr << "Failed to start audio playback.\n";
        return false;
    }

    return true;
}