#include "AudioRecorder.h"

#include <iostream>
#include <mutex>
#include <utility>





struct AudioRecorder::Impl
{
    ma_device device{};

    bool initialized = false;

    AudioCallback callback;

    std::mutex callbackMutex;
};


// -------------------------------------------------
// Miniaudio capture callback,automatically called by miniaudio when new audio data is available
// -------------------------------------------------

void AudioRecorder::captureCallback(
    ma_device* device,
    void* output,
    const void* input,
    ma_uint32 frameCount
)
{
    if (
        device == nullptr ||
        input == nullptr
        )
    {
        return;
    }


    auto* impl =
        static_cast<AudioRecorder::Impl*>(
            device->pUserData
            );


    if (impl == nullptr)
        return;


    // Our microphone format is:
    // int16
    // mono
    // 16 kHz

    const int16_t* samples =
        static_cast<const int16_t*>(
            input
            );


    AudioBuffer chunk{};

    chunk.sampleRate = 16000;
    chunk.channels = 1;


    chunk.samples.assign(
        samples,
        samples + frameCount
    );

    //static int counter = 0;

    //if (++counter % 50 == 0)
    //{
    //    std::cout
    //        << "Mic chunk: "
    //        << frameCount
    //        << std::endl;
    //}
    // Pass this PCM chunk to SpeechRecognizer.
    {
        std::lock_guard<std::mutex> lock(
            impl->callbackMutex
        );

        if (impl->callback)
        {
            impl->callback(chunk);
        }
    }
}


// -------------------------------------------------
// Constructor
// -------------------------------------------------

AudioRecorder::AudioRecorder()
{
    impl = new Impl();
}


// -------------------------------------------------
// Destructor
// -------------------------------------------------

AudioRecorder::~AudioRecorder()
{
    stop();

    delete impl;

    impl = nullptr;
}


// -------------------------------------------------
// Start microphone capture
// -------------------------------------------------

bool AudioRecorder::start(
    AudioCallback callback
)
{
    if (impl->initialized)
        return true;


    impl->callback =
        std::move(callback);


    ma_device_config config =
        ma_device_config_init(
            ma_device_type_capture
        );


    // -------------------------------------------------
    // Audio format
    // -------------------------------------------------

    config.capture.format =
        ma_format_s16;

    config.capture.channels =
        1;

    config.sampleRate =
        16000;


    // -------------------------------------------------
    // Callback
    // -------------------------------------------------

    config.dataCallback =
        captureCallback;

    config.pUserData =
        impl;


    // -------------------------------------------------
    // Initialize device
    // -------------------------------------------------

    ma_result result =
        ma_device_init(
            nullptr,
            &config,
            &impl->device
        );


    if (result != MA_SUCCESS)
    {
        std::cerr
            << "Failed to initialize microphone."
            << std::endl;

        return false;
    }


    // -------------------------------------------------
    // Start capture
    // -------------------------------------------------

    result =
        ma_device_start(
            &impl->device
        );


    if (result != MA_SUCCESS)
    {
        std::cerr
            << "Failed to start microphone."
            << std::endl;

        ma_device_uninit(
            &impl->device
        );

        return false;
    }


    impl->initialized = true;


    std::cout
        << "Microphone started."
        << std::endl;


    return true;
}


// -------------------------------------------------
// Stop microphone capture
// -------------------------------------------------

void AudioRecorder::stop()
{
    if (!impl->initialized)
        return;


    ma_device_stop(
        &impl->device
    );


    ma_device_uninit(
        &impl->device
    );


    impl->initialized = false;


    std::cout
        << "Microphone stopped."
        << std::endl;
}