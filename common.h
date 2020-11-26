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

#ifndef COMMON_H_
#define COMMON_H_

// Size of a MAC address (bytes)
#define MAC_SIZE 6

// Size of an IP address (bytes)
#define IP_SIZE 4

/* Network parameter defaults: feel free to change these to fit your network
 * setup. Note that these are the DEFAULTS, so they will only be used when your
 * board's EEPROM is being formatted.
 *
 * You can change the actual parameters through the web interface, but changing
 * these can be useful for the initial setup.
 *
 * To force formatting of the EEPROM use the "eeprom_clear" sketch provided
 * with Arduino.
 */
#define DEFAULT_RELAY_MODE RELMD_OFF
#define DEFAULT_RELAY_THRESHOLD 25
#define DEFAULT_RELAY_HYSTERESIS 1				// Degrees
#define DEFAULT_RELAY_DELAY 5					// Minutes

#define DEFAULT_MAC_ADDRESS 0x00,0x11,0x22,0x33,0x44,0x55
#define DEFAULT_NET_MODE NETMODE_DHCP // Set to NETMODE_STATIC to disable DHCP
#define DEFAULT_IP_ADDRESS 192,168,1,42
#define DEFAULT_NETMASK 255,255,255,0
#define DEFAULT_GATEWAY_ADDRESS 192,168,1,254
#define DEFAULT_DNS_ADDRESS 8,8,8,8


#define PROGRAM_VERSION "0.6git"

#include "board.h"

#endif
