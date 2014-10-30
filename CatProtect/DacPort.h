#pragma once
//
// The DAC Port
// ---------------
// (c)2014 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//
// Pushing values to the DAC.
//
// This class is made for the MCP4821 chip.
// The chip is connected to this pins:
//   Arduino <=====> MCP4821
//   Pin 2   <-----> Pin 2
//   Pin 3   <-----> Pin 3
//   Pin 4   <-----> Pin 4
//   Pin 5   <-----> Pin 5
//


#include <stdint.h>


namespace lr {


/// A software implementation to access the DAC port
///
class DacPort
{
public:
  /// Initialize everything for the DAC
  ///
  void initialize();

  /// Set a value to the DAC
  ///
  void setValue(uint16_t value);
  
  /// Push the value to the output
  ///
  void pushValue();
  
  /// Shutdown the DAC
  ///
  void shutdown();
};


/// The global instance of the DAC port
///
extern DacPort dacPort;


}

