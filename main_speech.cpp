#include "audio/TTSEngine.h"
#include "audio/AudioPlayer.h"

#include <iostream>
#include <string>

int main()
{
    TTSEngine tts;
    AudioPlayer player;

    while (true)
    {
        std::string text;

        std::cout << "Say: ";
        std::getline(std::cin, text);

        if (text == "exit")
            break;

        AudioBuffer audio = tts.synthesize(text);
        std::cout << "processing..... "
            << "\n";

        std::cout << "Samples: "
            << audio.samples.size()
            << "\n";

        std::cout << "Sample rate: "
            << audio.sampleRate
            << "\n";

        std::cout << "Channels: "
            << audio.channels
            << "\n";

        if (!audio.samples.empty())
        {
            player.play(audio);
        }
        // Important:
       // keep the program alive while audio is playing.
        std::cout << "Press Enter after playback..." << std::endl;
        std::cin.get();
    }

    return 0;
}