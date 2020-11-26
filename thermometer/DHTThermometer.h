/***************************************************************************
 *   This file is part of Sukkino.                                         *
 *                                                                         *
 *   Copyright (C) 2013-2016 by SukkoPera                                  *
 *                                                                         *
 *   Sukkino is free software: you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Sukkino is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with Sukkino.  If not, see <http://www.gnu.org/licenses/>.      *
 ***************************************************************************/

#ifndef _DHTTHERMO_H_
#define _DHTTHERMO_H_

#include "../common.h"

#ifdef USE_DHT_THERMO

#include <DHT.h>			// https://github.com/adafruit/DHT-sensor-library
#include "ThermometerBase.h"


class DHTThermometer: public ThermometerBase {
private:
	DHT sensor;

	boolean refreshTemperature () override {
		if (available) {
			DPRINT (F("Requesting temperature..."));
			currentTemp.celsius = sensor.readTemperature ();
			currentTemp.valid = !isnan (currentTemp.celsius);
			DPRINTLN (F(" Done"));
		}

		return currentTemp.valid;
	}

public:
	DHTThermometer (): sensor (0, 0) {		// Temporary values
	}

	void begin (byte busPin, byte type = 22) {		// Type is either 11, 21 or 22
		DPRINT (F("Scanning for temperature sensors on pin "));
		DPRINT (busPin);
		DPRINTLN ();
		sensor = DHT (busPin, type);			// Reinit with correct values. This is safe as of 22/01/2013
		sensor.begin ();
		available = sensor.read ();		 // Can we really detect if the sensor is available?
	}
};

typedef DHTThermometer Thermometer;

#endif

#endif
