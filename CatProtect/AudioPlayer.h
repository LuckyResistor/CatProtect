#pragma once
//
// AudioPlayer
// (c)2014 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//


#include <stdint.h>


//#define AUDIOPLAYER_DEBUG


namespace lr {

/// This is the audio player.
///
/// It uses the custom SDCard class and the DacPort class to stream audio at 22.1kHz
/// from the SD-Card. It uses timer 1 to create the correct timing. The player 
/// reserves 512bytes for a buffer while playing the sound. This memory is released
/// at the end of the method.
///
class AudioPlayer
{
public:
	/// Initialize the Audio Player
	///
	bool initialize();

	/// Play samples from the given start block.
	///
	bool play(uint32_t startBlock, uint32_t sampleCount);
	
	/// Play a sample with a given name.
	///
	bool play(const char *fileName);
};

/// The global instance of the audio player.
///
extern AudioPlayer audioPlayer;

}





