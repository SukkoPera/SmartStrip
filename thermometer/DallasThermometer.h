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

#ifndef _DALLASTHERMO_H_
#define _DALLASTHERMO_H_

#include "thermometer_common.h"

#ifdef USE_DALLAS_THERMO

#include <OneWire.h>
#include <DallasTemperature.h>
#include "ThermometerBase.h"
#include "thermometer_debug.h"

// Chosen data resolution
#define THERMOMETER_RESOLUTION 9


class DallasThermometer: public ThermometerBase {
private:
	//byte busPin;

	// OneWire instance to communicate with OneWire devices
	OneWire oneWire;
	DallasTemperature sensors;
	DeviceAddress thermometerAddress;

	// function to print a device address
	void printAddress (DeviceAddress deviceAddress) {
		for (uint8_t i = 0; i < 8; i++) {
			if (deviceAddress[i] < 16)
				DPRINT (F("0"));
			DPRINT (deviceAddress[i], HEX);
		}
	}

	bool refreshTemperature () {
		if (available) {
			/* Call sensors.requestTemperatures() to issue a global temperature
			* request to all devices on the bus.
			*/
			DPRINT (F("Requesting temperatures..."));
			sensors.requestTemperatures ();
			currentTemp.celsius = sensors.getTempC (thermometerAddress);
			currentTemp.valid = true;
			DPRINTLN (F(" Done"));
		}

		return currentTemp.valid;
	}

public:
	DallasThermometer (): oneWire (2), sensors (&oneWire) {
	}



	void begin (byte busPin) {
		// locate devices on the bus
		DPRINT (F("Scanning for temperature sensors on pin "));
		DPRINT (busPin);
		DPRINTLN ();
		oneWire = OneWire (busPin);
		sensors.begin ();

		// Report parasite power requirements
		DPRINT (F("Parasite power is: "));
		DPRINTLN (sensors.isParasitePowerMode () ? F("ON") : F("OFF"));

		if (sensors.getDeviceCount () > 0) {
			DPRINT (F("Found "));
			DPRINT (sensors.getDeviceCount (), DEC);
			DPRINTLN (F(" device(s)"));

			if (!sensors.getAddress (thermometerAddress, 0)) {
				DPRINTLN (F("Unable to find address for Device 0"));
				available = false;
			} else {
				// show the addresses we found on the bus
				DPRINT (F("Using device with address: "));
				printAddress (thermometerAddress);
				DPRINTLN ();

				// set the resolution
				sensors.setResolution (thermometerAddress, THERMOMETER_RESOLUTION);
				DPRINT (F("Device Resolution: "));
				DPRINT (sensors.getResolution (thermometerAddress), DEC);
				DPRINTLN ();

				available = true;
			}
		} else {
			DPRINTLN (F("No sensors found"));
			available = false;
		}
	}

};

class Thermometer: public DallasThermometer {
};

#endif

#endif
