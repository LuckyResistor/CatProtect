// 
// Cat Protector Project
// -------------------------------------------------------------------------------
// (c)2014 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//


#include <SPI.h>

#include "AudioPlayer.h"
#include "SDCard.h"
#include "Timer.h"
#include "LEDController.h"
#include "MotionSensor.h"


using namespace lr;


// Motion callback.
void onMotion(const unsigned long currentTime, MotionSensor::Status status);

/// The logic state.
enum LogicState : uint8_t {
	/// If the board is waiting for the sensor.
	WaitForSensor,
	/// If the board is idle
	IdleState,
	/// If the board is in error state, only the LED is controlled.
	ErrorState,
	/// If the board is in alarm state, the voice is played.
	AlarmState,
} logicState;


/// The controller for the LED
LEDController ledController; 

/// The motion sensor.
MotionSensor motionSensor;


void setup() {
	// Set the initial state.
	logicState = WaitForSensor;

	// setup the LED controller
	ledController.setup();

	// setup the motion sensor.
	motionSensor.setup();
	motionSensor.setCallback(&onMotion);

	// Initialize the serial interface
	Serial.begin(115200);
	Serial.println(F("Starting..."));
	
	// Initialize the audio player
	if (!audioPlayer.initialize()) {
		Serial.println(F("Error on initialize."));
		Serial.flush();
		signalError();
		return;
	}
	
	Serial.println(F("Success!"));
	Serial.flush();
}


void loop() {
	// Get the current time for this loop iteration.
	const unsigned long currentTime = millis();
	// Control the LED
	ledController.loop(currentTime);
	// Deactivate everything if there is an error.
	if (logicState != ErrorState) {
		// Check the motion sensor.
		motionSensor.loop(currentTime);
		// If the board goes into alarm state, play the sound.
		// This will block the loop, until the sound finishes.
		if (logicState == AlarmState) {
			if (!audioPlayer.play("voice.snd")) {
				Serial.println(F("Error on play."));
				Serial.flush();
				return;
			}
			// Go back into idle state after the sound.
			// The sensor might stay in alarm state for a while.
			logicState = IdleState;
		}
	}
}


void onMotion(const unsigned long currentTime, MotionSensor::Status status)
{
	if (status == MotionSensor::WaitStablilize) {
		Serial.println(F("Wait until the sensor is ready."));
		Serial.flush();
		ledController.setState(LEDController::Orange, LEDController::BlinkSlow);
	} else if (status == MotionSensor::Idle) {
		Serial.println(F("Sensor is in idle state."));
		Serial.flush();
		ledController.setState(LEDController::Green, LEDController::FlashVerySlow);
		logicState = IdleState; // Ready to observe.
	} else if (status == MotionSensor::Alarm) {
		Serial.println(F("Sensor alarm."));
		Serial.flush();
		ledController.setState(LEDController::Red, LEDController::On);
		logicState = AlarmState; // Activate the alarm and play a sound.
	}
}


/// Signal an error.
///
void signalError() {
	ledController.setState(LEDController::Red, LEDController::BlinkFast);
	logicState = ErrorState; // Deactivate everything except the LED blinking.
}

