#include "SpeechRecognizer.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <utility>
#include <exception>

SpeechRecognizer::SpeechRecognizer()
    :wakeWordDetector(
        "external/kws/sherpa-onnx-kws-zipformer-gigaspeech-3.3M-2024-01-01/encoder-epoch-12-avg-2-chunk-16-left-64.int8.onnx",
        "external/kws/sherpa-onnx-kws-zipformer-gigaspeech-3.3M-2024-01-01/decoder-epoch-12-avg-2-chunk-16-left-64.int8.onnx",
        "external/kws/sherpa-onnx-kws-zipformer-gigaspeech-3.3M-2024-01-01/joiner-epoch-12-avg-2-chunk-16-left-64.int8.onnx",
        "external/kws/sherpa-onnx-kws-zipformer-gigaspeech-3.3M-2024-01-01/tokens.txt",
        "external/kws/sherpa-onnx-kws-zipformer-gigaspeech-3.3M-2024-01-01/keywords.txt")
{
}


SpeechRecognizer::~SpeechRecognizer()
{
    stop();
}


// -------------------------------------------------
// Start Whisper worker
// -------------------------------------------------

void SpeechRecognizer::start()
{
    if (running)
        return;

    running = true;

    workerThread =
        std::thread(
            &SpeechRecognizer::workerLoop,
            this
        );

    std::cout
        << "SpeechRecognizer started."
        << std::endl;
}


// -------------------------------------------------
// Stop Whisper worker
// -------------------------------------------------

void SpeechRecognizer::stop()
{
    if (!running)
        return;

    running = false;

    if (workerThread.joinable())
    {
        workerThread.join();
    }

    std::cout
        << "SpeechRecognizer stopped."
        << std::endl;
}


// -------------------------------------------------
// Wake word detected
// -------------------------------------------------

void SpeechRecognizer::notifyWakeWordDetected()
{
    // Ignore another wake word while already recording.
    if (state !=RecognitionState::WaitingForWakeWord)
    {
        return;
    }

    std::cout
        << "Wake word detected."
        << std::endl;


    // Start collecting the user's command.
    state =RecognitionState::RecordingCommand;


    recordedSamples.clear();

    // Copy recent audio from rolling buffer.
    recordedSamples.insert(
        recordedSamples.end(),
        rollingBuffer.begin(),
        rollingBuffer.end()
    );



    silenceDuration = 0.0f;
}


// -------------------------------------------------
// Process microphone audio
// -------------------------------------------------

void SpeechRecognizer::processAudioChunk(
    const AudioBuffer& chunk
)
{
    if (chunk.samples.empty())
        return;

    // Always keep the latest audio.
    updateRollingBuffer(chunk);
    // ---------------------------------------------
    // Debug: check whether microphone audio arrives
    // ---------------------------------------------

    //static int debugCounter = 0;

    //if (++debugCounter % 30 == 0)
    //{
    //    int maxAmplitude = 0;

    //    for (int16_t sample : chunk.samples)
    //    {
    //        int value = std::abs(
    //            static_cast<int>(sample)
    //        );

    //        if (value > maxAmplitude)
    //        {
    //            maxAmplitude = value;
    //        }
    //    }

    //    std::cout
    //        << "Mic chunk: "
    //        << chunk.samples.size()
    //        << " samples | rate: "
    //        << chunk.sampleRate
    //        << " | max amplitude: "
    //        << maxAmplitude
    //        << std::endl;
    //}


    // -------------------------------------------------
    // Waiting for wake word
    // -------------------------------------------------

    if (
        state ==RecognitionState::WaitingForWakeWord
        )
    {
        //updateRollingBuffer(chunk);

        bool detected =
            wakeWordDetector.process(
                chunk.samples.data(),
                chunk.samples.size(),
                chunk.sampleRate
            );

        if (detected)
        {
            notifyWakeWordDetected();
        }

        return;
    }


    // -------------------------------------------------
    // Recording user command
    // -------------------------------------------------

    if (state ==RecognitionState::RecordingCommand )
    {
        // Remember the audio format.
        currentSampleRate =
            chunk.sampleRate;

        currentChannels =
            chunk.channels;


        // Append this microphone chunk to the
        // current utterance.
        recordedSamples.insert(
            recordedSamples.end(),
            chunk.samples.begin(),
            chunk.samples.end()
        );


        // Check whether the user has stopped speaking.
        if (speechFinished(chunk))
        {
            std::cout
                << "Speech finished."
                << std::endl;


            AudioBuffer completeAudio{};

            completeAudio.samples =std::move(recordedSamples);

            completeAudio.sampleRate =currentSampleRate;

            completeAudio.channels =currentChannels;


            // Send the complete utterance to the
            // Whisper worker thread.
            submitAudio(std::move(completeAudio));


            // Return to wake-word listening mode.
            state =RecognitionState::WaitingForWakeWord;

            silenceDuration = 0.0f;
        }
    }
}


// -------------------------------------------------
// Simple VAD / silence detection
// -------------------------------------------------

