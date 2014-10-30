//
// LEDController
// (c)2014 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//
#include "LEDController.h"


namespace lr {


LEDController::LEDController()
	: _blinkTimer(), _enabled(false), _color(Red), _state(Off)
{
}


LEDController::~LEDController()
{
}


void LEDController::setup()
{
	// Set the status LED outputs
	pinMode(RED_STATUS_PIN, OUTPUT);
	pinMode(GREEN_STATUS_PIN, OUTPUT);
	digitalWrite(RED_STATUS_PIN, LOW);
	digitalWrite(GREEN_STATUS_PIN, LOW);
}


void LEDController::loop(unsigned long currentTime)
{
	if (_blinkTimer.check(currentTime)) {
		onTimer();
	}
	if (_color == Orange && _enabled) {
		if ((currentTime & 0x07) < 4) {
			digitalWrite(RED_STATUS_PIN, HIGH);
			digitalWrite(GREEN_STATUS_PIN, LOW);
		} else {
			digitalWrite(RED_STATUS_PIN, LOW);
			digitalWrite(GREEN_STATUS_PIN, HIGH);
		}
	}
}


void LEDController::setState(Color color, State state)
{
	if (_color != color || _state != state) {
		_color = color;
		_state = state;
		switch(state) {
		case Off:
			disable();
			_blinkTimer.stop();
			break;
		case On:
			enable();
			_blinkTimer.stop();
			break;
		case BlinkSlow:
			enable();
			_blinkTimer.start(500, millis());				
			break;
		case BlinkFast:
			enable();
			_blinkTimer.start(250, millis());				
			break;
		case FlashVerySlow:
			disable();
			_blinkTimer.start(10000, millis());
		}
	}		
}


/// This method is called if the timer "fires".
///
void LEDController::onTimer()
{
	if (_state != FlashVerySlow) {
		if (_enabled) {
			disable();
		} else {
			enable();
		}
	} else {
		if (_enabled) {
			disable();
			_blinkTimer.start(10000, millis());
		} else {
			enable();
			_blinkTimer.start(25, millis());
		}
	}
}


/// Enable the LED with the selected color
///
void LEDController::enable()
{
	_enabled = true;
	switch (_color) {
	case Red:
		digitalWrite(RED_STATUS_PIN, HIGH);
		digitalWrite(GREEN_STATUS_PIN, LOW);
		break;
	case Green:
		digitalWrite(RED_STATUS_PIN, LOW);
		digitalWrite(GREEN_STATUS_PIN, HIGH);
		break;
	case Orange:
		// This is controlled in the Loop
		break;
	default:
		break;
	}
}

	
/// Diable the LED
///
void LEDController::disable()
{
	_enabled = false;
	digitalWrite(RED_STATUS_PIN, LOW);
	digitalWrite(GREEN_STATUS_PIN, LOW);
}


}

