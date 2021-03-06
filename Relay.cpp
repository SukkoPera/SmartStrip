/***************************************************************************
 *   This file is part of SmartStrip.                                      *
 *                                                                         *
 *   Copyright (C) 2012-2016 by SukkoPera                                  *
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
#include <EEPROM.h>
#include "common.h"
#include "debug.h"
#include "Relay.h"

// FIXME: This can be put to flash only?
const int Relay::optionAddress[RELAYS_NO] = {
	EEPROM_R1_PARAM_ADDR,
	EEPROM_R2_PARAM_ADDR,
	EEPROM_R3_PARAM_ADDR,
	EEPROM_R4_PARAM_ADDR
};

Relay::Relay (byte _id, byte _pin): id (_id), pin (_pin) {
	//if (_id >= 1 && _id <= RELAYS_NO)
        pinMode (_pin, OUTPUT);
}

void Relay::readOptions () {
	EEPROM.get (optionAddress[id - 1], static_cast<RelayOptions&> (*this));

	DPRINT (F("sizeof (RelayOptions) = "));
	DPRINTLN (sizeof (RelayOptions));
	DPRINT (F("Relay "));
	DPRINT (id);
	DPRINT (F(" mode is "));
	DPRINT (mode);
	DPRINT (F(" and state is "));
	DPRINT (state);
	DPRINT (F(" and delay is "));
	DPRINTLN (delay);
}

void Relay::writeOptions () {
	DPRINT (F("Saving options for relay "));
	DPRINTLN (id);

	EEPROM.put (optionAddress[id - 1], static_cast<RelayOptions&> (*this));
}

void Relay::setDefaults () {
	mode = DEFAULT_RELAY_MODE;
	state = DEFAULT_RELAY_STATE;
	threshold = DEFAULT_RELAY_THRESHOLD;
	units = DEFAULT_RELAY_UNITS;
	hysteresis = DEFAULT_RELAY_HYSTERESIS;
	delay = DEFAULT_RELAY_DELAY;
}

void Relay::switchState (RelayState newState) {
	DPRINT (F("Turning "));
	DPRINT (newState == RELAY_ON ? F("ON") : F("OFF"));
	DPRINT (F(" relay "));
	DPRINTLN (id, DEC);

	state = newState;
	effectState ();
}

void Relay::effectState () {
#ifdef RELAYS_ACTIVE_LOW
	digitalWrite (pin, !state);
#else
	digitalWrite (pin, state);
#endif
}
