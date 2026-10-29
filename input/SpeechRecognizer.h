#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "../audio/AudioBuffer.h"
//#include "../audio/STTEngine.h"
#include "../chat/ChatEngine.h"
#include <deque>
#include "WakeWordDetector.h"
enum class RecognitionState
{
    WaitingForWakeWord,
    RecordingCommand
};


class SpeechRecognizer
{
public:
    SpeechRecognizer();
    ~SpeechRecognizer();

    // Start / stop the Whisper worker thread.
    void start();
    void stop();

    // Called continuously by AudioRecorder.
    // Each call contains a small microphone PCM chunk.
    void processAudioChunk(const AudioBuffer& chunk );

    // Called when the wake-word detector detects
    // the configured keyword.
    void notifyWakeWordDetected();

    // Read recognized text from the main thread.
    bool tryGetText(std::string& text);
    // Read the reply audio from the main thread.
    bool tryGetReply(AudioBuffer& audio);

    RecognitionState getState() const;


private:
    // Whisper worker thread.
    void workerLoop();

    // Add one complete utterance to the Whisper queue.
    void submitAudio(AudioBuffer audio );

    // Detect whether the user has stopped speaking.
    bool speechFinished(const AudioBuffer& chunk);


private:

    WakeWordDetector wakeWordDetector;
    // -------------------------------------------------
    // Recognition state
    // -------------------------------------------------

    RecognitionState state =
        RecognitionState::WaitingForWakeWord;


    // -------------------------------------------------
    // Current recorded command
    // -------------------------------------------------

    std::vector<int16_t> recordedSamples;

    int currentSampleRate = 16000;
    int currentChannels = 1;


    // -------------------------------------------------
    // Simple silence detection
    // -------------------------------------------------

    float silenceDuration = 0.0f;

    // RMS threshold below which the chunk is treated
    // as silence.
    float silenceThreshold = 0.015f;

    // Stop recording after this amount of silence.
    float silenceLimit = 1.0f;


    // -------------------------------------------------
    // Whisper worker
    // -------------------------------------------------

    //STTEngine stt;
    ChatEngine chatEngine;
    std::thread workerThread;

    std::atomic<bool> running{ false };


    // -------------------------------------------------
    // Audio queue
    // -------------------------------------------------

    std::mutex audioMutex;

    std::queue<AudioBuffer> audioQueue;


    // -------------------------------------------------
    // Recognized text queue
    // -------------------------------------------------

    //std::mutex textMutex;
    //std::queue<std::string> textQueue;
    
	//rely audio queue
    std::mutex replyMutex;
    std::queue<AudioBuffer> replyQueue;
    

    // Rolling buffer:
    // always keeps the most recent microphone audio.
    std::deque<int16_t> rollingBuffer;

    // Keep about 1 second of previous audio.
    float rollingBufferSeconds = 1.0f;

	void updateRollingBuffer(const AudioBuffer& chunk);
 
};