#include "ConversationClient.h"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <utility>

#include <httplib.h>
#include <nlohmann/json.hpp> // For JSON parsing and serialization.


using json = nlohmann::json;


ConversationClient::ConversationClient(
    const std::string& host,
    int port
)
    : host(host),
    port(port) {
}


ConversationClient::~ConversationClient() {
    stop();
}


void ConversationClient::start() {
    if (running.load()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex);

        replies.clear();
        notifications.clear();
    }
	// Clear the last delivered reply ID to allow re-delivery of the same reply if needed.
    lastDeliveredReplyId.clear();
    running.store(true);

    try {
        workerThread = std::thread(
            &ConversationClient::workerLoop,
            this
        );
    }
    catch (...) {
        running.store(false);
        throw;
    }
}


void ConversationClient::stop() {
    running.store(false);
    wakeCondition.notify_all();

    // An active HTTP request exits after its configured timeout.
    if (workerThread.joinable()) {
        workerThread.join();
    }
}


bool ConversationClient::tryGetReply(
    ConversationReply& reply
) {
    std::lock_guard<std::mutex> lock(mutex);

    if (replies.empty()) {
        return false;
    }

    reply = std::move(replies.front());
    replies.pop_front();

    return true;
}


void ConversationClient::notifyPlaybackStarted(
    const std::string& replyId
) {
    enqueueNotification(replyId, "started");
}


void ConversationClient::notifyPlaybackFinished(
    const std::string& replyId
) {
    enqueueNotification(replyId, "finished");
}


void ConversationClient::notifyPlaybackFailed(
    const std::string& replyId
) {
    enqueueNotification(replyId, "failed");
}


void ConversationClient::enqueueNotification(
    const std::string& replyId,
    const std::string& state
) {
    if (replyId.empty()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex);

        notifications.push_back({
            replyId,
            state
            });
    }
	// Notify the worker thread to process the new notification.
    wakeCondition.notify_one();
}


void ConversationClient::workerLoop() {
    httplib::Client client(host, port);

    client.set_connection_timeout(2, 0);
    client.set_read_timeout(3, 0);
    client.set_write_timeout(3, 0);

    while (running.load()) {
        try {
            PlaybackNotification notification;
            bool hasNotification = false;

            {
                std::lock_guard<std::mutex> lock(mutex);

                if (!notifications.empty()) {
                    notification = notifications.front();
                    hasNotification = true;
                }
            }

            if (hasNotification) {
                json body = {
                    { "reply_id", notification.replyId },
                    { "state", notification.state }
                };

                auto response = client.Post(
                    "/playback",
                    body.dump(),
                    "application/json"
                );

                if (response) {
                    bool removeNotification = false;

                    if (response->status == 200) {
                        removeNotification = true;
                    }
                    else if (
                        response->status == 404 ||
                        response->status == 409
                        ) {
                        // The server no longer owns this reply.
                        removeNotification = true;

                        std::cerr
                            << "Playback notification is stale: "
                            << notification.replyId
                            << '\n';
                    }
                    else if (
                        response->status >= 400 &&
                        response->status < 500
                        ) {
                        // Retrying an invalid request will not fix it.
                        removeNotification = true;

                        std::cerr
                            << "Playback notification rejected: "
                            << response->status
                            << '\n';
                    }

                    if (removeNotification) {
                        std::lock_guard<std::mutex> lock(mutex);

                        notifications.pop_front();
                    }
                }

                // Keep failed notifications queued for retry.
                // Preserve started -> finished ordering.
            }
            else {
                auto statusResponse = client.Get("/status");

                if (
                    statusResponse &&
                    statusResponse->status == 200
                    ) {
                    const json status = json::parse(
                        statusResponse->body
                    );

                    const std::string serverState =
                        status.value("status", std::string{});

                    if (
                        serverState == "ready" &&
                        status.contains("reply_id") &&
                        status["reply_id"].is_string()
                        ) {
                        const std::string replyId =
                            status["reply_id"].get<std::string>();

                        if (
                            !replyId.empty() &&
                            replyId != lastDeliveredReplyId
                            ) {
                            const std::string path =
                                "/reply/" + replyId;

                            auto audioResponse = client.Get(
                                path.c_str()
                            );

                            if (
                                audioResponse &&
                                audioResponse->status == 200
                                ) {
                                const std::string format =
                                    audioResponse->get_header_value(
                                        "X-Audio-Format"
                                    );

                                if (format != "pcm_s16le") {
                                    throw std::runtime_error(
                                        "Unsupported audio format."
                                    );
                                }

                                ConversationReply reply;

                                reply.replyId = replyId;

                                reply.userText = status.value(
                                    "user_text",
                                    std::string{}
                                );

                                reply.replyText = status.value(
                                    "reply_text",
                                    std::string{}
                                );

                                reply.audio.sampleRate = std::stoi(
                                    audioResponse->get_header_value(
                                        "X-Sample-Rate"
                                    )
                                );

                                reply.audio.channels = std::stoi(
                                    audioResponse->get_header_value(
                                        "X-Channels"
                                    )
                                );

                                const std::string& pcm =
                                    audioResponse->body;

                                if (
                                    reply.audio.sampleRate <= 0 ||
                                    reply.audio.channels != 1 ||
                                    pcm.empty() ||
                                    pcm.size() % 2 != 0
                                    ) {
                                    throw std::runtime_error(
                                        "Invalid PCM audio metadata."
                                    );
                                }

                                reply.audio.samples.resize(
                                    pcm.size() / 2
                                );

                                // Decode signed 16-bit little-endian PCM.
                                for (
                                    std::size_t i = 0;
                                    i < reply.audio.samples.size();
                                    ++i
                                    ) {
                                    const auto low =
                                        static_cast<unsigned char>(
                                            pcm[i * 2]
                                            );

                                    const auto high =
                                        static_cast<unsigned char>(
                                            pcm[i * 2 + 1]
                                            );

                                    const std::uint32_t value =
                                        static_cast<std::uint32_t>(low) |
                                        (
                                            static_cast<std::uint32_t>(high)
                                            << 8
                                            );

                                    const std::int32_t signedValue =
                                        value >= 32768
                                        ? static_cast<std::int32_t>(value)
                                        - 65536
                                        : static_cast<std::int32_t>(value);

                                    reply.audio.samples[i] =
                                        static_cast<std::int16_t>(
                                            signedValue
                                            );
                                }

                                {
                                    std::lock_guard<std::mutex> lock(mutex);

                                    replies.push_back(
                                        std::move(reply)
                                    );
                                }

                                lastDeliveredReplyId = replyId;
                            }
                        }
                    }
                }
            }
        }
        catch (const std::exception& error) {
            std::cerr
                << "Conversation client error: "
                << error.what()
                << '\n';
        }

        // A notification or stop request can interrupt this wait.
        std::unique_lock<std::mutex> lock(mutex);

        wakeCondition.wait_for(
            lock,
            std::chrono::milliseconds(200)
        );
    }
}