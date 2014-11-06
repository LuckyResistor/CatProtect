//
// AudioPlayer
// (c)2014 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//

#include "AudioPlayer.h"


#include "SDCard.h"
#include "DacPort.h"


namespace lr {


/// The global instance of the audio player.
///
AudioPlayer audioPlayer;


bool AudioPlayer::initialize()
{
	// Initialize the DAC
	dacPort.initialize();
	
	// Shutdown the output
	dacPort.shutdown();
	
	// Initialize the SDCard	
	SDCard::Status status = sdCard.initialize();	
	if (status != SDCard::StatusReady) {
#ifdef AUDIOPLAYER_DEBUG
		Serial.println(String(F("SD Card Init Failure, error="))+String(sdCard.error()));
		Serial.flush();
#endif
		return false;
	}

#ifdef AUDIOPLAYER_DEBUG
	Serial.println(String(F("SD Card Init Success.")));
	Serial.flush();
#endif

	// Maximum speed
	SPI.beginTransaction(SPISettings(32000000, MSBFIRST, SPI_MODE0));

	// Read the directory.
	status = sdCard.readDirectory();
	if (status != SDCard::StatusReady) {
#ifdef AUDIOPLAYER_DEBUG
		Serial.println(String(F("SD Card read directory failure, error="))+String(sdCard.error()));
		Serial.flush();
#endif
		return false;
	}

	SPI.endTransaction();

	return true;
}


bool AudioPlayer::play(const char *fileName)
{
	const SDCard::DirectoryEntry *entry = sdCard.findFile(fileName);
	if (entry != 0) {
		return play(entry->startBlock, entry->fileSize/2);
	} else {
		return false;
	}
}


bool AudioPlayer::play(uint32_t startBlock, uint32_t sampleCount)
{
	SDCard::Status status;

	// Maximum speed
	SPI.beginTransaction(SPISettings(32000000, MSBFIRST, SPI_MODE0));
	
	// Wait until we can start a read.
	while((status = sdCard.startMultiRead(startBlock)) == SDCard::StatusWait) {
		delayMicroseconds(1);
	}
	
	// Check for any errors.
	if (status != SDCard::StatusReady) {
#ifdef AUDIOPLAYER_DEBUG
		Serial.println(String(F("Start Read Failure, error="))+String(sdCard.error()));
		Serial.flush();
#endif
		SPI.endTransaction();
		return false;
	}

	// Create a buffer with a given number of samples.
	const uint16_t bufferSize = 0x100; // the size of the buffer.
	uint16_t sampleBuffer[bufferSize]; // The buffer (created on the stack).

	// Fill the initial audio buffer.
	sdCard.startFastRead();
	for (uint16_t sampleIndex = 0; sampleIndex < bufferSize; ) {
		uint8_t* const bufferPointer =
			reinterpret_cast<uint8_t*>(&sampleBuffer[sampleIndex]);
		status = sdCard.readFast4(bufferPointer);
		if (status == SDCard::StatusReady) {
			sampleIndex += 2;
		} else if (status == SDCard::StatusError) {
			SPI.endTransaction();
			return false;
		}
	}

	// Initialize the Timer
	uint8_t oldSREG = SREG;				
  	cli();
	TCCR1A = 0;
	// no pre-scaling, use ICR1 as TOP
	TCCR1B = _BV(CS10)|_BV(WGM13);
	// Set the TOP value.
	uint16_t timerTop = (F_CPU / 2 / 22050); // number of clocks for 22.5 kHz
	ICR1 = timerTop;
	TIMSK1 = 0; // no interrupts from timer
	SREG = oldSREG;

	// Variables to play the sound.
	uint32_t currentSample = 0; // The absolute sample position
	// The mask to get the sample buffer from the current sample value.
	const uint32_t sampleBufferMask = 0x000000ff;
	// The number of buffered samples. (start at 100%)
	uint16_t bufferedSamples = 0x100;
	// The DAC value, used for fade out.
	uint16_t dacValue;
		
	// Fade in
	for (uint16_t v = 0; v < 0x0800; v += 0x10) {
		dacPort.setValue(v);
		dacPort.pushValue();
		delayMicroseconds(100);
	}
		
	// Main loop, until sample count reached.
	for (;;) {

		// 1. Wait for the timer, and push the DAC register.
		// This is done at the begin to play the samples as close to the 
		// sample frequency as possible.  
		while ((TIFR1 & _BV(TOV1)) == 0) {
		}
		dacPort.pushValue(); // Set the DAC output.
		TIFR1 |= _BV(TOV1); // reset the timer flag.

		// 2. Read the next sample and write it to the DAC.
		if (bufferedSamples > 0) { // Check if we have buffered samples.
			const uint16_t sample = sampleBuffer[(currentSample & sampleBufferMask)];
			dacValue = (sample >> 4);
			dacPort.setValue(dacValue);
			// Move the current sample pointer.
			--bufferedSamples;
			// 3. Increase the sample counter and check for the end.
			if (++currentSample > sampleCount) {
				break;
			}
		}
		
		// 4. Refill the sample buffer if necessary and possible
		const uint16_t currentReadPos = (currentSample & sampleBufferMask);
		const uint16_t currentWritePos = (currentReadPos + bufferedSamples) & sampleBufferMask;
		// Only write if there is space, and never at the end of the buffer.
		if (bufferedSamples < (bufferSize - 4)) {
			// try to read
			uint8_t* const writePointer =
				reinterpret_cast<uint8_t*>(&sampleBuffer[currentWritePos]);
			status = sdCard.readFast4(writePointer);
			if (status == SDCard::StatusError) {
				goto readError;
			} else if (status == SDCard::StatusReady) {
				bufferedSamples += 2; // (4 bytes)
			}
		}
	}

	// Fade out
	for (uint16_t v = 0x800; v > 0; v -= 0x10) {
		dacPort.setValue(v);
		dacPort.pushValue();
		delayMicroseconds(100);
	}
	
	// Stop the timer.
	TCCR1B &= ~(_BV(CS10)|_BV(CS11)|_BV(CS12));          

	// Stop reading from the SD Card.
	sdCard.stopRead();

	// Shutdown the output
	dacPort.shutdown();

	// End SPI transaction.
	SPI.endTransaction();

	return true; // success

readError:
	// Stop the timer.
	TCCR1B &= ~(_BV(CS10)|_BV(CS11)|_BV(CS12));          

	// Shutdown the output
	dacPort.shutdown();

	// End SPI transaction.
	SPI.endTransaction();

	return false; // error
}



}
