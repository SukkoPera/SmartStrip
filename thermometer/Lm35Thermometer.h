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

#ifndef _LM35THERMO_H_
#define _LM35THERMO_H_

#include "thermometer_common.h"

#ifdef USE_LM35_THERMO

#include "ThermometerBase.h"
#include "thermometer_debug.h"


class Lm35Thermometer: public ThermometerBase {
private:
	// Number of actual ADC samplings per read operation
	static const byte N_READS = 5;

	// ADC resolution
#ifdef ARDUINO_ARCH_AVR
	static const word ADC_STEPS = 1024;
#elif defined ARDUINO_ARCH_STM32F1
	static const word ADC_STEPS = 4096;
#else
	#error "Please define ADC resolution"
#endif

	// ADC reference voltage
#ifdef ARDUINO_ARCH_AVR
	static const word VCC = 5000; // Millivolt
#elif defined ARDUINO_ARCH_STM32F1
	static const word VCC = 3300;
#else
	#error "Please define ADC reference voltage"
#endif

	byte pin;

	boolean refreshTemperature () {
		word tot = 0;

		for (byte i = 0; i < N_READS; ++i) {
			tot += analogRead (pin);
		}
		word adc = tot / N_READS;
		word v = (unsigned long) adc * VCC / ADC_STEPS;

		currentTemp.celsius = v / 10;	// LM35 has 10 mV/deg

		return true;
	}

public:
	void begin (byte _pin) {
		pin = _pin;
		available = true;	// No way of knowing if it's actually there or not
		currentTemp.valid = true;	// So temp will always be valid too
	}
};

typedef Lm35Thermometer Thermometer;

#endif

#endif