bool SpeechRecognizer::speechFinished(
    const AudioBuffer& chunk
)
{
    if (chunk.samples.empty())
        return false;


    // Calculate RMS amplitude.
    double sumSquares = 0.0;

    for (int16_t sample : chunk.samples)
    {
        float normalized =
            static_cast<float>(sample)
            / 32768.0f;

        sumSquares +=
            normalized * normalized;
    }


    float rms =
        static_cast<float>(
            std::sqrt(
                sumSquares /
                chunk.samples.size()
            )
            );


    // Calculate the duration represented by this chunk.
    float chunkDuration =
        static_cast<float>(
            chunk.samples.size()
            )
        /
        static_cast<float>(
            chunk.sampleRate
            * chunk.channels
            );


    // -------------------------------------------------
    // Silence
    // -------------------------------------------------

    if (rms < silenceThreshold)
    {
        silenceDuration +=
            chunkDuration;
    }

    // -------------------------------------------------
    // Speech detected again
    // -------------------------------------------------

    else
    {
        silenceDuration = 0.0f;
    }


    // User is considered finished after enough silence.
    return (
        silenceDuration >=
        silenceLimit
        );
}


// -------------------------------------------------
// Submit complete audio to Whisper queue
// -------------------------------------------------

void SpeechRecognizer::submitAudio(
    AudioBuffer audio
)
{
    if (audio.samples.empty())
        return;


    std::lock_guard<std::mutex> lock(
        audioMutex
    );


    audioQueue.push(
        std::move(audio)
    );
}


// -------------------------------------------------
// Whisper worker loop
// -------------------------------------------------

void SpeechRecognizer::workerLoop()
{
    while (running)
    {
        AudioBuffer audio{};

        bool hasAudio = false;


        // ---------------------------------------------
        // Try to get one complete utterance
        // ---------------------------------------------

        {
            std::lock_guard<std::mutex> lock(
                audioMutex
            );

            if (!audioQueue.empty())
            {
                audio =
                    std::move(
                        audioQueue.front()
                    );

                audioQueue.pop();

                hasAudio = true;
            }
        }


        // ---------------------------------------------
        // Nothing to recognize
        // ---------------------------------------------

        if (!hasAudio)
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(10)
            );

            continue;
        }


        //// ---------------------------------------------
        //// Whisper STT
        //// ---------------------------------------------

        //std::cout
        //    << "Sending audio to Whisper..."
        //    << std::endl;


        //std::string result =
        //    stt.transcribe(audio);


        //if (result.empty())
        //    continue;


        //// ---------------------------------------------
        //// Store recognized text
        //// ---------------------------------------------

        //{
        //    std::lock_guard<std::mutex> lock(
        //        textMutex
        //    );

        //    textQueue.push(
        //        std::move(result)
        //    );
        //}

        // ---------------------------------------------
		// chat part send audio to chat server and get audio reply
        // ---------------------------------------------
        std::cout
            << "Sending audio to chat server..."
            << std::endl;

        try
        {
            AudioBuffer reply = chatEngine.chat(audio);

            if (reply.samples.empty())
                continue;

            // Do not publish a result after shutdown was requested.
            if (!running)
                break;

            std::lock_guard<std::mutex> lock(replyMutex);

            replyQueue.push(std::move(reply));
        }
        catch (const std::exception& error)
        {
            std::cerr
                << "Chat request failed: "
                << error.what()
                << std::endl;
        }
    }
}

bool SpeechRecognizer::tryGetReply(AudioBuffer& audio)
{
    std::lock_guard<std::mutex> lock(replyMutex);

    if (replyQueue.empty())
        return false;

    audio = std::move(replyQueue.front());
    replyQueue.pop();

    return true;
}
// -------------------------------------------------
// Get recognized text
// -------------------------------------------------

bool SpeechRecognizer::tryGetText(
    std::string& text
)
{
    //std::lock_guard<std::mutex> lock(
    //    textMutex
    //);


    //if (textQueue.empty())
    //    return false;


    //text =
    //    std::move(
    //        textQueue.front()
    //    );


    //textQueue.pop();


    return true;
}


// -------------------------------------------------
// Get current recognition state
// -------------------------------------------------

RecognitionState SpeechRecognizer::getState() const
{
    return state;
}

void SpeechRecognizer::updateRollingBuffer(
    const AudioBuffer& chunk
)
{
    if (chunk.samples.empty())
        return;

    currentSampleRate =
        chunk.sampleRate;

    currentChannels =
        chunk.channels;


    // Add new samples.
    rollingBuffer.insert(
        rollingBuffer.end(),
        chunk.samples.begin(),
        chunk.samples.end()
    );


    // Maximum number of samples we want to keep.
    size_t maxSamples =
        static_cast<size_t>(
            currentSampleRate *
            currentChannels *
            rollingBufferSeconds
            );


    // Remove oldest samples.
    while (
        rollingBuffer.size() >
        maxSamples
        )
    {
        rollingBuffer.pop_front();
    }
}