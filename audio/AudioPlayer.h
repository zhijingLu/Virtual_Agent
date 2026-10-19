#pragma once
#include <string>
#include "AudioBuffer.h"
class AudioPlayer
{
public:
	AudioPlayer();
	~AudioPlayer();
    // Play PCM audio directly from memory.
	bool play(const AudioBuffer& buffer);
private:
    // Forward declaration :
    // This only tells the compiler that
    // AudioPlayer contains a struct called Impl.
    //
    // The compiler does not know yet what members
    // are inside Impl.
    //
    // The advantage is that AudioPlayer.h does not
    // need to include miniaudio.h and does not need
    // to know what ma_engine is.
    //
    // The complete definition of Impl will be placed
    // inside AudioPlayer.cpp.
	struct Impl;

    // Pointer to the implementation object.
    //
    // This works even though Impl is not fully defined here,
    // because the compiler already knows the size of a pointer.
    //
    // This design pattern is called PImpl:
    // Pointer to Implementation.
    //
    // Conceptually:
    //
    // AudioPlayer
    //      |
    //      v
    //    Impl*
    //      |
    //      v
    //  ma_engine
    //
    // AudioPlayer exposes only its public interface,
    // while miniaudio-specific implementation details
    // are hidden inside Impl.
	Impl* impl;
};