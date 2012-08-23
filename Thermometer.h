/***************************************************************************
 *   This file is part of SmartStrip.                                      *
 *                                                                         *
 *   Copyright (C) 2012 by SukkoPera                                       *
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

#ifndef _THERMOMETER_H_
#define _THERMOMETER_H_

#include "common.h"
#include "debug.h"
#include <OneWire.h>
#include <DallasTemperature.h>


#define THERMO_READ_INTERVAL (1 * 1000U)        //delay between temperature readings


struct Temperature {
	bool valid;
	float celsius;

	float toFahrenheit () const {
		return DallasTemperature::toFahrenheit (celsius);
	}
};

class Thermometer {
	// OneWire instance to communicate with OneWire devices
	OneWire oneWire;
	DallasTemperature sensors;
	DeviceAddress thermometerAddress;

	bool thermometerAvailable;
	unsigned long lastTemperatureRequest;
	Temperature currentTemp;

	// function to print a device address
	static void printAddress (DeviceAddress deviceAddress) {
		for (uint8_t i = 0; i < 8; i++) {
			if (deviceAddress[i] < 16)
				Serial.print ("0");
			Serial.print (deviceAddress[i], HEX);
		}
	}

		// function to print the temperature for a device
	static void printTemperature (float tempC, bool fahrenheit = false) {
		if (!fahrenheit) {
			DPRINT (F("Temp C: "));
			DPRINTLN (tempC);
		} else {
			DPRINT (F("Temp F: "));
			DPRINTLN (DallasTemperature::toFahrenheit (tempC)); // Converts tempC to Fahrenheit
		}
	}

public:	
	Thermometer (int busPin = ONE_WIRE_BUS_PIN): oneWire (busPin), sensors (&oneWire) {
	}
	
	bool setup (void) {
		// locate devices on the bus
		Serial.print (F("Scanning for temperature sensors on pin "));
		Serial.print (ONE_WIRE_BUS_PIN);
		Serial.println ();
		sensors.begin ();

		// Report parasite power requirements
		Serial.print (F("Parasite power is: "));
		Serial.println (sensors.isParasitePowerMode () ? F("ON") : F("OFF"));

		if (sensors.getDeviceCount () > 0) {
			Serial.print (F("Found "));
			Serial.print (sensors.getDeviceCount (), DEC);
			Serial.println (F(" device(s)"));

			if (!sensors.getAddress (thermometerAddress, 0)) {
				Serial.println (F("Unable to find address for Device 0"));
				thermometerAvailable = false;
			} else {
				// show the addresses we found on the bus
				Serial.print (F("Using device with address: "));
				printAddress (thermometerAddress);
				Serial.println ();

				// set the resolution
				sensors.setResolution (thermometerAddress, THERMOMETER_RESOLUTION);
				Serial.print (F("Device Resolution: "));
				Serial.print (sensors.getResolution (thermometerAddress), DEC);
				Serial.println ();

				thermometerAvailable = true;
			}
		} else {
			Serial.println (F("No sensors found"));
			thermometerAvailable = false;
		}

		return thermometerAvailable;
	}

	void loop (void) {
		if (thermometerAvailable && millis () - lastTemperatureRequest > THERMO_READ_INTERVAL) {
			/* Call sensors.requestTemperatures() to issue a global temperature
			* request to all devices on the bus.
			*/
			DPRINT (F("Requesting temperatures..."));
			sensors.requestTemperatures ();
			currentTemp.celsius = sensors.getTempC (thermometerAddress);
			currentTemp.valid = true;
			DPRINTLN (F(" Done"));

			// Print out the data
			printTemperature (currentTemp.celsius);

			lastTemperatureRequest = millis ();
		}
		
	}

/*	bool isThermometerAvailable () {
		return thermometerAvailable;
	}
*/

	Temperature& getTemp () {
		return currentTemp;
	}
};

#endif
