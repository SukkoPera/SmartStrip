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

#include <Webbino.h>
#include <EEPROM.h>
#include "debug.h"
#include "Relay.h"
#include "enums.h"
#include "common.h"
#include "html.h"

// Instantiate the WebServer and page storage
WebServer webserver;
FlashStorage flashStorage;

// Instantiate the network interface defined in the Webbino headers
#if defined (WEBBINO_USE_ENC28J60)
	#include <WebbinoInterfaces/ENC28J60.h>
	NetworkInterfaceENC28J60 netint;
#elif defined (WEBBINO_USE_WIZ5100) || defined (WEBBINO_USE_WIZ5500) || \
	  defined (WEBBINO_USE_ENC28J60_UIP)
	#include <WebbinoInterfaces/WIZ5x00.h>
	NetworkInterfaceWIZ5x00 netint;

	// ENC28J60_UIP also needs an SS pin
	const byte SS_PIN = PA4;		// STM32
#elif defined (WEBBINO_USE_ESP8266)
	#include <WebbinoInterfaces/AllWiFi.h>

	#include <SoftwareSerial.h>
	SoftwareSerial swSerial (6, 7);

	// Wi-Fi parameters
	#define WIFI_SSID        "ssid"
	#define WIFI_PASSWORD    "password"

	NetworkInterfaceWiFi netint;
#elif defined (WEBBINO_USE_WIFI) || defined (WEBBINO_USE_WIFI101) || \
	  defined (WEBBINO_USE_ESP8266_STANDALONE)
	#include <WebbinoInterfaces/AllWiFi.h>

	// Wi-Fi parameters
	#define WIFI_SSID        "SukkoNet-TO"
	#define WIFI_PASSWORD    "everythingyouknowiswrong"

	NetworkInterfaceWiFi netint;
#elif defined (WEBBINO_USE_FISHINO)
	#include <WebbinoInterfaces/FishinoIntf.h>

	// Wi-Fi parameters
	#define WIFI_SSID        "ssid"
	#define WIFI_PASSWORD    "password"

	FishinoInterface netint;
#elif defined (WEBBINO_USE_DIGIFI)
	#include <WebbinoInterfaces/DigiFi.h>
	NetworkInterfaceDigiFi netint;
#endif


#ifdef ENABLE_THERMOMETER

#include "thermometer/Thermometer.h"

// Instantiate the thermometer
Thermometer thermometer;

// Current temperature (Start with average ambient temperature)
float temperature = 25;

// Time the last temperature request was issued
unsigned long lastTemperatureRequest = 0;
#endif

#ifdef ENABLE_TIME
#include "sstime.h"

byte rtCheckIdx = 0;
#endif

// Other stuff
byte lastSelectedRelay;

Relay relays[RELAYS_NO] = {
	Relay (1, RELAY1_PIN),
	Relay (2, RELAY2_PIN),
	Relay (3, RELAY3_PIN),
#if RELAYS_NO == 4
	Relay (4, RELAY4_PIN)
#endif
};

bool relayHysteresis[RELAYS_NO];

#ifndef PSTR_TO_F
#define PSTR_TO_F(s) reinterpret_cast<const __FlashStringHelper *> (s)
#endif
//~ #define F_TO_PSTR(s) reinterpret_cast<PGM_P> (s)
//~ #define FlashString const __FlashStringHelper *


