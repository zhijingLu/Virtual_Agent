#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

class ConsoleInput
{
public:
    ConsoleInput();
    ~ConsoleInput();

    void start();
    void stop();

    bool hasInput();
    bool tryPop(std::string& text);

private:
    void inputLoop();

    std::thread inputThread;

    std::mutex mutex;
    std::queue<std::string> inputQueue;
	// Flag to indicate if the input thread is running. specially useful for stopping the thread gracefully.
    std::atomic<bool> running{ false };
};