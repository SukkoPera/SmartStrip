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

// Define this to disable debug messages
#define NDEBUG

#ifdef ARDUINO_ARCH_AVR
// These pins drive the relays on the KMTronic DINo
#define RELAYS_NO 4
#define RELAY1_PIN 8
#define RELAY2_PIN 7
#define RELAY3_PIN 6
#define RELAY4_PIN 5
#elif defined (ARDUINO_ARCH_ESP32)
// If using the above AVR pins on ESP32, it will crash at startup!
#define RELAYS_NO 4
#define RELAY1_PIN 12
#define RELAY2_PIN 13
#define RELAY3_PIN 14
#define RELAY4_PIN LED_BUILTIN
#elif defined (ARDUINO_ARCH_STM32F1)
// These are the pins used on my (private) ThingSwitcher board
#define RELAYS_NO 4
#define RELAY1_PIN PB14
#define RELAY2_PIN PB13
#define RELAY3_PIN PB12
#define RELAY4_PIN PC13		// This is actully the onboard led
#elif defined (ARDUINO_ESP8266_NODEMCU)
// These are the pins used on my (private) ThingSwitcherWifi board
#define RELAYS_NO 4
#define RELAY1_PIN D3
#define RELAY2_PIN D4
#define RELAY3_PIN D8
#define RELAY4_PIN D0		// This is actully the onboard led
#else
#error "Please define relay pins for this board"
#endif

/* Define these to enable HTTP authentication. Note that HTTP authentication
 * must also be enabled in Webbino for this to work.
 */
//~ #define AUTH_REALM "SmartStrip"
//~ #define AUTH_USER "smart"
//~ #define AUTH_PASSWD "strip"

/* Define this if your relays are activated setting their input pin LOW.
 * Note that this applies to all relays, you can't currently have mixed active-
 * high and active-low relays.
 */
#define RELAYS_ACTIVE_LOW

// Define to enable temperature-controlled relays
#define ENABLE_THERMOMETER

// Thermometer's data wire is connected to pin 2
#define THERMOMETER_PIN 2
//~ #define THERMOMETER_PIN PB0

// Define to express temperatures in degrees Fahrenheit
//#define USE_FAHRENHEIT

// Delay between temperature readings
#define THERMO_READ_INTERVAL (60 * 1000UL)

// Enable the possibility to schedule on/off times according to time (Experimental)
#define ENABLE_TIME

// Enable EXPERIMENTAL RESTful API
//~ #define SMARTSTRIP_ENABLE_REST

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

#endif
