//
// A SD-Card Access library for asynchronous timed access in timer interrupts
// --------------------------------------------------------------------------
// (c)2014 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//
#include "SDCard.h"


// define macros to send text to serial if debug is activated.
#ifdef SDCARD_DEBUG
#define SDC_DEBUG_PRINT(text) Serial.print(text); Serial.flush();
#define SDC_DEBUG_PRINTLN(text) Serial.println(text); Serial.flush();
#else
#define SDC_DEBUG_PRINT(text)
#define SDC_DEBUG_PRINTLN(text)
#endif

// The pin and port for the chip select of the SD-Card.
#define SDCARD_CSPINNUM 10
#define SDCARD_CSPORT PORTB 
#define SDCARD_CSPIN PINB2 


namespace lr {
		

/// The state
///
struct SDCardState {

	/// The timeout to initialize the SD Card in ms
	///
	const uint16_t initTimeout = 2000;

	/// The block size.
	///
	const uint16_t blockSize = 512;

	/// The SD Card type
	///
	enum CardType : uint8_t {
		CardTypeSD1, //< Standard capacity V1 SD card
		CardTypeSD2, //< Standard capacity V2 SD card
		CardTypeSDHC, //< High Capacity SD card
	};

	/// The response type.
	///
	enum ResponseType : uint16_t {
		ResponseMask = 0x00c0, ///< The mask for the responses.
		Response1    = 0 << 6, ///< One byte response.
		Response3    = 1 << 6, ///< One byte plus 32bit value
		Response7    = 1 << 6, ///< One byte plus 32bit value (supported voltage)
	};

	/// The command to send.
	///
	enum Command : uint16_t {
		Cmd_GoIdleState          = 0 | Response1, ///< Go idle state.
		Cmd_SendIfCond           = 8 | Response7, ///< Verify SD Memory Card interface operating condition.
		Cmd_StopTransmission     = 12 | Response1, ///< Stop reading blocks.
		Cmd_SetBlockLenght       = 16 | Response1, ///< Set the block length
		Cmd_ReadSingleBlock      = 17 | Response1, ///< Read one block.
		Cmd_ReadMultiBlock       = 18 | Response1, ///< Read multiple blocks.
		Cmd_ApplicationCommand   = 37 | Response1, ///< Escape for application specific command.
		Cmd_ReadOCR              = 58 | Response3, ///< Retrieve the OCR register.
		ACmd_Flag                = 0x100, ///< The flag for app commands.
		ACmd_SendOpCond          = 41 | ACmd_Flag | Response1, ///< Sends host capacity support information and activates the card's initialization process. 
	};

	/// The state of the read command
	///
	enum ReadState : uint8_t {
		ReadStateWait = 0, ///< Waiting for the start of the data.
		ReadStateReadData = 1, ///< In the middle of data reading.
		ReadStateReadCRC = 2, ///< Reached end of block, read CRC bytes.
		ReadStateEnd = 3, ///< The read process has ended (end of block or error).
	};

	/// The block read mode
	///
	enum ReadMode : uint8_t {
		ReadModeSingleBlock = 0, ///< Read just a single block.
		ReadModeMultipleBlocks = 1, ///< Read multiple blocks until stop is sent.
	};

	/// Reponses and flags.
	///
	const uint8_t R1_IdleState = 0x01; ///< The state if the card is idle.
	const uint8_t R1_IllegalCommand = 0x04; ///< The flag for an illegal command.
	const uint8_t R1_ReadyState = 0x00; ///< The ready state.
	const uint8_t BlockDataStart = 0xfe; ///< Byte to indicate the block data will start.

	/// The mode flags
	///
	uint8_t modeFlags = 0;

	/// The last error
	///
	SDCard::Error error = SDCard::NoError;
	
	/// The SPI settings for SD data transfers.
	///
	SPISettings spiSettings;
	
	/// The card type.
	///
	CardType cardType = CardTypeSD1;
	
	/// The byte count in the current block.
	///
	uint16_t blockByteCount;
	
	/// The state of the read command.
	///
	ReadState blockReadState;
	
	/// The mode for the block read command.
	///
	ReadMode blockReadMode;

	/// The directory.
	///
	SDCard::DirectoryEntry *directoryEntry = 0;


	/// Only chip select.
	///
	inline void onlyChipSelectBegin()
	{
		SDCARD_CSPORT &= ~_BV(SDCARD_CSPIN);
	}

	/// Only chip select.
	///
	inline void onlyChipSelectEnd()
	{
		SDCARD_CSPORT |= _BV(SDCARD_CSPIN);
	}

