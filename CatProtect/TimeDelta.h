//
// TimeDelta
// (c)2014 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//
#pragma once


#include <Arduino.h>


namespace lr {


/// Helper class to measure a time delta.
///
class TimeDelta 
{
public:
	/// Create a new time delta.
	///
	TimeDelta()
		: _start(0)
	{
	}
	
	/// dtor
	///
	~TimeDelta()
	{
	}
	
	/// Start the time delta with the given time.
	///
	void start(const unsigned long time)
	{
		_start = time;
	}
	
	/// Check how much time elapsed since the start.
	///
	unsigned long elapsed(const unsigned long time) const
	{
		return time - _start;
	}

private:
	unsigned long _start; /// The start time.
};


}


