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

#ifndef _DUMMYTHERMO_H_
#define _DUMMYTHERMO_H_

#include "../common.h"

#ifdef USE_DUMMY_THERMO

#include "ThermometerBase.h"


class DummyThermometer: public ThermometerBase {
private:
	boolean refreshTemperature () {
		currentTemp.celsius = 21.5;
		currentTemp.valid = true;

		return true;
	}

public:
	void begin (byte pin) {
		(void) pin;
		available = true;
	}
};

typedef DummyThermometer Thermometer;

#endif

#endif
