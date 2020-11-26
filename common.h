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

/* Uncomment one of these #defines according to the board you will be compiling
 * for. If you want to fine-tune the settings, leave these all commented out and
 * see below (Look for "#else").
 */
//! \name Board selection
//! @{
//#define SSBRD_KMTRONIC_DINO_328
//#define SSBRD_THINGSWITCHER
//#define SSBRD_THINGSWITCHER_WIFI
//#define SSBRD_GENERIC_328
//#define SSBRD_GENERIC_ESP8266
//#define SSBRD_GENERIC_ESP32
//#define SSBRD_GENERIC_STM32
//! @}

/* Define these to enable HTTP authentication. Note that HTTP authentication
 * must also be enabled in Webbino for this to work.
 */
//! @{ \name HTTP Authentication
//~ #define AUTH_REALM "SmartStrip"
//~ #define AUTH_USER "smart"
//~ #define AUTH_PASSWD "strip"
//! @}

/** \def USE_FAHRENHEIT
 *
 * \brief Use degrees Fahrneheit
 * 
 * Define to express temperatures in degrees Fahrenheit, leave undefined for
 * Celsius
 */
//#define USE_FAHRENHEIT

//! \brief Delay between temperature readings
#define THERMO_READ_INTERVAL (60 * 1000UL)

/** \def NDEBUG
 * 
 * \brief Define this to \a disable debug messages
 */
#define NDEBUG


/*******************************************************************************
 * BOARD SETTINGS
 ******************************************************************************/

#if defined (SSBRD_KMTRONIC_DINO_328)
	// The ATmega328-based KMTronic DINo was our first supported board ever!
	#define RELAYS_NO 4
	#define RELAY1_PIN 8
	#define RELAY2_PIN 7
	#define RELAY3_PIN 6
	#define RELAY4_PIN 5
	#define WEBINTF_SIMPLE
#elif defined (SSBRD_THINGSWITCHER)
	/* Configuration for my (private) STM32-based ThingSwitcher board
	 * 
	 * Build as Generic STM32F1
	 */
	#define RELAYS_NO 3
	#define RELAY1_PIN PB14
	#define RELAY2_PIN PB13
	#define RELAY3_PIN PB12
	#define RELAYS_ACTIVE_LOW
	#define ENABLE_THERMOMETER
	#define THERMOMETER_PIN PB0
	#define ENABLE_TIME
	#define TIMESRC_STM32_RTC
	#define WEBINTF_RICH
	#define SMARTSTRIP_ENABLE_REST
#elif defined (SSBRD_THINGSWITCHER_WIFI)
	/* Configuration for my (private) ESP8266-based ThingSwitcherWifi board
	 * 
	 * Build as NodeMCU 1.0
	 */
	#define RELAYS_NO 3
	#define RELAY1_PIN 2
	#define RELAY2_PIN 1
	#define RELAY3_PIN 0
	#define RELAYS_ACTIVE_LOW
	#define ENABLE_THERMOMETER
	#define THERMOMETER_PIN 7		// Actually this is the multiplexer channel
	#define ENABLE_TIME
	#define TIMESRC_NTP
	#define WEBINTF_RICH
	#define SMARTSTRIP_ENABLE_REST
#elif defined (SSBRD_GENERIC_328)
	// FIXME
#elif defined (SSBRD_GENERIC_ESP8266)
	// FIXME
#elif defined (SSBRD_GENERIC_ESP32)
	#define RELAYS_NO 4
	#define RELAY1_PIN 12
	#define RELAY2_PIN 13
	#define RELAY3_PIN 14
	#define RELAY4_PIN LED_BUILTIN
	#define RELAYS_ACTIVE_LOW
	#define ENABLE_TIME
	#define TIMESRC_NTP
	#define WEBINTF_RICH
#elif defined (SSBRD_GENERIC_STM32)
	// FIXME
#else
	#warning "Using generic board"

	/* If you are using a custom board, feel free to customize the settings in
	 * this section
	 */
	
	#define RELAYS_NO 3
	#define RELAY1_PIN 2
	#define RELAY2_PIN 1
	#define RELAY3_PIN 0
	
	/* Define this if your relays are activated setting their input pin LOW.
	 * Note that this applies to all relays, you can't currently have mixed active-
	 * high and active-low relays.
	 */
	#define RELAYS_ACTIVE_LOW

	// Define to enable temperature-controlled relays
	//~ #define ENABLE_THERMOMETER

	// Thermometer's data wire is connected to pin 2
	//~ #define THERMOMETER_PIN 2

	// Enable the possibility to schedule on/off times according to time (Experimental)
	//~ #define ENABLE_TIME

	// Enable EXPERIMENTAL RESTful API
	//~ #define SMARTSTRIP_ENABLE_REST
#endif