	/// Chip select low
	///
	inline void chipSelectBegin()
	{
#ifdef SDCARD_USE_SPI_TRANSACTIONS
		SPI.beginTransaction(spiSettings);
#endif
		SDCARD_CSPORT &= ~_BV(SDCARD_CSPIN);
	}

	/// Chip select high
	///
	inline void chipSelectEnd()
	{
		SDCARD_CSPORT |= _BV(SDCARD_CSPIN);
#ifdef SDCARD_USE_SPI_TRANSACTIONS		
		SPI.endTransaction();
#endif
	}

	/// Send a byte over the SPI bus
	///
	inline void spiSend(uint8_t value)
	{
		SPI.transfer(value);
	}

	/// Receive a byte from the SPI bus.
	///
	inline uint8_t spiReceive()
	{
		return SPI.transfer(0xff);
	}

	/// Skip a number of bytes from the SPI bus.
	///
	inline void spiSkip(uint8_t count) {
		for (uint8_t i = 0; i < count; ++i) {
			spiReceive();
		}
	}

	/// Wait while sending clocks on the SPI bus
	///
	inline void spiWait(uint8_t count)
	{
		for (uint8_t i = 0; i < count; ++i) {
			spiSend(0xff);
		}
	}

	/// Wait until the chip isn't busy anymore
	///
	inline bool waitUntilReady(uint16_t timeoutMillis) {
		uint16_t startTime = millis();
		do {
			const uint8_t result = spiReceive();
			if (result == 0xff) {
				return true;
			}
		} while ((static_cast<uint16_t>(millis()) - startTime) < timeoutMillis);
		return false;
	}

	/// Send a command to the SD card synchronous.
	///
	/// @param command The command to send.
	/// @param argument A pointer to the 4 byte buffer for the argument.
	/// @param responseValue A ponter to the variable where to save the 32bit value for R3/R7 responses. 0 = ignore.
	///
	inline uint8_t sendCommand(Command command, uint32_t argument, uint32_t *responseValue = 0)
	{
		uint8_t result;
		uint8_t responseValues[4];
		// Check if this is an app command
		if ((command & ACmd_Flag) != 0) {
			// For app commands, send a command 55 first.
			spiSend(55 | 0x40);
			spiSend(0);
			spiSend(0);
			spiSend(0);
			spiSend(0);
			for (uint8_t i = 0; ((result = spiReceive()) & 0x80) && i < 0x10; ++i);
			// now send the app command.
		}
		// Send the command.
		spiSend((command & 0x3f) | 0x40);
		spiSend(argument >> 24);
		spiSend(argument >> 16);
		spiSend(argument >> 8);
		spiSend(argument);
		uint8_t crc = 0xff;
		if (command == Cmd_GoIdleState) {
			crc = 0x95;
		} else if (command == Cmd_SendIfCond) {
			crc = 0x87;
		}
		spiSend(crc);
		// There can be 0-8 no command return values until the return is received.
		for (uint8_t i = 0; ((result = spiReceive()) & 0x80) && i < 0x10; ++i);
		const uint8_t response = (command & ResponseMask);
		if (response != Response1) {
			// Read the next 4 bytes
			for (uint8_t i = 0; i < 4; ++i) {
				responseValues[i] = spiReceive();
			}
			// If there is a pointer, assign the response value (MSB).
			if (responseValue != 0) {
				*responseValue =
					(static_cast<uint32_t>(responseValues[0]) << 24) | 
					(static_cast<uint32_t>(responseValues[1]) << 16) | 
					(static_cast<uint32_t>(responseValues[2]) << 8) |
					static_cast<uint32_t>(responseValues[3]);
			}
		}
		return result;
	}

	/// Wait until the card is ready, then send the command.
	///
	/// Same parameters as sendCommand().
	///
	inline uint8_t waitAndSendCommand(Command command, uint32_t argument, uint32_t *responseValue = 0)
	{
		waitUntilReady(300);
		return sendCommand(command, argument, responseValue);
	}