void checkAndFormatEEPROM () {
	unsigned long magic;

#if defined (ARDUINO_ARCH_ESP32) || defined (ARDUINO_ARCH_ESP8266)
	EEPROM.begin (2048);
#endif

	// The casts here are very important
	EEPROM.get (0, magic);
	if (magic != EEPROM_MAGIC) {
		// Need to format EEPROM
		WDPRINT (F("Formatting EEPROM (Wrong Magic: 0x"));
		WDPRINT (magic, HEX);
		WDPRINTLN (F(")"));

		// Magic header
		EEPROM.put (0, EEPROM_MAGIC);

		// Relay data
		for (byte i = 0; i < RELAYS_NO; i++) {
			relays[i].setDefaults ();
			relays[i].writeOptions ();
#ifdef ENABLE_TIME
			relays[i].schedule.clear ();
#endif
		}

		// Network configuration
		EEPROM.put (EEPROM_MAC_ADDR, DEFAULT_MAC_ADDRESS_B1);
		EEPROM.put (EEPROM_MAC_ADDR + 1, DEFAULT_MAC_ADDRESS_B2);
		EEPROM.put (EEPROM_MAC_ADDR + 2, DEFAULT_MAC_ADDRESS_B3);
		EEPROM.put (EEPROM_MAC_ADDR + 3, DEFAULT_MAC_ADDRESS_B4);
		EEPROM.put (EEPROM_MAC_ADDR + 4, DEFAULT_MAC_ADDRESS_B5);
		EEPROM.put (EEPROM_MAC_ADDR + 5, DEFAULT_MAC_ADDRESS_B6);
		EEPROM.put (EEPROM_NETMODE_ADDR, DEFAULT_NET_MODE);
		EEPROM.put (EEPROM_IP_ADDR, DEFAULT_IP_ADDRESS_B1);
		EEPROM.put (EEPROM_IP_ADDR + 1, DEFAULT_IP_ADDRESS_B2);
		EEPROM.put (EEPROM_IP_ADDR + 2, DEFAULT_IP_ADDRESS_B3);
		EEPROM.put (EEPROM_IP_ADDR + 3, DEFAULT_IP_ADDRESS_B4);
		EEPROM.put (EEPROM_NETMASK_ADDR, DEFAULT_NETMASK_B1);
		EEPROM.put (EEPROM_NETMASK_ADDR + 1, DEFAULT_NETMASK_B2);
		EEPROM.put (EEPROM_NETMASK_ADDR + 2, DEFAULT_NETMASK_B3);
		EEPROM.put (EEPROM_NETMASK_ADDR + 3, DEFAULT_NETMASK_B4);
		EEPROM.put (EEPROM_GATEWAY_ADDR, DEFAULT_GATEWAY_ADDRESS_B1);
		EEPROM.put (EEPROM_GATEWAY_ADDR + 1, DEFAULT_GATEWAY_ADDRESS_B2);
		EEPROM.put (EEPROM_GATEWAY_ADDR + 2, DEFAULT_GATEWAY_ADDRESS_B3);
		EEPROM.put (EEPROM_GATEWAY_ADDR + 3, DEFAULT_GATEWAY_ADDRESS_B4);

#if defined (ARDUINO_ARCH_ESP32) || defined (ARDUINO_ARCH_ESP8266)
		EEPROM.commit ();
#endif

		WDPRINTLN (F("EEPROM Format complete"));
	} else {
		WDPRINTLN (F("EEPROM OK"));
	}
}


/******************************************************************************
 * DEFINITION OF PAGES                                                        *
 ******************************************************************************/

// Make sure to compile the pages with the proper #defines!
const Page page01 PROGMEM = {about_html_name, about_html, about_html_len};
const Page page02 PROGMEM = {index_html_name, index_html, index_html_len};
const Page page03 PROGMEM = {left_html_name, left_html, left_html_len};
const Page page04 PROGMEM = {main_html_name, main_html, main_html_len};
const Page page05 PROGMEM = {net_html_name, net_html, net_html_len};
const Page page06 PROGMEM = {opts_html_name, opts_html, opts_html_len};
const Page page07 PROGMEM = {sck_html_name, sck_html, sck_html_len};


const Page* const pages[] PROGMEM = {
	&page01,
	&page02,
	&page03,
	&page04,
	&page05,
	&page06,
	&page07,
	NULL
};


/******************************************************************************
 * PAGE FUNCTIONS                                                             *
 ******************************************************************************/

#ifndef ENABLE_PAGE_FUNCTIONS
#error "Please define ENABLE_PAGE_FUNCTIONS in webbino_config.h"
#endif


/* This is out own stripped-down, nothing-compliant version of strtol(). It
 * converts a string to an integer (not to a long!) in an arbitrary (< 37) base
 * with minimal error checking. It has issues (similar to the standard atoi()
 * function), but it is good enough for our needs.
 *
 * We use this because this is the only place where we need such a function,
 * and the standard strtol() makes the binary 1270 bytes bigger! It was a nice
 * discovery, since I originally wanted to avoid the usage of the String class,
 * which is so big...
 *
 * Inspired by DJ Delorie's implementation of strtoll().
 *
 * We can probably use this to replace atoi() to save even more.
 */
#define atoi(x) my_strtoi (x, 10)
int my_strtoi (const char *nptr, int base) {
	int c, acc;

	for (acc = 0, c = *nptr; c; c = *++nptr) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha (c))
			c -= isupper (c) ? 'A' - 10 : 'a' - 10;
		else
			break;

		if (c >= base)
			break;

		acc *= base;
		acc += c;
	}

	return acc;
}

bool tokenize (const char *str, PGM_P sep, byte *buffer, size_t bufsize, int base) {
	byte count;
	bool ret;

	count = 0;
	for (const char *str2 = str; (str2 = strstr_P (str2, sep)); ++str2)
		++count;

	if (count != bufsize - 1) {
		WDPRINT (F("Cannot tokenize string: "));
		WDPRINTLN (str);
		ret = false;
	} else {
		for (byte i = 0; i < bufsize; ++i) {
			buffer[i] = my_strtoi (str, base);
			str = strstr_P (str, sep) + 1;
		}
		ret = true;
	}

	return ret;
}

