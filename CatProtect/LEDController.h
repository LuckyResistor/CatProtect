#pragma once
//
// LEDController
// (c)2014 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//


#include "Timer.h"

#include <Arduino.h>


namespace lr {


/// This class controls the Signal LED
///
class LEDController
{
private:
	/// The pin where the red part of the LED is connected.
	///
	const int RED_STATUS_PIN = 6;
	
	/// The pin where the green part of the LED is connected.
	///
	const int GREEN_STATUS_PIN = 7;
	
public:
	/// The color for the LED
	///
	enum Color : uint8_t {
		Red, ///< Red
		Green, ///< Green
		Orange ///< Orange (both)
	};
	
	/// The states for the LED
	///
	enum State : uint8_t {
		Off, ///< Off
		On, ///< On
		BlinkSlow, ///< Blink with 500ms
		BlinkFast, ///< Blink with 250ms
		FlashVerySlow, ///< Short flash to indicate the device is running.
	};
	
public:
	/// ctor
	///
	LEDController();
	
	/// dtor
	///
	~LEDController();
	
public:
	/// Call this method in the setup() method.
	///
	void setup();

	/// Call this method in each loop.
	///
	/// @param The current time to the loop.
	///
	void loop(unsigned long currentTime);

	/// Set the LED state.
	///
	/// @param color The color for the LED
	/// @param state The state for the LED
	///
	void setState(Color color, State state);
	
private:
	void onTimer();
	void enable();
	void disable();
	
private:
	State _state; ///< The current selected state.
	Color _color; ///< The current selected color.
	Timer _blinkTimer; ///< The timer used to create the blink state.
	bool _enabled; ///< The state variable to let the LED blink.
};


}