	/// Initialize the SD Card
	///
	inline SDCard::Status initialize(uint8_t mode)
	{
		// The mode flags
		modeFlags = mode;
	
		// Keep the start time to detect time-outs.
		uint16_t startTime = static_cast<uint16_t>(millis());
		uint32_t argument = 0;
		uint8_t result = 0;
		uint32_t responseValue = 0;

		// Initialize the SPI library
		pinMode(SDCARD_CSPINNUM, OUTPUT);
		onlyChipSelectEnd();
		SPI.begin();
	
		// Speed should be <400kHz for the initialization.
		spiSettings = SPISettings(250000, MSBFIRST, SPI_MODE0);
		SPI.beginTransaction(spiSettings);
		// Send >74 clocks to prepare the card.
		onlyChipSelectBegin();
		spiWait(100);
		onlyChipSelectEnd();
		spiWait(2);

		onlyChipSelectBegin();
		// Send the CMD0
		while (waitAndSendCommand(Cmd_GoIdleState, 0) != R1_IdleState) {
			if ((static_cast<uint16_t>(millis())-startTime) > initTimeout) {
				error = SDCard::Error_TimeOut;
				goto initFail;
			}
		}
	
		// Try to send CMD8 to check SD Card version.
		result = waitAndSendCommand(Cmd_SendIfCond, 0x01aa, &responseValue);
		if ((result & R1_IllegalCommand) != 0) {
			cardType = CardTypeSD1;
		} else {
			if ((responseValue & 0x000000ff) != 0x000000aa) {
				error = SDCard::Error_SendIfCondFailed;
				goto initFail;
			}
			cardType = CardTypeSD2;
		}
	
		// Send the ACMD41 to initialize the card.
		if (cardType == CardTypeSD2) {
			argument = 0x40000000; // Enable HCS Flag
		} else {
			argument = 0x00000000;
		}
		while (waitAndSendCommand(ACmd_SendOpCond, argument) != R1_ReadyState) {
			if ((static_cast<uint16_t>(millis())-startTime) > initTimeout) {
				error = SDCard::Error_TimeOut;
				goto initFail;
			}
		}
	
		// Check if we have a SDHC card
		if (cardType == CardTypeSD2) {
			if (waitAndSendCommand(Cmd_ReadOCR, 0, &responseValue) != R1_ReadyState) {
				error = SDCard::Error_ReadOCRFailed;
				goto initFail;
			}
			// Check "Card Capacity Status (CCS)", bit 30 which is only valid
			// if the "Card power up status bit", bit 31 is set.
			if ((responseValue & 0xc0000000) != 0) {
				cardType = CardTypeSDHC;
			}
		}
	
		// Set the block size to 512byte.
		if (waitAndSendCommand(Cmd_SetBlockLenght, blockSize) != R1_ReadyState) {
			error = SDCard::Error_SetBlockLengthFailed;
			goto initFail;
		}
	
		onlyChipSelectEnd();
		SPI.endTransaction();
	
		// now rise the clock speed to maximum.
		spiSettings = SPISettings(32000000, MSBFIRST, SPI_MODE0);

		// Debug output
#ifdef SDCARD_DEBUG
		SDC_DEBUG_PRINT(String(F("Card type ")));
		switch (cardType) {
			case CardTypeSD1: SDC_DEBUG_PRINTLN(String(F("SD1"))); break;
			case CardTypeSD2: SDC_DEBUG_PRINTLN(String(F("SD2"))); break;
			case CardTypeSDHC: SDC_DEBUG_PRINTLN(String(F("SDHC"))); break;
		}
#endif
	
		// Make sure we can use SPI in the timer interrupt.
		SPI.usingInterrupt(255); // Using from timer.
		return SDCard::StatusReady;

initFail:
		onlyChipSelectEnd();
		SPI.endTransaction();
		return SDCard::StatusError;
	}
	
	inline SDCard::Status startRead(uint32_t block)
	{
		// Begin a transaction.
		chipSelectBegin();
		uint8_t result = spiReceive();
		if (result != 0xff) { // Check if the chip is idle.
			chipSelectEnd();
			return SDCard::StatusWait; // The chip isn't ready yet.
		}
	
		// Ok, the chip is ready send the read command.
		result = sendCommand(Cmd_ReadSingleBlock, block);
		if (result != R1_ReadyState) {
			error = SDCard::Error_ReadSingleBlockFailed;
			chipSelectEnd();
			return SDCard::StatusError;
		}
		// Reset the block byte count
		blockByteCount = 0;
		blockReadState = ReadStateWait;
		blockReadMode = ReadModeSingleBlock;
		chipSelectEnd();
		return SDCard::StatusReady;	
	}
	
	inline SDCard::Status startMultiRead(uint32_t startBlock)
	{
		// Begin a transaction.
		chipSelectBegin();
		uint8_t result = spiReceive();
		if (result != 0xff) { // Check if the chip is idle.
			chipSelectEnd();
			return SDCard::StatusWait; // The chip isn't ready yet.
		}
	
		// Ok, the chip is ready send the read command.
		result = sendCommand(Cmd_ReadMultiBlock, startBlock);
		if (result != R1_ReadyState) {
			error = SDCard::Error_ReadSingleBlockFailed;
			chipSelectEnd();
			return SDCard::StatusError;
		}
		// Reset the block byte count
		blockByteCount = 0;
		blockReadState = ReadStateWait;
		blockReadMode = ReadModeMultipleBlocks;
		chipSelectEnd();
		return SDCard::StatusReady;	
	}