void netconfig_func (HTTPRequestParser& request) {
	char *param;
	byte buf[MAC_SIZE], i;

	param = request.get_parameter (F("mac"));
	if (strlen (param) > 0) {
		if (tokenize (param, PSTR ("%3A"), buf, MAC_SIZE, 16)) {		// ':' gets encoded to "%3A" when submitting the form
			for (i = 0; i < MAC_SIZE; i++)
				EEPROM.put (EEPROM_MAC_ADDR + i, buf[i]);
		}
	}

	param = request.get_parameter (F("mode"));
	if (strlen (param) > 0) {
		if (strcmp_P (param, PSTR ("static")) == 0)
			EEPROM.put (EEPROM_NETMODE_ADDR, NETMODE_STATIC);
		else
			EEPROM.put (EEPROM_NETMODE_ADDR, NETMODE_DHCP);
	}

	param = request.get_parameter (F("ip"));
	if (strlen (param) > 0 && tokenize (param, PSTR ("."), buf, IP_SIZE, 10)) {
		for (i = 0; i < IP_SIZE; i++)
			EEPROM.put (EEPROM_IP_ADDR + i, buf[i]);
	}

	param = request.get_parameter (F("mask"));
	if (strlen (param) > 0 && tokenize (param, PSTR ("."), buf, IP_SIZE, 10)) {
		for (i = 0; i < IP_SIZE; i++)
			EEPROM.put (EEPROM_NETMASK_ADDR + i, buf[i]);
	}

	param = request.get_parameter (F("gw"));
	if (strlen (param) > 0 && tokenize (param, PSTR ("."), buf, IP_SIZE, 10)) {
		for (i = 0; i < IP_SIZE; i++)
			EEPROM.put (EEPROM_GATEWAY_ADDR + i, buf[i]);
	}

#if defined  (ARDUINO_ARCH_ESP32) || defined (ARDUINO_ARCH_ESP8266)
	EEPROM.commit ();
#endif

	WDPRINTLN (F("Configuration saved"));
}

void opts_func (HTTPRequestParser& request) {
	char *param;

	for (byte i = 0; i < RELAYS_NO; i++) {
		Relay& relay = relays[i];

		param = request.get_parameter (F("delay"));
		if (strlen (param) > 0) {
			relay.delay = atoi (param);
		}

		param = request.get_parameter (F("hyst"));
		if (strlen (param) > 0) {
			relay.hysteresis = atoi (param) * 10;
		}

		relay.writeOptions ();
	}
}

void sck_func (HTTPRequestParser& request) {
	char *param;

#ifdef ENABLE_TIME
	rtCheckIdx = 0;
#endif

	param = request.get_parameter (F("rel"));
	if (strlen (param) > 0) {
		int relayNo = atoi (param);
		if (relayNo >= 1 && relayNo <= RELAYS_NO) {
			/* Save the last selected relay for later. I know this is crap, but...
			 * See below.
			 */
			lastSelectedRelay = relayNo;

			Relay& relay = relays[relayNo - 1];

			param = request.get_parameter (F("mode"));
			if (strlen (param) > 0) {		// Only do something if we got this parameter
				if (strcmp_P (param, PSTR ("on")) == 0) {
					relay.mode = RELMD_ON;
				} else if (strcmp_P (param, PSTR ("off")) == 0) {
					relay.mode = RELMD_OFF;
#ifdef ENABLE_THERMOMETER
				} else if (strcmp_P (param, PSTR ("temp")) == 0) {
					param = request.get_parameter (F("thres"));
					if (strcmp_P (param, PSTR ("gt")) == 0)
						relay.mode = RELMD_GT;
					else
						relay.mode = RELMD_LT;

					param = request.get_parameter (F("temp"));
					relay.threshold = atoi (param);

					param = request.get_parameter (F("units"));
					if (strcmp_P (param, PSTR ("F")) == 0)
						relay.units = TEMP_F;
					else
						relay.units = TEMP_C;

					relayHysteresis[relayNo - 1] = false;
#endif
#ifdef ENABLE_TIME
				} else if (strcmp_P (param, PSTR ("time")) == 0) {
					char tmpBuf[8];
					PString tmp (tmpBuf, sizeof (tmpBuf));
					for (byte h = 0; h < 24; ++h) {
						for (byte m = 0; m < 60; m += 15) {
							tmp.begin ();
							tmp.print ('t');
							tmp.print (h);
							tmp.print ('_');
							tmp.print (m);

							param = request.get_parameter (static_cast<const char *> (tmp));
							if (strlen (param) > 0) {
								DPRINT (F("On at"));
								DPRINTLN (tmp);
							}

							relay.schedule.set (h, m, strlen (param) > 0);
						}
					}

					relay.schedule.save ();

					relay.mode = RELMD_TIMED;
#endif
				}
			}

			relay.writeOptions ();
		}
	}
}


