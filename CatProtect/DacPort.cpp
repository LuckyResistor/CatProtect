//
// The DAC Port
// ---------------
// (c)2014 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//

#include "DacPort.h"


#include <SPI.h>

#include <avr/io.h>


// To make this SPI transfer as fast as possible, I use preprocessor macros
// to build the whole transfer, manipulating the registers directly.
// The compiler will produce highly optimized code, which IMO can not
// made faster.
//
// To change the pins for the output, you have to lookup the correct
// ports and pins in the schema for your Arduino board.

// Chip Select: Pin 2
#define DAC_CS_PORT PORTD
#define DAC_CS PIND2

// Clock: Pin 3
#define DAC_CLK_PORT PORTD
#define DAC_CLK PIND3

// Data In: Pin 4
#define DAC_DI_PORT PORTD
#define DAC_DI PIND4

// Latch: Pin 5
#define DAC_LATCH_PORT PORTD
#define DAC_LATCH PIND5

// This are helper macros to create the actions from the ports and pins above.
#define dacSelect() DAC_CS_PORT &= ~_BV(DAC_CS);
#define dacUnselect() DAC_CS_PORT |= _BV(DAC_CS);
#define dacLatchUp() DAC_LATCH_PORT |= _BV(DAC_LATCH);
#define dacLatchDown() DAC_LATCH_PORT &= ~_BV(DAC_LATCH);
#define dacClockUp() DAC_CLK_PORT |= _BV(DAC_CLK);
#define dacClockDown() DAC_CLK_PORT &= ~_BV(DAC_CLK);
#define dacClockPulse() dacClockUp();dacClockDown();
#define dacDataUp() DAC_DI_PORT |= _BV(DAC_DI);
#define dacDataDown() DAC_DI_PORT &= ~_BV(DAC_DI);

// This will send one bit to the chip.
#define dacSendBit(bit) if (value & bit) { dacDataUp() } else { dacDataDown() }; dacClockPulse();


namespace lr {


/// The global instance of the DAC port
///
DacPort dacPort;


void DacPort::initialize()
{
  // Set all ports to output
  pinMode(2, OUTPUT); 
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  
  // Set the outputs to the initial states
  dacUnselect();
  dacClockDown();
  dacDataDown();
  dacLatchUp();
}


// This generates the fastest possible machine code.
// Sad the CPU lacks some conditional bit set/clear mechanism which
// causes a lot of jumps.
//
void DacPort::setValue(uint16_t value)
{
  dacSelect(); // select the chip
  // Send the header
  dacDataDown(); // bit 15, 0 = Write to DAC register  
  dacClockPulse();
  dacClockPulse(); // bit 14, don't care.
  dacClockPulse();
  dacDataUp(); // bit 13, 1 = 1x Gain, 0 = 2x Gain
  dacClockPulse(); // bit 12, 1 = Enabled.
  // Send the data bits.
  dacSendBit(_BV(11));
  dacSendBit(_BV(10));
  dacSendBit(_BV(9));
  dacSendBit(_BV(8));
  dacSendBit(_BV(7));
  dacSendBit(_BV(6));
  dacSendBit(_BV(5));
  dacSendBit(_BV(4));
  dacSendBit(_BV(3));
  dacSendBit(_BV(2));
  dacSendBit(_BV(1));
  dacSendBit(_BV(0));
  dacUnselect(); // unselect the chip.
}


void DacPort::pushValue()
{
  // Push the value into the DAC
  dacLatchDown();
  dacLatchUp();
}


void DacPort::shutdown()
{
  dacSelect(); // select the chip
  // Send the header
  dacDataDown(); // bit 15, 0 = Write to DAC register  
  dacClockPulse();
  dacClockPulse(); // bit 14, don't care.
  dacClockPulse();
  dacClockPulse(); // bit 12, 0 = Disabled.
  // Send the data bits.
  dacClockPulse(); // bit 11
  dacClockPulse(); // 10
  dacClockPulse(); // 9
  dacClockPulse(); // 8
  dacClockPulse(); // 7
  dacClockPulse(); // 6
  dacClockPulse(); // 5
  dacClockPulse(); // 4
  dacClockPulse(); // 3
  dacClockPulse(); // 2
  dacClockPulse(); // 1
  dacClockPulse(); // 0
  dacUnselect(); // unselect the chip.
  // Push the value into the DAC
  dacLatchDown();
  dacLatchUp();
}


}


