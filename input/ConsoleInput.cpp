#include "ConsoleInput.h"

#include <iostream>

ConsoleInput::ConsoleInput()
{
}

ConsoleInput::~ConsoleInput()
{
    stop();
}

void ConsoleInput::start()
{
    if (running)
        return;

    running = true;

    inputThread =
        std::thread(
            &ConsoleInput::inputLoop,
            this
        );
}

void ConsoleInput::stop()
{
    if (!running)
        return;

    running = false;

    // std::getline() may still be blocking here.
    // Press Enter once in the console to let the input thread continue.

    if (inputThread.joinable())
    {
        inputThread.join();
    }
}

void ConsoleInput::inputLoop()
{
    while (running)
    {
        std::string text;

        std::cout << "Input: ";

        if (!std::getline(std::cin, text))
        {
            break;
        }

        if (!running)
            break;

        if (text.empty())
            continue;

        {
            std::lock_guard<std::mutex> lock(mutex);

            inputQueue.push(text);
        }
    }
}

bool ConsoleInput::hasInput()
{
    std::lock_guard<std::mutex> lock(mutex);

    return !inputQueue.empty();
}

bool ConsoleInput::tryPop(std::string& text)
{
    std::lock_guard<std::mutex> lock(mutex);

    if (inputQueue.empty())
        return false;

    text = inputQueue.front();

    inputQueue.pop();

    return true;
}