FileFuncAssoc (netAss, "/net.html", netconfig_func);
FileFuncAssoc (relaysAss, "/opts.html", opts_func);
FileFuncAssoc (sckAss, "/sck.html", sck_func);

FileFuncAssociationArray associations[] PROGMEM = {
	&netAss,
	&relaysAss,
	&sckAss,
	NULL
};


/******************************************************************************
 * DEFINITION OF TAGS                                                         *
 ******************************************************************************/

#ifndef ENABLE_TAGS
#error "Replacement Tags must be enabled in Webbino"
#endif

#define REP_BUFFER_LEN 32
char replaceBuffer[REP_BUFFER_LEN];
PString pBuffer (replaceBuffer, REP_BUFFER_LEN);

const char NOT_AVAIL_STR[] PROGMEM = "N/A";
const char CHECKED_STRING[] PROGMEM = "checked";
const char SELECTED_STRING[] PROGMEM = "selected=\"true\"";

#ifdef ENABLE_TIME

PString& evaluate_time (void *data) {
	int x;

	(void) data;

	if (timeStatus () != timeNotSet) {
		time_t t = now ();

		x = hour (t);
		if (x < 10)
			pBuffer.print ('0');
		pBuffer.print (x);
		pBuffer.print (':');

		x = minute (t);
		if (x < 10)
			pBuffer.print ('0');
		pBuffer.print (x);
		pBuffer.print (':');

		x = second (t);
		if (x < 10)
			pBuffer.print ('0');
		pBuffer.print (x);
	} else {
		pBuffer.print (NOT_AVAIL_STR);
	}

	return pBuffer;
}

PString& evaluate_date (void *data) {
	int x;

	(void) data;

	if (timeStatus () != timeNotSet) {
		time_t t = now ();

		x = day (t);
		if (x < 10)
			pBuffer.print ('0');
		pBuffer.print (x);

		pBuffer.print ('/');

		x = month (t);
		if (x < 10)
			pBuffer.print ('0');
		pBuffer.print (x);

		pBuffer.print ('/');

		x = year (t);
		pBuffer.print (x);
		} else {
		pBuffer.print (NOT_AVAIL_STR);
	}


	return pBuffer;
}

/* We're cheating quite a bit here, as we assume this function will be called
 * sequentially for every checkbox, so we can calculate the hour and minute for
 * the checkbox we are currently being called for.
 *
 * PS: rtCheckIdx is first zero'ed in sck_func().
 */
PString& evaluate_relay_time_checked (void *data) {
	(void) data;

	if (lastSelectedRelay >= 1 && lastSelectedRelay <= RELAYS_NO) {
		byte h = rtCheckIdx % 24;
		byte m = (rtCheckIdx / 24) * 15;

		if (relays[lastSelectedRelay - 1].schedule.get (h, m)) {
			pBuffer.print (CHECKED_STRING);
		}

		++rtCheckIdx;
	}

 	return pBuffer;
}

#endif	// ENABLE_TIME


#ifdef ENABLE_THERMOMETER
static PString& evaluate_temp_deg (void *data __attribute__ ((unused))) {
	if (thermometer.available) {
		Temperature& temp = thermometer.getTemp ();
		if (temp.valid)
			pBuffer.print (temp.celsius, 2);
		else
			pBuffer.print (PSTR_TO_F (NOT_AVAIL_STR));
	} else {
		pBuffer.print (PSTR_TO_F (NOT_AVAIL_STR));
	}

	return pBuffer;
}

static PString& evaluate_temp_fahr (void *data __attribute__ ((unused))) {
	if (thermometer.available) {
		Temperature& temp = thermometer.getTemp ();
		if (temp.valid)
			pBuffer.print (temp.toFahrenheit (), 2);
		else
			pBuffer.print (PSTR_TO_F (NOT_AVAIL_STR));
	} else {
		pBuffer.print (PSTR_TO_F (NOT_AVAIL_STR));
	}

	return pBuffer;
}
#endif

PString& evaluate_ip (void *data __attribute__ ((unused))) {
 	pBuffer.print (netint.getIP ());

 	return pBuffer;
}

PString& evaluate_netmask (void *data __attribute__ ((unused))) {
	pBuffer.print (netint.getNetmask ());

 	return pBuffer;
}

PString& evaluate_gw (void *data __attribute__ ((unused))) {
	pBuffer.print (netint.getGateway ());

 	return pBuffer;
}

PString& evaluate_mac_addr (void *data __attribute__ ((unused))) {
	const byte *buf = netint.getMAC ();

	for (byte i = 0; i < 6; i++) {
		if (buf[i] < 16)
			pBuffer.print ('0');
		pBuffer.print (buf[i], HEX);

		if (i < 5)
			pBuffer.print (':');
	}

	return pBuffer;
}

