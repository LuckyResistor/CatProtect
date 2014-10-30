#pragma once
//
// A SD-Card Access library for asynchronous timed access in timer interrupts
// --------------------------------------------------------------------------
// (c)2014 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//
//
// I developed this library to allow perfectly timed audio output, which
// requires that the data from the SD Card is loaded in the timed interrupt
// which plays the samples.
//
// This library assumes the chip select for the SD-Card is on Pin 10.
// The library is tested with the AdaFruit Data Logging Shield.
//


#include <SPI.h>


/// Enable debugging via Serial. You have to setup the serial port before calling initialize().
///
//#define SDCARD_DEBUG

/// Use the SPI transactions for all read calls.
/// If not defined, it is assumed one huge SPI.transaction for the whole read process.
/// Only the CS signal is handled.
///
//#define SDCARD_USE_SPI_TRANSACTIONS 


namespace lr {


/// The SD Card access class for asynchronous SD Card access.
///
class SDCard
{
public:
	/// The mode of operation.
	///
	enum ModeFlags : uint8_t {
		/// Use SPI transactions.
		TransactionMode = 0x01, 
	};

	/// Errors
	///
	enum Error : uint8_t {
		NoError = 0,
		Error_TimeOut = 1,
		Error_SendIfCondFailed = 2,
		Error_ReadOCRFailed = 3,
		Error_SetBlockLengthFailed = 4,
		Error_ReadSingleBlockFailed = 5,
		Error_ReadFailed = 6,
		Error_UnknownMagic = 7,
	};
	
	/// The status of a command.
	///
	enum Status : uint8_t {
		StatusReady = 0, ///< The call was successful and the card is ready.
		StatusWait = 1, ///< You have to wait, the card is busy
		StatusError = 2, ///< There was an error. Check error() for details.
		StatusEndOfBlock = 3, ///< Reached the end of the block.
	};

	/// A single directory entry.
	///
	struct DirectoryEntry {
		uint32_t startBlock; ///< The start block of the file in blocks.
		uint32_t fileSize; ///< The size of the file in bytes.
		char *fileName; ///< Null terminated filename ascii.
		DirectoryEntry *next; ///< The next entry, or a null pointer at the end.
	};
	
public:
	/// Initialize the library and the SD-Card.
	/// This call needs some time until the SD-Card is ready for read. It
	/// should be placed in the setup() method.
	///
	/// @return StatusReady on succes, StatusError on any error.
	///
	Status initialize(uint8_t mode = 0);

	/// Read the SD Card Directory in HCDI format
	///
	/// @return StatusReady on success, StatusError on any error.
	///
	Status readDirectory();

	/// Find a file with the given name
	///
	/// @return The found directory entry, or 0 if no such file was found.
	///
	const DirectoryEntry* findFile(const char *fileName);

	/// Start reading the given block.
	///
	/// @param block The block in (512 byte blocks).
	/// @return StatusWait = call again, StatusError = there was an error,
	///    StatusReady = reading of the block has started, call readData().
	///
	Status startRead(uint32_t block);

	/// Start reading from given block until stopRead() is called.
	///
	/// @param startBlock The first block in (512 byte blocks).
	/// @return StatusWait = call again, StatusError = there was an error,
	///    StatusReady = reading of the block has started, call readData().
	///
	Status startMultiRead(uint32_t startBlock);

	/// Read data if ready.
	///
	/// @param buffer The buffer to read the data into.
	/// @param byteCount in: The number of bytes to read, out: the actual number of read bytes.
	/// @return StatusReady on success, StatusError is there was an error,
	///     StatusEndOfBlock if the end of the block was reached.
	///
	Status readData(uint8_t *buffer, uint16_t *byteCount);

	/// Start the fast reading.
	///
	void startFastRead();

	/// Super fast data multi read.
	///
	/// For extreme situations. It always reads 4 bytes to the given address.
	/// 
	/// @param buffer A pointer to the buffer to write 4 bytes.
	/// @return StatusWait if no bytes were written, StatusReady if bytes were written.
	///
	Status readFast4(uint8_t *buffer);

	/// End reading data.
	///
	/// You have to call this method in any case. This is a blocking call and
	/// and can take a while.
	///
	/// @return StatusReady = success, block read ended.
	/// 
	Status stopRead();

	/// Get the last error
	///
	Error error();
};

/// The global instance to access the SD Card
///
extern SDCard sdCard;

}

