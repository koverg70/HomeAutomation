/*
 * Heater.cpp
 *
 *  Created on: 2013.12.14.
 *      Author: koverg
 */

#include "Heater.h"

Heater::Heater()
{
	// TODO Auto-generated constructor stub

}

Heater::~Heater()
{
	// TODO Auto-generated destructor stub
}

uint8_t calcQuarter(time_t currentTime)
{
//	int hour = hour(currentTime);
//	int minute = minute(currentTime);
//	return hour * 4 + (minute / 15);
}

uint8_t Heater::calculate(time_t currentTime)
{
	uint8_t currentQuarter = calcQuarter(currentTime);
	uint8_t lastQuarter = calcQuarter(lastCheck);

	if (currentQuarter != lastQuarter)
	{
		// moved to next Quarter
	}

	int idx = currentQuarter / 8;
	int bit = currentQuarter % 8;

	uint8_t ret = (day0[idx] >> bit) & 1;

	lastCheck = currentTime;

	return ret;
}

uint8_t Heater::set(int period, uint8_t required)
{
}