PString& evaluate_netmode (void *data) {
	NetworkMode netmode;
	int checkedMode = reinterpret_cast<int> (data);

	EEPROM.get (EEPROM_NETMODE_ADDR, netmode);
	if (netmode == checkedMode)
		pBuffer.print (PSTR_TO_F (CHECKED_STRING));

	return pBuffer;
}

PString& evaluate_relay_status (void *data) {
	int relayNo = reinterpret_cast<int> (data);
	if (relayNo >= 1 && relayNo <= RELAYS_NO) {
		if (relays[relayNo - 1].enabled) {
			pBuffer.print (F("ON"));
		} else {
			pBuffer.print (F("OFF"));
		}
	}

	return pBuffer;
}

/* OK, this works in a quite crap way, but it's going to work as long as a
 * single instance of the web interface is being used, which is likely our
 * case.
 *
 * PS: lastSelectedRelay is saved in sck_func().
 */
static PString& evaluate_relay_onoff_checked (void *data) {
	if (lastSelectedRelay >= 1 && lastSelectedRelay <= RELAYS_NO) {
		int md = reinterpret_cast<int> (data);			// If we cast to RelayMode it won't compile, nevermind!
		if (relays[lastSelectedRelay - 1].mode == md)
			pBuffer.print (PSTR_TO_F (CHECKED_STRING));
	}

	return pBuffer;
}


#ifdef ENABLE_THERMOMETER

PString& evaluate_relay_temp_checked (void *data __attribute__ ((unused))) {
	if (lastSelectedRelay >= 1 && lastSelectedRelay <= RELAYS_NO) {
		if (relays[lastSelectedRelay - 1].mode == RELMD_GT || relays[lastSelectedRelay - 1].mode == RELMD_LT)
			pBuffer.print (PSTR_TO_F (CHECKED_STRING));
	}

	return pBuffer;
}

PString& evaluate_relay_temp_gtlt_checked (void *data) {
	if (lastSelectedRelay >= 1 && lastSelectedRelay <= RELAYS_NO) {
		int md = static_cast<RelayMode> (reinterpret_cast<int> (data));		// ;)
		if (relays[lastSelectedRelay - 1].mode == md)
			pBuffer.print (PSTR_TO_F (CHECKED_STRING));
	}
	return pBuffer;
}

PString& evaluate_relay_temp_threshold (void *data __attribute__ ((unused))) {
	if (lastSelectedRelay >= 1 && lastSelectedRelay <= RELAYS_NO)
		pBuffer.print (relays[lastSelectedRelay - 1].threshold);

	return pBuffer;
}

PString& evaluate_relay_temp_units_c_checked (void *data __attribute__ ((unused))) {
	if (lastSelectedRelay >= 1 && lastSelectedRelay <= RELAYS_NO) {
		if (relays[lastSelectedRelay - 1].units == TEMP_C)
			pBuffer.print (PSTR_TO_F (SELECTED_STRING));
	}

	return pBuffer;
}

PString& evaluate_relay_temp_units_f_checked (void *data __attribute__ ((unused))) {
	if (lastSelectedRelay >= 1 && lastSelectedRelay <= RELAYS_NO) {
		if (relays[lastSelectedRelay - 1].units == TEMP_F)
			pBuffer.print (PSTR_TO_F (SELECTED_STRING));
	}

	return pBuffer;
}

PString& evaluate_relay_temp_delay (void *data __attribute__ ((unused))) {
	// Always use first relay's data
	pBuffer.print (relays[0].delay);

	return pBuffer;
}

PString& evaluate_relay_temp_margin (void *data __attribute__ ((unused))) {
	// Always use first relay's data
	pBuffer.print (relays[0].hysteresis / 10);

	return pBuffer;
}

#endif		// ENABLE_THERMOMETER

PString& evaluate_version (void *data __attribute__ ((unused))) {
	pBuffer.print (PROGRAM_VERSION);
	return pBuffer;
}

PString& evaluate_uptime (void *data __attribute__ ((unused))) {
	unsigned long uptime = millis () / 1000;
	byte d, h, m, s;

	d = uptime / 86400;
	uptime %= 86400;
	h = uptime / 3600;
	uptime %= 3600;
	m = uptime / 60;
	uptime %= 60;
	s = uptime;

	if (d > 0) {
		pBuffer.print (d);
		pBuffer.print (d == 1 ? F(" day, ") : F(" days, "));
	}

	if (h < 10)
		pBuffer.print ('0');
	pBuffer.print (h);
	pBuffer.print (':');
	if (m < 10)
		pBuffer.print ('0');
	pBuffer.print (m);
	pBuffer.print (':');
	if (s < 10)
		pBuffer.print ('0');
	pBuffer.print (s);

	return pBuffer;
}


