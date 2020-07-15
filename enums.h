/***************************************************************************
 *   This file is part of SmartStrip.                                      *
 *                                                                         *
 *   Copyright (C) 2012-2020 by SukkoPera                                  *
 *                                                                         *
 *   SmartStrip is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   SmartStrip is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with SmartStrip.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef _ENUMS_H_
#define _ENUMS_H_

#include "common.h"

enum RelayMode {
	RELMD_ON = 0,			// Always on
	RELMD_OFF = 1,			// Always off
#ifdef ENABLE_THERMOMETER
	RELMD_GT = 2,			// On if T > Tthres
	RELMD_LT = 3,			// On if T < Tthres
#endif
#ifdef ENABLE_TIME
	RELMD_TIMED = 4,		// Timed on/off
#endif
};

enum TemperatureUnits {
	TEMP_C = 0,
	TEMP_F = 1
};

enum RelayState {
	RELAY_OFF = LOW,
	RELAY_ON = HIGH
};

enum NetworkMode {
	NETMODE_DHCP = 0,
	NETMODE_STATIC = 1
};

// If this is modified, please review the EEPROM offsets in common.h
struct RelayOptions {
	RelayMode mode;
	RelayState state;
	TemperatureUnits units;
	byte threshold;
	byte hysteresis;
	byte delay;
};

#endif
