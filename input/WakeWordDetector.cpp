#include "WakeWordDetector.h"

#include <iostream>
#include <stdexcept>
#include <vector>

#include "sherpa-onnx/c-api/c-api.h"


struct WakeWordDetector::Impl
{
    const SherpaOnnxKeywordSpotter* spotter = nullptr;

    const SherpaOnnxOnlineStream* stream = nullptr;
};


WakeWordDetector::WakeWordDetector(
    const std::string& encoderPath,
    const std::string& decoderPath,
    const std::string& joinerPath,
    const std::string& tokensPath,
    const std::string& keywordsPath
)
{
    impl = new Impl();


    SherpaOnnxKeywordSpotterConfig config{};


    // -------------------------------------------------
    // Model files
    // -------------------------------------------------

    config.model_config.transducer.encoder =
        encoderPath.c_str();

    config.model_config.transducer.decoder =
        decoderPath.c_str();

    config.model_config.transducer.joiner =
        joinerPath.c_str();


    config.model_config.tokens =
        tokensPath.c_str();


    // -------------------------------------------------
    // Runtime configuration
    // -------------------------------------------------

    config.model_config.num_threads = 1;

    config.model_config.provider =
        "cpu";


    // -------------------------------------------------
    // Keyword configuration
    // -------------------------------------------------

    config.keywords_file =
        keywordsPath.c_str();

    config.max_active_paths = 4;

    config.keywords_score = 3.0f;

    config.keywords_threshold = 0.1f;


    // -------------------------------------------------
    // Create keyword spotter
    // -------------------------------------------------

    impl->spotter =
        SherpaOnnxCreateKeywordSpotter(
            &config
        );


    if (impl->spotter == nullptr)
    {
        delete impl;
        impl = nullptr;

        throw std::runtime_error(
            "Failed to create sherpa-onnx keyword spotter."
        );
    }


    // -------------------------------------------------
    // Create streaming recognition stream
    // -------------------------------------------------

    impl->stream =
        SherpaOnnxCreateKeywordStream(
            impl->spotter
        );


    if (impl->stream == nullptr)
    {
        SherpaOnnxDestroyKeywordSpotter(
            impl->spotter
        );

        delete impl;
        impl = nullptr;

        throw std::runtime_error(
            "Failed to create sherpa-onnx keyword stream."
        );
    }


    std::cout
        << "WakeWordDetector initialized."
        << std::endl;
}


WakeWordDetector::~WakeWordDetector()
{
    if (impl == nullptr)
        return;


    if (impl->stream != nullptr)
    {
        SherpaOnnxDestroyOnlineStream(
            impl->stream
        );
    }


    if (impl->spotter != nullptr)
    {
        SherpaOnnxDestroyKeywordSpotter(
            impl->spotter
        );
    }


    delete impl;

    impl = nullptr;
}


bool WakeWordDetector::process(
    const int16_t* samples,
    size_t sampleCount,
    int sampleRate
)
{
    
    if (
        impl == nullptr ||
        impl->spotter == nullptr ||
        impl->stream == nullptr
        )
    {
        return false;
    }


    if (
        samples == nullptr ||
        sampleCount == 0
        )
    {
        return false;
    }


    // -------------------------------------------------
    // Convert int16 PCM to float [-1, 1]
    // -------------------------------------------------

    std::vector<float> floatSamples(
        sampleCount
    );


    for (
        size_t i = 0;
        i < sampleCount;
        ++i
        )
    {
        floatSamples[i] =
            static_cast<float>(
                samples[i]
                )
            / 32768.0f;
    }


    // -------------------------------------------------
    // Feed audio into sherpa-onnx
    // -------------------------------------------------

    SherpaOnnxOnlineStreamAcceptWaveform(
        impl->stream,
        sampleRate,
        floatSamples.data(),
        static_cast<int32_t>(
            floatSamples.size()
            )
    );

   
    // -------------------------------------------------
    // Decode available frames
    // -------------------------------------------------

    while (
        SherpaOnnxIsKeywordStreamReady(
            impl->spotter,
            impl->stream
        )
        )
    {
        SherpaOnnxDecodeKeywordStream(
            impl->spotter,
            impl->stream
        );
       
    }

    // -------------------------------------------------
    // Get keyword result
    // -------------------------------------------------

    const SherpaOnnxKeywordResult* result =
        SherpaOnnxGetKeywordResult(
            impl->spotter,
            impl->stream
        );


    bool detected = false;


    if (
        result != nullptr &&
        result->keyword != nullptr &&
        result->keyword[0] != '\0'
        )
    {
        std::cout
            << "Wake word detected: "
            << result->keyword
            << std::endl;


        detected = true;


        // Reset stream after detection.
        SherpaOnnxResetKeywordStream(
            impl->spotter,
            impl->stream
        );
    }


    if (result != nullptr)
    {
        SherpaOnnxDestroyKeywordResult(
            result
        );
    }


    return detected;
}