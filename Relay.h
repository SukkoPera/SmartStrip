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

#ifndef _RELAY_H_
#define _RELAY_H_

#include "enums.h"
#include "common.h"

	
class Relay: public RelayOptions {
	static const int optionAddress[RELAYS_NO];

public:	
	byte id;
	byte pin;
	
	Relay (byte _id, byte _pin);

	void readOptions ();
	void writeOptions ();
	void setDefaults ();

	void switchState (RelayState newState);
	void effectState ();
};
	
#endif
