#pragma once
//
// MotionSensor
// (c)2014 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//


#include "TimeDelta.h"

#include <Arduino.h>


namespace lr {


/// The class which handles the motion sensor.
///
class MotionSensor
{
public:
	/// The status of the motion sensor.
	enum Status : uint8_t {
		Uninitialized, ///< Loop was never called before.
		WaitStablilize, ///< Wait for the sensor to stabilize.
		Idle, ///< The motion sensor is working.
		Alarm, ///< The motion sensor registered motion.
	};

	/// The callback.
	///
	/// The first parameter is the current time, and the
	/// The second parameter is the new status.
	///
	typedef void (*Callback)(const unsigned long, Status);

private:
	/// The analog pin of the motion sensor.
	///
	const int MOTION_SENSOR = 0;

	/// The idle time in seconds.
	/// This is the time which the sensor has to be in LOW state, before another event
	/// is triggered. It is also the initialization time which is required until
	/// the sensor is considered as ready.
	///
	const int IDLE_TIME = 20;

public:
	/// ctor
	///
	MotionSensor();
	
	/// dtor
	///
	~MotionSensor();

public:
	/// Call this method in setup()
	///
	void setup();
	
	/// Set a callback if the status changes.
	///
	void setCallback(Callback callback);
	
	/// Call this method in loop();
	///
	void loop(const unsigned long currentTime);
	
	/// Get the current status of the motion sensor.
	///
	inline Status status() const { return _status; }
	
private:
	/// Set the status and call the callback
	///
	void setStatus(Status status, const unsigned long currentTime);
	
	/// Get the current sensor state.
	///
	bool currentSensorState() const;
	
private:
	Status _status; ///< The status of the sensor.
	TimeDelta _lastEvent; ///< The last event of the sensor.
	bool _lastState; ///< The last state to detect a change.
	Callback _callback; ///< The callback to call if the sensor state changes.
};


}
