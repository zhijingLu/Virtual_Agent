#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "../audio/AudioBuffer.h"

struct ConversationReply {
    std::string replyId;
    std::string userText;
    std::string replyText;

    AudioBuffer audio;
};

class ConversationClient {
public:
    explicit ConversationClient(
        const std::string& host = "127.0.0.1",
        int port = 5000
    );

    ~ConversationClient();

    ConversationClient(const ConversationClient&) = delete;
    ConversationClient& operator=(const ConversationClient&) = delete;

    void start();
    void stop();

    bool tryGetReply(ConversationReply& reply);

    void notifyPlaybackStarted(const std::string& replyId);
    void notifyPlaybackFinished(const std::string& replyId);
    void notifyPlaybackFailed(const std::string& replyId);

private:
    struct PlaybackNotification {
        std::string replyId;
        std::string state;
    };

    void workerLoop();

    void enqueueNotification(
        const std::string& replyId,
        const std::string& state
    );

    std::string host;
    int port;

    std::atomic<bool> running{ false };
    std::thread workerThread;

    std::mutex mutex;
    std::condition_variable wakeCondition;

    std::deque<ConversationReply> replies;
    std::deque<PlaybackNotification> notifications;

    // Accessed only by the network thread.
    std::string lastDeliveredReplyId;
};