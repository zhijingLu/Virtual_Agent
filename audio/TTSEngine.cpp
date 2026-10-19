#include "TTSEngine.h"

#include "httplib.h"

#include <cstring>
#include <iostream>


struct TTSEngine::Impl
{
    // HTTP client used to communicate with
    // the local Python TTS server.
    httplib::Client client;

    Impl()
        : client("127.0.0.1", 5000)
    {
    }
};


TTSEngine::TTSEngine()
{
    impl = new Impl();
}


TTSEngine::~TTSEngine()
{
    delete impl;
}


AudioBuffer TTSEngine::synthesize(const std::string& text)
{
    AudioBuffer buffer;

    if (text.empty())
    {
        return buffer;
    }

    // JSON request body.
    //
    // Example:
    //
    // {
    //     "text": "Hello world"
    // }
    std::string body =
        "{\"text\":\"" + text + "\"}";


    // Send text to:
    //
    // POST http://127.0.0.1:5000/tts
    auto response = impl->client.Post(
        "/tts",
        body,
        "application/json"
    );


    // Failed to connect to the Python server.
    if (!response)
    {
        std::cerr
            << "Failed to connect to TTS server.\n";

        return buffer;
    }


    // The Python server should return HTTP 200
    // when synthesis succeeds.
    if (response->status != 200)
    {
        std::cerr
            << "TTS server returned HTTP "
            << response->status
            << "\n";

        return buffer;
    }


    // Read audio metadata from HTTP headers.
    //
    // Python sends:
    //
    // X-Sample-Rate: 44100
    // X-Channels: 1
    // std::stoi ->string to integer

    if (response->has_header("X-Sample-Rate"))
    {
        buffer.sampleRate =
            std::stoi(
                response->get_header_value(
                    "X-Sample-Rate"
                )
            );
    }


    if (response->has_header("X-Channels"))
    {
        buffer.channels =
            std::stoi(
                response->get_header_value(
                    "X-Channels"
                )
            );
    }


    // HTTP response body contains raw PCM bytes.
    const std::string& pcmBytes =
        response->body;


    // Each int16_t sample occupies 2 bytes.
    size_t sampleCount =
        pcmBytes.size() / sizeof(int16_t);


    // Allocate enough space for all samples.
    buffer.samples.resize(sampleCount);


    // Copy raw HTTP bytes into the int16 PCM buffer.
    std::memcpy(
        buffer.samples.data(),
        pcmBytes.data(),
        pcmBytes.size()
    );


    return buffer;
}