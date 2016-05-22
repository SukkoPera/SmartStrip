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

#ifndef COMMON_H_
#define COMMON_H_

// Define this to disable debug messages
#define NDEBUG

#define RELAYS_NO 4
#define RELAY1_PIN 8
#define RELAY2_PIN 7
#define RELAY3_PIN 6
#define RELAY4_PIN 5

/* Define this if your relays are activated setting their input pin LOW.
 * Note that this applies to all relays, you can't currently have mixed active-
 * high and active-low relays.
 */
//#define RELAYS_ACTIVE_LOW

// Define to enable temperature-controlled relays
#define ENABLE_THERMOMETER

// Thermometer's data wire is connected to pin 5
#define THERMOMETER_PIN 2

// DS18B20's data resolution
#define THERMOMETER_RESOLUTION 9

// Delay between temperature readings
#define THERMO_READ_INTERVAL (5 * 1000U)

// Totally useless at this time ;)
//#define USE_ARDUINO_TIME_LIBRARY

// Size of a MAC address (bytes)
#define MAC_SIZE 6

// Size of an IP address (bytes)
#define IP_SIZE 4

// EEPROM offsets
#define EEPROM_MAGIC 0x50545353UL			// "SSTP"
#define EEPROM_R1_PARAM_ADDR 4
#define EEPROM_R2_PARAM_ADDR 20
#define EEPROM_R3_PARAM_ADDR 36
#define EEPROM_R4_PARAM_ADDR 52
#define EEPROM_MAC_ADDR 64
#define EEPROM_NETMODE_ADDR 70				// This has turned to 2 bytes in some recent version of Arduino
#define EEPROM_IP_ADDR 72
#define EEPROM_NETMASK_ADDR 76
#define EEPROM_GATEWAY_ADDR 80



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
#define DEFAULT_RELAY_STATE RELAY_OFF
#define DEFAULT_RELAY_THRESHOLD 25
#define DEFAULT_RELAY_UNITS TEMP_C
#define DEFAULT_RELAY_HYSTERESIS 10				// Tenths of degrees
#define DEFAULT_RELAY_DELAY 5					// Minutes

#define DEFAULT_MAC_ADDRESS_B1 0x00
#define DEFAULT_MAC_ADDRESS_B2 0x11
#define DEFAULT_MAC_ADDRESS_B3 0x22
#define DEFAULT_MAC_ADDRESS_B4 0x33
#define DEFAULT_MAC_ADDRESS_B5 0x44
#define DEFAULT_MAC_ADDRESS_B6 0x55
#define DEFAULT_NET_MODE NETMODE_DHCP // Set to NETMODE_STATIC to disable DHCP
#define DEFAULT_IP_ADDRESS_B1 192
#define DEFAULT_IP_ADDRESS_B2 168
#define DEFAULT_IP_ADDRESS_B3 1
#define DEFAULT_IP_ADDRESS_B4 42
#define DEFAULT_NETMASK_B1 255
#define DEFAULT_NETMASK_B2 255
#define DEFAULT_NETMASK_B3 255
#define DEFAULT_NETMASK_B4 0
#define DEFAULT_GATEWAY_ADDRESS_B1 192
#define DEFAULT_GATEWAY_ADDRESS_B2 168
#define DEFAULT_GATEWAY_ADDRESS_B3 1
#define DEFAULT_GATEWAY_ADDRESS_B4 254


#define PROGRAM_VERSION "0.6git"

#endif
