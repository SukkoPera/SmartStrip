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

#ifndef COMMON_H_
#define COMMON_H_

// #define NDEBUG

//#define USE_UNIX
//#define USE_ARDUINO_TIME_LIBRARY

#define RELAYS_NO 4
#define RELAY1_PIN 8
#define RELAY2_PIN 7
#define RELAY3_PIN 6
#define RELAY4_PIN 5

// Define to enable temperature-controlled relays
#define ENABLE_THERMOMETER

// DS18B20's data wire is connected to pin 2
#define ONE_WIRE_BUS_PIN 2

// DS18B20's data resolution
#define THERMOMETER_RESOLUTION 9

//Delay between temperature readings
#define THERMO_READ_INTERVAL (1 * 1000U)


#define EEPROM_MAGIC 0x50545353UL			// "SSTP"
#define EEPROM_SIZE 64						// Don't waste write cycles of much more EEPROM than we need
#define EEPROM_R1_PARAM_ADDR 4
#define EEPROM_R2_PARAM_ADDR 12
#define EEPROM_R3_PARAM_ADDR 20
#define EEPROM_R4_PARAM_ADDR 28
#define EEPROM_MAC_B1_ADDR 40
#define EEPROM_MAC_B2_ADDR 41
#define EEPROM_MAC_B3_ADDR 42
#define EEPROM_MAC_B4_ADDR 43
#define EEPROM_MAC_B5_ADDR 44
#define EEPROM_MAC_B6_ADDR 45
#define EEPROM_NETMODE_ADDR 49
#define EEPROM_IP_B1_ADDR 50
#define EEPROM_IP_B2_ADDR 51
#define EEPROM_IP_B3_ADDR 52
#define EEPROM_IP_B4_ADDR 53
#define EEPROM_NETMASK_B1_ADDR 54
#define EEPROM_NETMASK_B2_ADDR 55
#define EEPROM_NETMASK_B3_ADDR 56
#define EEPROM_NETMASK_B4_ADDR 57
#define EEPROM_GATEWAY_B1_ADDR 58
#define EEPROM_GATEWAY_B2_ADDR 59
#define EEPROM_GATEWAY_B3_ADDR 60
#define EEPROM_GATEWAY_B4_ADDR 61

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
#define DEFAULT_RELAY_HYSTERESIS 10
#define DEFAULT_RELAY_DELAY 5

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
#define DEFAULT_NETMASK_B2 0
#define DEFAULT_NETMASK_B3 0
#define DEFAULT_NETMASK_B4 0
#define DEFAULT_GATEWAY_ADDRESS_B1 192
#define DEFAULT_GATEWAY_ADDRESS_B2 168
#define DEFAULT_GATEWAY_ADDRESS_B3 1
#define DEFAULT_GATEWAY_ADDRESS_B4 254


#define PROGRAM_VERSION "0.5git"

#endif