	SDCard::Status readData(uint8_t *buffer, uint16_t *byteCount)
	{
		// variables
		uint8_t result;
		SDCard::Status status = SDCard::StatusReady;
		uint16_t bytesToRead;

		// Start the read.
		chipSelectBegin();
		switch (blockReadState) {
		case ReadStateWait:
			result = spiReceive();
			if (result == 0xff) {
				status = SDCard::StatusWait;
				break;
			} else if (result == BlockDataStart) {
				blockReadState = ReadStateReadData;
				// no break! continue with read data.
			} else {
				error = SDCard::Error_ReadFailed;
				blockReadState = ReadStateEnd;
				status = SDCard::StatusError; // Failed.
				break;
			}
		case ReadStateReadData:
			bytesToRead = min(blockSize - blockByteCount, *byteCount);
			for (uint16_t i = 0; i < bytesToRead; ++i) {
				buffer[i] = spiReceive();
			}
			*byteCount = bytesToRead;
			blockByteCount += bytesToRead;
			if (blockByteCount < blockSize) {
				break;
			}
			blockReadState = ReadStateReadCRC;
		case ReadStateReadCRC:
			spiSkip(2);
			blockByteCount = 0;
			if (blockReadMode == ReadModeSingleBlock) {
				blockReadState = ReadStateEnd;
				status = SDCard::StatusEndOfBlock;
			} else {
				blockReadState = ReadStateWait;
				status = SDCard::StatusWait;
			}
			break;
		case ReadStateEnd:
			status = SDCard::StatusEndOfBlock;
			break;
		}
		chipSelectEnd();
#ifdef SDCARD_DEBUG
		if (status == SDCard::StatusError) {
			SDC_DEBUG_PRINT(F("Read error, last byte = 0x"));
			SDC_DEBUG_PRINTLN(String(result, 16));
		}
#endif
		return status;
	}

	inline SDCard::Status readFast4(uint8_t *buffer)
	{
		uint8_t readByte;
		switch (blockReadState) {
		case ReadStateWait:
			readByte = spiReceive();
			if (readByte == 0xff) {
				return SDCard::StatusWait;
			} else if (readByte == BlockDataStart) {
				blockReadState = ReadStateReadData;
				return SDCard::StatusWait;
			} else {
				blockReadState = ReadStateEnd;
				chipSelectEnd();
				return SDCard::StatusError; // Failed.
			}
		case ReadStateReadData:
			buffer[0] = spiReceive();
			buffer[1] = spiReceive();
			buffer[2] = spiReceive();
			buffer[3] = spiReceive();
			blockByteCount += 4;
			if (blockByteCount >= blockSize) {
				blockReadState = ReadStateReadCRC;
			}
			return SDCard::StatusReady;
		case ReadStateReadCRC:
			spiSkip(2);
			blockByteCount = 0;
			blockReadState = ReadStateWait;
			return SDCard::StatusWait;
		case ReadStateEnd:
			return SDCard::StatusError; // Failed.
		}		
	}

	inline SDCard::Status stopRead()
	{
		if (blockReadMode == ReadModeSingleBlock) {
			if (blockReadState != ReadStateEnd) {
				// Make sure we read the rest of the data.
				uint16_t byteCount = blockSize;
				while (readData(0, &byteCount) != SDCard::StatusEndOfBlock) {
				}
			}
		} else {
			// Send Command 12 in a special way
			chipSelectBegin(); // If not already done			
			spiSend(Cmd_StopTransmission | 0x40);
			spiSend(0);
			spiSend(0);
			spiSend(0);
			spiSend(0);
			spiSend(0xff); // Fake CRC
			// Skip one byte
			spiSkip(1);
			uint8_t result;
			for (uint8_t i = 0; ((result = spiReceive()) & 0x80) && i < 0x10; ++i);
			if (result != R1_ReadyState) {
#ifdef SDCARD_DEBUG
				SDC_DEBUG_PRINT(F("Error on response, last byte = 0x"));
				SDC_DEBUG_PRINTLN(String(result, 16));
#endif
				return SDCard::StatusError;
			}
			waitUntilReady(300);
		}
		return SDCard::StatusReady;
	}
	
	inline SDCard::Status synchronousStartRead(uint32_t block)
	{
		SDCard::Status localStatus;
		do {
			localStatus = startRead(block);
		} while (localStatus == SDCard::StatusWait);
		return localStatus;
	}
	
