#include "STTEngine.h"

#include "httplib.h"

#include <cstring>
#include <iostream>
#include <string>

struct STTEngine::Impl
{
    // HTTP client used to communicate with
    // the local Python STT server.
    httplib::Client client;

    Impl()
        : client("127.0.0.1", 5000)
    {
    }
};


STTEngine::STTEngine()
{
    impl = new Impl();
}


STTEngine::~STTEngine()
{
    delete impl;
}


std::string STTEngine::transcribe(const AudioBuffer& audio)
{
    // Implementation for transcribing audio
    if(audio.samples.empty())
    {
        std::cerr
            << "STT error: audio buffer is empty."
            << std::endl;
        return "";
    }
    // Convert int16 PCM samples into raw binary bytes.
    const char* audioData = reinterpret_cast<const char*>(audio.samples.data());
    size_t audioSize =audio.samples.size()* sizeof(int16_t);
    // Store PCM data inside std::string because
    // cpp-httplib accepts the request body as a string.
    std::string body(audioData,audioSize);
    // Send audio metadata as HTTP headers.
    httplib::Headers headers =
    {
        {"X-Sample-Rate",std::to_string(audio.sampleRate)},
        { "X-Channels", std::to_string(audio.channels)}
    };
    auto response =
        impl->client.Post(
            "/stt",
            headers,
            body,
            "application/octet-stream"
        );
    if (!response)
    {
        std::cerr
            << "STT error: could not connect to Python server."
            << std::endl;

        return "";
    }


    if (response->status != 200)
    {
        std::cerr
            << "STT error: server returned status "
            << response->status
            << std::endl;

        return "";
    }


    // Python server returns recognized text directly.
    return response->body;
}