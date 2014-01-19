/*
 Heater.h - Heater library
 Copyright (c) 2013 Gábor Kövér  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef Heater_h
#define Heater_h

#include <inttypes.h>
#include <Time/Time.h>

/**
 * This class encapsulates the functionality of the heating system.
 * It uses 96 heating periods per day each of which is 15 minutes long.
 *
 *
 */
class Heater
{
private:
	time_t lastCheck; // the last time we computed the heat flag
	uint8_t  heatStatus; // the current heat status
	uint8_t  day0[12]; // the actual preset for heating
	uint8_t  day1[12]; // the preset for next day
public:
	uint8_t  calculate(time_t currentTime); // calculates if we need to turn on heating
	uint8_t  set(int period, uint8_t  required); // set the heating request
	Heater();
	virtual ~Heater();
};

extern Heater heater;

#endif

