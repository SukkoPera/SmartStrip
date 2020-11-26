/***************************************************************************
 *   This file is part of Webbino                                          *
 *                                                                         *
 *   Copyright (C) 2012-2019 by SukkoPera                                  *
 *                                                                         *
 *   Webbino is free software: you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Webbino is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with Webbino. If not, see <http://www.gnu.org/licenses/>.       *
 ***************************************************************************/

/* This file provides convenience functions for I/O manipulation on the
 * ThingSwitcherWifi V1 board
 */

#ifndef TSWIFI_H_INCLUDED
#define TSWIFI_H_INCLUDED

#include "GlobalOptions.h"

#ifdef SSBRD_THINGSWITCHER_WIFI

#include <Arduino.h>
//~ #include <Wire.h>
#include "Adafruit_MCP23017.h"

static Adafruit_MCP23017 mcp;

class ThingSwitcherWifi {
private:
	// Lowest 3 bits of the MCP i2c address
	static const byte DEFAULT_MCP_ADDRESS = 0;

	static const byte ANALOG_N_READS = 3;

public:
	boolean begin (byte mcpAddress = DEFAULT_MCP_ADDRESS) {
		mcp.begin (mcpAddress);

		// Avoid noise on unconnected pins
		mcp.pinMode (11, INPUT_PULLUP);
		mcp.pinMode (12, INPUT_PULLUP);

		return true;	// FIXME
	}

	void pinMode (byte pin, byte mode) {
		mcp.pinMode (pin, mode);
	}

	void digitalWrite(byte pin, byte level) {
		mcp.digitalWrite (pin, level);
	}

	byte digitalRead (byte pin) {
		return mcp.digitalRead (pin);
	}

	word analogRead (byte pin) {
		word ret = -1;

		if (pin < 8) {
			// Switch multiplexer to desired channel (last 3 bits of GPIOB)
			mcp.digitalWrite (13, (pin & (1 << 0)) ? HIGH : LOW);
			mcp.digitalWrite (14, (pin & (1 << 1)) ? HIGH : LOW);
			mcp.digitalWrite (15, (pin & (1 << 2)) ? HIGH : LOW);

			// Let things settle
			delayMicroseconds (100);

			// Discard first read
			::analogRead (0);	// Make sure to call the analogRead() in the global namespace!

			// Finally do the readings
			word tot = 0;
			for (byte i = 0; i < ANALOG_N_READS; ++i) {
				tot += ::analogRead (0);
				delayMicroseconds (10);
			}
			ret = tot / ANALOG_N_READS;
		}

		return ret;
	}
};


#endif

#endif