	inline SDCard::Status synchronousReadData(uint8_t *buffer, uint16_t *byteCount)
	{
		SDCard::Status localStatus;
		do {
			localStatus = readData(buffer, byteCount);
		} while (localStatus == SDCard::StatusWait);
		return localStatus;
	}
	
	inline uint32_t getLittleEndianUInt32(const uint8_t *value) {
		uint32_t result = 0;
		result |= value[0];
		result |= static_cast<uint32_t>(value[1]) << 8;
		result |= static_cast<uint32_t>(value[2]) << 16;
		result |= static_cast<uint32_t>(value[3]) << 24;
		return result;		
	}
	
	inline SDCard::Status readDirectory()
	{
		SDCard::Status status;
		
		// Wait until the block 0 read command has started.
		if (synchronousStartRead(0) == SDCard::StatusError) {
			return SDCard::StatusError;
		}
		
		// The buffer to read the data.
		uint8_t buffer[9];
		memset(buffer, 0xaa, 9); 
		uint16_t byteCount = 4;
		if (synchronousReadData(buffer, &byteCount) == SDCard::StatusError) {
			return SDCard::StatusError;
		}
		
		// Check the magic.
		const char *magic = "HCDI";
		if (strncmp(magic, reinterpret_cast<char*>(buffer), 4) != 0) {
			error = SDCard::Error_UnknownMagic;
			return SDCard::StatusError;
		}
		
		// Read the directory.
		uint32_t startBlock;
		uint32_t fileSize;
		uint8_t stringLength;
		SDCard::DirectoryEntry *newEntry = 0;
		SDCard::DirectoryEntry *lastEntry = 0;		
		do {
			// Read the initial bytes.
			byteCount = 9; // 2x 32bit integer + 1 byte string length.
			if (readData(buffer, &byteCount) == SDCard::StatusError) {
				return SDCard::StatusError;
			}
			// Interpret the bytes (not portable).
			startBlock = getLittleEndianUInt32(buffer);
			fileSize = getLittleEndianUInt32(buffer + 4);
			stringLength = buffer[8];
			if (startBlock > 0) {
				newEntry = new SDCard::DirectoryEntry;
				newEntry->startBlock = startBlock;
				newEntry->fileSize = fileSize;
				newEntry->fileName = new char[stringLength+1];
				memset(newEntry->fileName, 0, stringLength+1);
				byteCount = stringLength;
				if (readData(reinterpret_cast<uint8_t*>(newEntry->fileName), &byteCount) == SDCard::StatusError) {
					return SDCard::StatusError;
				}
				newEntry->next = 0;
				if (lastEntry != 0) {
					lastEntry->next = newEntry;
				} else {
					directoryEntry = newEntry;
				}
				lastEntry = newEntry;
			}
		} while(startBlock > 0);
		
		// Success
		return SDCard::StatusReady;
	}
	
	const SDCard::DirectoryEntry* findFile(const char *fileName)
	{
		SDCard::DirectoryEntry *entry = directoryEntry;
		while (entry != 0) {
			if (strcmp(fileName, entry->fileName) == 0) {
				return entry;
			}
			entry = entry->next;
		}
		return 0;
	}
};


/// The global state for the SD Card class
///
SDCardState sdCardState;
	
/// The global instance for the SDCard class
///
SDCard sdCard;
	

SDCard::Status SDCard::initialize(uint8_t mode)
{	
	return sdCardState.initialize(mode);
}


SDCard::Status SDCard::readDirectory()
{
	return sdCardState.readDirectory();	
}


const SDCard::DirectoryEntry* SDCard::findFile(const char *fileName)
{
	return sdCardState.findFile(fileName);
}



SDCard::Status SDCard::startRead(uint32_t block)
{
	return sdCardState.startRead(block);
}


SDCard::Status SDCard::startMultiRead(uint32_t startBlock)
{
	return sdCardState.startMultiRead(startBlock);
}


SDCard::Status SDCard::readData(uint8_t *buffer, uint16_t *byteCount)
{
	return sdCardState.readData(buffer, byteCount);	
}


void SDCard::startFastRead()
{
	// Select the chip and start reading.
	sdCardState.chipSelectBegin();
}


SDCard::Status SDCard::readFast4(uint8_t *buffer)
{
	return sdCardState.readFast4(buffer);
}


SDCard::Status SDCard::stopRead()
{
	return sdCardState.stopRead();
}


SDCard::Error SDCard::error()
{
	return sdCardState.error;
}


} // end of namespace




