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

#include <Arduino.h>

class Schedule {
private:
	static const byte N_HOURS = 24;
	static const byte N_DAYS = 7;
	static const byte SLOTS_PER_HOUR = 4;
	static const byte MINS_PER_SLOT = 60 / SLOTS_PER_HOUR;
	static const byte HOURS_PER_BYTE = 8 / SLOTS_PER_HOUR;
	static const byte BYTES_PER_DAY = N_HOURS / HOURS_PER_BYTE;

	byte schedule[BYTES_PER_DAY];

public:
	void begin () {
		for (byte i = 0; i < BYTES_PER_DAY; ++i) {
			schedule[i] = 0;
		}
	}

	boolean get (const byte h, const byte m) {
		byte b = h / HOURS_PER_BYTE;
		byte hi = h % HOURS_PER_BYTE;
		byte bit = m / MINS_PER_SLOT;
		return schedule[b] & (1 << (hi ? bit + 4 : bit));
	}

	void set (const byte h, const byte m, boolean on) {
		byte b = h / HOURS_PER_BYTE;
		byte hi = h % HOURS_PER_BYTE;
		byte bit = m / MINS_PER_SLOT;

		if (on) {
			schedule[b] |= (1 << (hi ? bit + 4 : bit));
		} else {
			schedule[b] &= ~(1 << (hi ? bit + 4 : bit));
		}
	}
};
