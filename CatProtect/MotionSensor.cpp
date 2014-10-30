//
// MotionSensor
// (c)2014 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//
#include "MotionSensor.h"


namespace lr {


MotionSensor::MotionSensor()
	: _lastState(false), _status(Uninitialized), _callback(0)
{
}


MotionSensor::~MotionSensor()
{
}


void MotionSensor::setup()
{
}


void MotionSensor::loop(const unsigned long currentTime)
{
	if (_status == Uninitialized) {
		_lastState = currentSensorState();
		setStatus(WaitStablilize, currentTime);
	} else if (_status == WaitStablilize) {
		// Check the sensor state.
		const bool sensorState = currentSensorState();
		if (sensorState != _lastState) {
			_lastEvent.start(currentTime);
			_lastState = sensorState;			
		} else if (sensorState == false) {
			if ((_lastEvent.elapsed(currentTime)/1000) >= IDLE_TIME) {
				// Great, we can process in working state.
				setStatus(Idle, currentTime);
			}
		}
	} else if (_status == Idle) {
		const bool sensorState = currentSensorState();
		if (sensorState) { // Alarm!
			_lastState = true;
			_lastEvent.start(currentTime);
			setStatus(Alarm, currentTime);
		}		
	} else if (_status == Alarm) {
		// Wait until the sensor goes back to idle state.
		const bool sensorState = currentSensorState();
		if (sensorState != _lastState) {
			_lastEvent.start(currentTime);
			_lastState = sensorState;			
		} else if (sensorState == false) {
			if ((_lastEvent.elapsed(currentTime)/1000) >= IDLE_TIME) {
				// Great, we can process in working state.
				setStatus(Idle, currentTime);
			}
		}
	}
}


bool MotionSensor::currentSensorState() const
{
	const int sensorValue = analogRead(MOTION_SENSOR);
	const bool sensorState = (sensorValue > 200);
	return sensorState;
}


void MotionSensor::setStatus(Status status, const unsigned long currentTime)
{
	if (_status != status) {
		_status = status;
		if (_callback != 0) {
			_callback(currentTime, status);
		}
	}
}


void MotionSensor::setCallback(Callback callback)
{
	_callback = callback;
}


}


