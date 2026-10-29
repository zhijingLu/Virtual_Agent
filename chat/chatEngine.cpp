#include "ChatEngine.h"
#include "httplib.h"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>


struct ChatEngine::Impl
{
    httplib::Client client;

    Impl()
        : client("127.0.0.1", 5000)
    {
        client.set_connection_timeout(5, 0);

        // Wait for Whisper + LLM + TTS.
        client.set_read_timeout(200, 0);
        client.set_write_timeout(30, 0);
    }
};


ChatEngine::ChatEngine()
    : impl(new Impl())
{
}


ChatEngine::~ChatEngine()
{
    delete impl;
}


AudioBuffer ChatEngine::chat(const AudioBuffer& audio)
{
    AudioBuffer reply;

    // Validate recorded audio.
    if (audio.samples.empty() ||
        audio.sampleRate != 16000 ||
        (audio.channels != 1 && audio.channels != 2) ||
        audio.samples.size() % audio.channels != 0)
    {
        std::cerr
            << "Chat error: expected non-empty 16000 Hz "
            << "mono or stereo audio.\n";

        return reply;
    }

    // Convert recorded int16 samples into HTTP bytes.
    const char* audioData =
        reinterpret_cast<const char*>(audio.samples.data());

    const size_t audioSize =
        audio.samples.size() * sizeof(int16_t);

    std::string body(audioData, audioSize);

    httplib::Headers headers =
    {
        {"X-Sample-Rate", std::to_string(audio.sampleRate)},
        {"X-Channels", std::to_string(audio.channels)}
    };

    // Python performs Whisper -> LLM -> TTS.
    auto response = impl->client.Post(
        "/chat",
        headers,
        body,
        "application/octet-stream"
    );

    if (!response)
    {
        std::cerr
            << "Chat error: HTTP request failed "
            << "(connection or timeout).\n";

        return reply;
    }

    if (response->status != 200)
    {
        std::cerr
            << "Chat server returned HTTP "
            << response->status
            << ": " << response->body << "\n";

        return reply;
    }

    // Read and validate the returned audio metadata.
    try
    {
        if (!response->has_header("X-Sample-Rate") ||
            !response->has_header("X-Channels"))
        {
            throw std::runtime_error("Missing audio headers");
        }

        reply.sampleRate = std::stoi(
            response->get_header_value("X-Sample-Rate")
        );

        reply.channels = std::stoi(
            response->get_header_value("X-Channels")
        );

        if (reply.sampleRate <= 0 ||
            (reply.channels != 1 && reply.channels != 2))
        {
            throw std::runtime_error("Invalid audio metadata");
        }
    }
    catch (const std::exception& error)
    {
        std::cerr
            << "Chat error: " << error.what() << "\n";

        return AudioBuffer{};
    }

    // Response contains little-endian int16 PCM.
    const std::string& pcmBytes = response->body;

    const size_t frameSize =
        sizeof(int16_t) * reply.channels;

    if (pcmBytes.empty() || pcmBytes.size() % frameSize != 0)
    {
        std::cerr << "Chat error: invalid PCM response.\n";
        return AudioBuffer{};
    }

    reply.samples.resize(
        pcmBytes.size() / sizeof(int16_t)
    );

    std::memcpy(
        reply.samples.data(),
        pcmBytes.data(),
        pcmBytes.size()
    );

    return reply;
}