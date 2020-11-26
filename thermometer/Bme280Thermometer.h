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

#ifndef _BME280THERMO_H_
#define _BME280THERMO_H_

#include <Wire.h>
#include "../common.h"

#ifdef USE_BME280_THERMO

// https://github.com/SukkoPera/Arduino-BoschBME280
#include <BoschBME280.h>

#include "ThermometerBase.h"
#include "thermometer_debug.h"


class Bme280Thermometer: public ThermometerBase {
private:
	BoschBME280 bme;
	BoschBME280::data conditions = {0, 0, 0};

	boolean refreshTemperature () {
		boolean ret = false;
		currentTemp.valid = false;

		if (bme.set_sensor_mode (BoschBME280::MODE_FORCED) == 0) {
			// Wait for the measurements to complete
			delay (40);

			BoschBME280::data data;
			if (bme.get_sensor_data (BoschBME280::DATA_ALL, data) == 0) {
				currentTemp.celsius = data.temperature;
				currentTemp.valid = true;

				DPRINT (F("Temperature: "));
				DPRINT (data.temperature, 2);
				DPRINTLN (F("*C"));

				DPRINT (F("Humidity: "));
				DPRINT (data.humidity, 2);
				DPRINTLN (F("%"));

				DPRINT (F("Absolute pressure: "));
				DPRINT (data.pressure / 100.0, 2);		// 100 Pa = 1 mbar
				DPRINTLN (F(" mbar"));

				ret = true;
			} else {
				DPRINTLN (F("Error retrieving measurement"));
			}
		} else {
			DPRINTLN (F("Error starting measurement"));
		}

		return ret;
	}

public:
	void begin (byte _pin) {
		(void) _pin;		// Not used

		Wire.begin ();

		// Init sensor, recommended mode of operation: Indoor navigation
		BoschBME280::Settings settings;
		settings.osr_h = BoschBME280::OVERSAMPLING_1X;
		settings.osr_p = BoschBME280::OVERSAMPLING_16X;
		settings.osr_t = BoschBME280::OVERSAMPLING_2X;
		settings.filter = BoschBME280::FILTER_COEFF_16;

		if (bme.begin (BoschBME280::I2C_ADDR_PRIM, BoschBME280::INTF_I2C, settings) == 0) {
			uint8_t settings_sel = BoschBME280::SEL_OSR_PRESS | BoschBME280::SEL_OSR_TEMP | BoschBME280::SEL_OSR_HUM | BoschBME280::SEL_FILTER;
			if (bme.set_sensor_settings (settings_sel) == 0) {
				DPRINTLN ("BME280 found and initilized");
				available = true;
			} else {
				DPRINTLN (F("Cannot set BME280 settings"));
			}
		} else {
			DPRINTLN (F("BME280 failed begin()"));
		}
	}

};

typedef Bme280Thermometer Thermometer;

#endif

#endif
