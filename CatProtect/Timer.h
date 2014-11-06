#pragma once
//
// Timer
// (c)2014 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//


namespace lr {


/// A simple timer class to check periodic events in the loop.
///
/// It provides a simple callback principle to triggers things if the timer "fires".
/// But you can also use the return of the check() method to trigger your event.
///
class Timer
{
public:
	/// The callback.
	///
	typedef void (*Callback)(const unsigned long);

public:
	/// Create a new timer with the given delay.
	///
	Timer() : _callback(0), _start(0), _delay(200), _enabled(false) {
	}
	
	/// Start the timer.
	///
	void start(const unsigned long delay, const unsigned long currentTime) {
		_delay = delay;
		_start = currentTime;
		_enabled = true;
	}
	
	/// Stop the timer.
	///
	void stop() {
		_enabled = false;
	}
	
	/// Check the timer.
	///
	/// @return true if the timer "fires"
	///
	bool check(const unsigned long currentTime) {
		if (!_enabled) {
			return false;
		}
		const unsigned long delta = currentTime - _start;
		if (delta >= _delay) {
			_start = currentTime;
			if (_callback) {
				_callback(currentTime);
			}
			return true;
		}
		return false;
	}
	
	/// Set a function to call if the timer fires.
	///
	void setCallback(Callback callback) {
		_callback = callback;
	}
	
private:
	Callback _callback; ///< The callback to call if the timer "fires".
	bool _enabled; ///< If the timer is enabled or not.
	unsigned long _start; ///< The start of the timer.
	unsigned long _delay; ///< The delay for the next event.
};


}


