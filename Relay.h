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

#ifndef _RELAY_H_
#define _RELAY_H_

#include "enums.h"
#include "common.h"
#include "debug.h"

#ifdef ENABLE_TIME
#include "Schedule.h"
#endif

class Relay {
private:
	const byte pin;

public:
	const byte id;

	boolean enabled;		// true when contacts are closed and relay is conducing

#ifdef ENABLE_THERMOMETER
	byte threshold;			// SetPoint +/- hysteresis
#endif

#ifdef ENABLE_TIME
	Schedule schedule;
#endif

	Relay (const byte _id, const byte _pin): pin (_pin), id (_id) {
#ifdef RELAYS_ACTIVE_LOW
		/* Switching the pin from the start-up mode to OUTPUT will make it go low
		 * and turn the relay on in this case. By writing the pin HIGH before
		 * calling pinMode(), we will turn on its pull-up resistor and have it high
		 * when mode is changed.
		 *
		 * Note that this works on both AVRs and STM32F1.
		 */
		digitalWrite (_pin, HIGH);
#endif
		pinMode (_pin, OUTPUT);

		enabled = false;

#ifdef ENABLE_TIME
		schedule.begin (GlobalOptions::getScheduleOffset (id));
#endif
	}

	void setDefaults () {
		setMode (DEFAULT_RELAY_MODE);
#ifdef ENABLE_THERMOMETER
		setSetPoint (DEFAULT_RELAY_THRESHOLD);
#endif

#ifdef ENABLE_TIME
		schedule.clear ();
#endif
	}

	RelayMode getMode () const {
		byte mode = EEPROM.read (GlobalOptions::getRelayParamOffset (id));
		return static_cast<RelayMode> (mode);
	}

	void setMode (const RelayMode mode) {
		EEPROM.write (GlobalOptions::getRelayParamOffset (id), static_cast<const byte> (mode));
#if defined (ARDUINO_ARCH_ESP32) || defined (ARDUINO_ARCH_ESP8266)
		EEPROM.commit ();
#endif
	}

#ifdef ENABLE_THERMOMETER
	byte getSetPoint () const {
		byte setPoint = EEPROM.read (GlobalOptions::getRelayParamOffset (id) + 1);
		return setPoint;
	}

	void setSetPoint (const byte setPoint) {
		EEPROM.write (GlobalOptions::getRelayParamOffset (id) + 1, setPoint);
#if defined (ARDUINO_ARCH_ESP32) || defined (ARDUINO_ARCH_ESP8266)
		EEPROM.commit ();
#endif
	}
#endif

	void setEnabled (boolean e) {
		WDPRINT (F("Turning "));
		WDPRINT (e ? F("ON") : F("OFF"));
		WDPRINT (F(" relay "));
		WDPRINTLN (id, DEC);

		enabled = e;

#ifdef RELAYS_ACTIVE_LOW
		digitalWrite (pin, enabled ? LOW : HIGH);
#else
		digitalWrite (pin, enabled ? HIGH : LOW);
#endif
	}
};

#endif