#ifdef __arm__
#include <malloc.h>		// For struct mallinfo
#include <unistd.h>		// For sbrk()
#endif

static PString& evaluate_free_ram (void *data __attribute__ ((unused))) {
#ifdef __arm__
	// http://forum.arduino.cc/index.php?topic=404908
	struct mallinfo mi = mallinfo ();
	char* heapend = (char*) sbrk (0);
	register char* stack_ptr asm ("sp");
	pBuffer.print (stack_ptr - heapend + mi.fordblks);
#elif defined (ARDUINO_ARCH_ESP32) || defined (ARDUINO_ARCH_ESP8266)
	pBuffer.print (ESP.getFreeHeap ());
#elif defined (ARDUINO_ARCH_AVR)
	// http://playground.arduino.cc/Code/AvailableMemory
	extern int __heap_start, *__brkval;
	int v;

	pBuffer.print ((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
#else
	pBuffer.print (NOT_AVAIL_STR);
#endif

	return pBuffer;
}


// Max length of these is MAX_TAG_LEN (24)
EasyReplacementTag (tagMacAddr, NET_MAC, evaluate_mac_addr);
EasyReplacementTag (tagIPAddress, NET_IP, evaluate_ip);
EasyReplacementTag (tagNetmask, NET_MASK, evaluate_netmask);
EasyReplacementTag (tagGateway, NET_GW, evaluate_gw);
EasyReplacementTag (tagVer, VERSION, evaluate_version);
EasyReplacementTag (tagUptime, UPTIME, evaluate_uptime);
EasyReplacementTag (tagFreeRAM, FREERAM, evaluate_free_ram);

EasyReplacementTag (tagNMDHCP, NETMODE_DHCP_CHK, evaluate_netmode, reinterpret_cast<void *> (NETMODE_DHCP));
EasyReplacementTag (tagNMStatic, NETMODE_STATIC_CHK, evaluate_netmode, reinterpret_cast<void *> (NETMODE_STATIC));
EasyReplacementTag (tagRelayOn, RELAY_ON_CHK, evaluate_relay_onoff_checked, reinterpret_cast<void *> (RELMD_ON));
EasyReplacementTag (tagRelayOff, RELAY_OFF_CHK, evaluate_relay_onoff_checked, reinterpret_cast<void *> (RELMD_OFF));

EasyReplacementTag (tagRelay1Status, RELAY1_ST, evaluate_relay_status, reinterpret_cast<void *> (1));
EasyReplacementTag (tagRelay2Status, RELAY2_ST, evaluate_relay_status, reinterpret_cast<void *> (2));
EasyReplacementTag (tagRelay3Status, RELAY3_ST, evaluate_relay_status, reinterpret_cast<void *> (3));
EasyReplacementTag (tagRelay4Status, RELAY4_ST, evaluate_relay_status, reinterpret_cast<void *> (4));

#ifdef ENABLE_THERMOMETER
EasyReplacementTag (tagDegC, DEGC, evaluate_temp_deg);
EasyReplacementTag (tagDegF, DEGF, evaluate_temp_fahr);
EasyReplacementTag (tagRelay, RELAY_TEMP_CHK, evaluate_relay_temp_checked);
EasyReplacementTag (tagRelayTempGT, RELAY_TGT_CHK, evaluate_relay_temp_gtlt_checked, reinterpret_cast<void *> (RELMD_GT));
EasyReplacementTag (tagRelayTempLT, RELAY_TLT_CHK, evaluate_relay_temp_gtlt_checked, reinterpret_cast<void *> (RELMD_LT));
EasyReplacementTag (tagRelayTempThreshold, RELAY_THRES, evaluate_relay_temp_threshold);
EasyReplacementTag (tagRelayTempUnitsC, RELAY_TEMPC_CHK, evaluate_relay_temp_units_c_checked);
EasyReplacementTag (tagRelayTempUnitsF, RELAY_TEMPF_CHK, evaluate_relay_temp_units_f_checked);
EasyReplacementTag (tagRelayTempDelay, RELAY_DELAY, evaluate_relay_temp_delay);
EasyReplacementTag (tagRelayTempMargin, RELAY_MARGIN, evaluate_relay_temp_margin);
#endif

#ifdef ENABLE_TIME
EasyReplacementTag (tagDateStr, DATE, evaluate_date);
EasyReplacementTag (tagTimeStr, TIME, evaluate_time);
EasyReplacementTag (tagRelayTimed, RELAY_TIME_CHK, evaluate_relay_onoff_checked, reinterpret_cast<void *> (RELMD_TIMED));
EasyReplacementTag (tagRelayTimeChecked, RTIME_CHK, evaluate_relay_time_checked);
#endif


EasyReplacementTagArray tags[] PROGMEM = {
	&tagMacAddr,
	&tagIPAddress,
	&tagNetmask,
	&tagGateway,
	&tagVer,
	&tagUptime,
	&tagFreeRAM,

	&tagNMDHCP,
	&tagNMStatic,
	&tagRelayOn,
	&tagRelayOff,

	&tagRelay1Status,
	&tagRelay2Status,
	&tagRelay3Status,
	&tagRelay4Status,

#ifdef ENABLE_THERMOMETER
	&tagDegC,
	&tagDegF,
    &tagRelay,
    &tagRelayTempGT,
    &tagRelayTempLT,
    &tagRelayTempThreshold,
    &tagRelayTempUnitsC,
    &tagRelayTempUnitsF,
    &tagRelayTempDelay,
	&tagRelayTempMargin,
#endif

#ifdef ENABLE_TIME
	&tagDateStr,
	&tagTimeStr,
	&tagRelayTimed,
	&tagRelayTimeChecked,
#endif

	NULL
};


/******************************************************************************
 * MAIN STUFF                                                                 *
 ******************************************************************************/

boolean authorize (const char *user, const char *passwd) {
	WDPRINT (F("Validating username \""));
	WDPRINT (user);
	WDPRINT (F("\" with password \""));
	WDPRINT (passwd);
	WDPRINTLN ("\"");
	return strcmp_P (user, PSTR ("smart")) == 0 && strcmp_P (passwd, PSTR ("strip")) == 0;
}

void setup () {
	byte i;

	WDSTART ();
	WDPRINTLN (F("SmartStrip " PROGRAM_VERSION));
	WDPRINTLN (F("Using Webbino " WEBBINO_VERSION));

	// Check and format EEPROM, in case
	checkAndFormatEEPROM ();

	for (i = 0; i < RELAYS_NO; i++) {
		relays[i].readOptions ();
		relayHysteresis[i] = false;     // Start with no hysteresis
	}

#if defined (WEBBINO_USE_ENC28J60) || defined (WEBBINO_USE_WIZ5100) || \
      defined (WEBBINO_USE_ENC28J60_UIP)

	// Get MAC from EEPROM and init network interface
	byte mac[MAC_SIZE];

	for (i = 0; i < MAC_SIZE; i++)
		EEPROM.get (EEPROM_MAC_ADDR + i, mac[i]);
#endif

	NetworkMode netmode;
	EEPROM.get (EEPROM_NETMODE_ADDR, netmode);
	switch (netmode) {
		case NETMODE_STATIC: {
			byte ip[IP_SIZE], mask[IP_SIZE], gw[IP_SIZE];

			for (i = 0; i < IP_SIZE; i++) {
				EEPROM.get (EEPROM_IP_ADDR + i, ip[i]);
				EEPROM.get (EEPROM_NETMASK_ADDR + i, mask[i]);
				EEPROM.get (EEPROM_GATEWAY_ADDR + i, gw[i]);
			}

#if defined (WEBBINO_USE_ENC28J60) || defined (WEBBINO_USE_WIZ5100) || \
    defined (WEBBINO_USE_WIZ5500) || defined (WEBBINO_USE_FISHINO)
			bool ok = netint.begin (mac, ip, gw, gw /* FIXME: DNS */, mask);
#elif defined (WEBBINO_USE_ENC28J60_UIP)
			bool ok = netint.begin (mac, ip, gw, gw /* FIXME: DNS */, mask, SS_PIN);
#elif defined (WEBBINO_USE_ESP8266) || defined (WEBBINO_USE_WIFI) || \
      defined (WEBBINO_USE_WIFI101) || defined (WEBBINO_USE_ESP8266_STANDALONE) || \
      defined (WEBBINO_USE_DIGIFI)
			// This is incredibly not possible at the moment!
			//~ swSerial.begin (9600);
			//~ bool ok = netint.begin (FIXME);
			bool ok = false;		// Always fail
#else
			#error "Unsupported network interface"
#endif
			if (!ok) {
				WDPRINTLN (F("Failed to set static IP address"));
				while (1)
					;
			} else {
				WDPRINTLN (F("Static IP setup done"));
			}
			break;
		}
		default:
			WDPRINTLN (F("Unknown netmode, defaulting to DHCP"));
			// No break here
		case NETMODE_DHCP:
			WDPRINTLN (F("Trying to get an IP address through DHCP"));
#if defined (WEBBINO_USE_ENC28J60) || defined (WEBBINO_USE_WIZ5100) || \
    defined (WEBBINO_USE_WIZ5500)
			bool ok = netint.begin (mac);
#elif defined (WEBBINO_USE_ENC28J60_UIP)
			bool ok = netint.begin (mac, SS_PIN);
#elif defined (WEBBINO_USE_ESP8266)
			swSerial.begin (9600);
			bool ok = netint.begin (swSerial, WIFI_SSID, WIFI_PASSWORD);
#elif defined (WEBBINO_USE_DIGIFI) || defined (WEBBINO_USE_FISHINO)
			bool ok = netint.begin ();
#elif defined  (WEBBINO_USE_WIFI) || defined (WEBBINO_USE_WIFI101) || \
	  defined (WEBBINO_USE_ESP8266_STANDALONE)
			bool ok = netint.begin (WIFI_SSID, WIFI_PASSWORD);
#endif
			if (!ok) {
				WDPRINTLN (F("Failed to get configuration from DHCP"));
				while (1)
					;
			} else {
				WDPRINTLN (F("DHCP configuration done:"));
				WDPRINT (F("- IP: "));
				WDPRINTLN (netint.getIP ());
				WDPRINT (F("- Netmask: "));
				WDPRINTLN (netint.getNetmask ());
				WDPRINT (F("- Default Gateway: "));
				WDPRINTLN (netint.getGateway ());
			}
			break;
	}

	// Init webserver
	webserver.begin (netint);
	webserver.enableReplacementTags (tags);
	webserver.associateFunctions (associations);

	flashStorage.begin (pages);
	webserver.addStorage (flashStorage);

	static const char *realm = "SmartStrip";
	webserver.enableAuth (realm, authorize);

#ifdef ENABLE_THERMOMETER
	thermometer.begin (THERMOMETER_PIN);
#endif

#ifdef ENABLE_TIME
	setSyncProvider (timeProvider);
#endif

	// Signal we're ready!
 	WDPRINTLN (F("SmartStrip is ready!"));

	/* Note that this might interfere with Ethernet boards that use SPI,
	 * since pin 13 is SCK.
	 *
	 * Note that on Blue Pills and NodeMCU, the led is actually active low.
	 * It might be the case on other ESP8266 boards, too.
	 */
#if defined (ARDUINO_ARCH_STM32F1) || defined (ARDUINO_ESP8266_NODEMCU)
#define LED_ON LOW
#define LED_OFF HIGH
#else
#define LED_ON HIGH
#define LED_OFF LOW
#endif
	pinMode (LED_BUILTIN, OUTPUT);
	for (int i = 0; i < 3; i++) {
		digitalWrite (LED_BUILTIN, LED_ON);
		delay (100);
		digitalWrite (LED_BUILTIN, LED_OFF);
		delay (100);
	}
}

void loop () {


	webserver.loop ();

#ifdef ENABLE_THERMOMETER
	// Update temperature
	if (thermometer.available && (millis () - lastTemperatureRequest > THERMO_READ_INTERVAL)) {
		Temperature& temp = thermometer.getTemp ();
		if (temp.valid) {
			temperature = temp.celsius;

			WDPRINT (F("Temperature is now: "));
			WDPRINT (temperature);
			WDPRINTLN (F(" *C"));

			lastTemperatureRequest = millis ();
		}
	}
#endif

	for (byte i = 0; i < RELAYS_NO; i++) {
		Relay& r = relays[i];

#ifdef ENABLE_THERMOMETER
		bool& hysteresisEnabled = relayHysteresis[i];
#endif

		switch (r.mode) {
			case RELMD_ON:
				if (!r.enabled) {
					r.setEnabled (true);
				}
				break;
			case RELMD_OFF:
				if (r.enabled) {
					r.setEnabled (false);
				}
				break;
#ifdef ENABLE_THERMOMETER
			case RELMD_GT:
				if (((!hysteresisEnabled && temperature > r.threshold) || (hysteresisEnabled && temperature > r.threshold + r.hysteresis / 10.0)) && !r.enabled) {
					r.setEnabled (true);
					hysteresisEnabled = true;
				} else if (temperature <= r.threshold && r.enabled) {
					r.setEnabled (false);
				}
				break;
			case RELMD_LT:
				if (((!hysteresisEnabled && temperature < r.threshold) || (hysteresisEnabled && temperature < r.threshold - r.hysteresis / 10.0)) && r.enabled) {
					r.setEnabled (true);
					hysteresisEnabled = true;
				} else if (temperature >= r.threshold && r.enabled) {
					r.setEnabled (false);
				}
				break;
#endif
#ifdef ENABLE_TIME
			case RELMD_TIMED: {
				boolean shouldBeOn = r.schedule.get (hour (), minute ());
				if (shouldBeOn && !r.enabled) {
					r.setEnabled (true);
				} else if (!shouldBeOn && r.enabled) {
					r.setEnabled (false);
				}
				break;
			}
#endif
			default:
				WDPRINT (F("Bad relay mode: "));
				WDPRINTLN (r.mode);
				break;
		}
	}
}
