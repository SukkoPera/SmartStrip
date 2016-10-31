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

#include <Webbino.h>
#include <EEPROM.h>
#include "debug.h"
#include "Relay.h"
#include "enums.h"
#include "common.h"
#include "html.h"

// Instantiate the WebServer
WebServer webserver;

// Instantiate the network interface defined in the Webbino headers
#if defined (WEBBINO_USE_ENC28J60)
	#include <WebbinoInterfaces/ENC28J60.h>
	NetworkInterfaceENC28J60 netint;
#elif defined (WEBBINO_USE_WIZ5100) || defined (WEBBINO_USE_WIZ5500)
	#include <WebbinoInterfaces/WIZ5x00.h>
	NetworkInterfaceWIZ5x00 netint;
#elif defined (WEBBINO_USE_ESP8266)
	#include <WebbinoInterfaces/ESP8266.h>

	#include <SoftwareSerial.h>
	SoftwareSerial swSerial (7, 8);

	// Wi-Fi parameters
	#define WIFI_SSID        "ssid"
	#define WIFI_PASSWORD    "password"

	NetworkInterfaceESP8266 netint;
#elif defined (WEBBINO_USE_WIFI101)
	#include <WebbinoInterfaces/WiFi.h>

	// Wi-Fi parameters
	#define WIFI_SSID        "ssid"
	#define WIFI_PASSWORD    "password"

	NetworkInterfaceWiFi netint;
#elif defined (WEBBINO_USE_DIGIFI)
	#include <WebServer_DigiFi.h>
	NetworkInterfaceDigiFi netint;
#endif

#ifdef ENABLE_THERMOMETER

#include <Thermometer.h>

// Instantiate the thermometer
Thermometer thermometer;

// Current temperature (Start with average ambient temperature)
float temperature = 25;

// Time the last temperature request was issued
unsigned long lastTemperatureRequest = 0;
#endif

// Other stuff
byte lastSelectedRelay;

Relay relays[RELAYS_NO] = {
	Relay (1, RELAY1_PIN),
	Relay (2, RELAY2_PIN),
	Relay (3, RELAY3_PIN),
	Relay (4, RELAY4_PIN)
};

bool relayHysteresis[RELAYS_NO];


#define PSTR_TO_F(s) reinterpret_cast<const __FlashStringHelper *> (s)
//~ #define F_TO_PSTR(s) reinterpret_cast<PGM_P> (s)
//~ #define FlashString const __FlashStringHelper *


void checkAndFormatEEPROM () {
	unsigned long magic;

	// The casts here are very important
	EEPROM.get (0, magic);
	if (magic != EEPROM_MAGIC) {
		// Need to format EEPROM
		DPRINT (F("Formatting EEPROM (Wrong Magic: 0x"));
		DPRINT (magic, HEX);
		DPRINTLN (F(")"));

		// Magic header
		EEPROM.put (0, EEPROM_MAGIC);

		// Relay data
		for (byte i = 0; i < RELAYS_NO; i++) {
			relays[i].setDefaults ();
			relays[i].writeOptions ();
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

		DPRINTLN (F("EEPROM Format complete"));
	} else {
		DPRINTLN (F("EEPROM OK"));
	}
}


/******************************************************************************
 * DEFINITION OF PAGES                                                        *
 ******************************************************************************/

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
		DPRINT (F("Cannot tokenize string: "));
		DPRINTLN (str);
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

	DPRINTLN (F("Configuration saved"));
}

void relconfig_func (HTTPRequestParser& request) {
	char *param;

	param = request.get_parameter (F("delay"));
	if (strlen (param) > 0) {
		for (byte i = 0; i < RELAYS_NO; i++) {
			Relay& relay = relays[i];

			relay.delay = atoi (param);

			param = request.get_parameter (F("hyst"));
			if (strlen (param) > 0)
				relay.hysteresis = atoi (param) * 10;
		}
	}
}

void sck_func (HTTPRequestParser& request) {
	char *param;

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
				}
			}
		}
	}
}

const Page aboutPage PROGMEM = {about_html_name, about_html, NULL};
const Page indexPage PROGMEM = {index_html_name, index_html, NULL};
const Page leftPage PROGMEM = {left_html_name, left_html, NULL};
const Page netconfigPage PROGMEM = {net_html_name, net_html, netconfig_func};
const Page relconfigPage PROGMEM = {relays_html_name, relays_html, relconfig_func};
const Page sckPage PROGMEM = {sck_html_name, sck_html, sck_func};
const Page welcomePage PROGMEM = {main_html_name, main_html, NULL};

const Page* const pages[] PROGMEM = {
	&aboutPage,
	&indexPage,
	&leftPage,
	&netconfigPage,
	&relconfigPage,
	&sckPage,
	&welcomePage,
 	NULL
};


/******************************************************************************
 * DEFINITION OF TAGS                                                         *
 ******************************************************************************/

#ifdef ENABLE_TAGS

#define REP_BUFFER_LEN 32
char replaceBuffer[REP_BUFFER_LEN];
PString pBuffer (replaceBuffer, REP_BUFFER_LEN);

const char NOT_AVAIL_STR[] PROGMEM = "N/A";

#ifdef USE_ARDUINO_TIME_LIBRARY

PString& evaluate_time (void *data __attribute__ ((unused))) {
	int x;

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

	return pBuffer;
}

// FIXME
PString& evaluate_date (void *data __attribute__ ((unused))) {
	int x;

	time_t t = now ();

	x = day (t);
	pBuffer[0] = (x / 10) + '0';
	pBuffer[1] = (x % 10) + '0';

	pBuffer[2] = '/';

	x = month (t);
	pBuffer[3] = (x / 10) + '0';
	pBuffer[4] = (x % 10) + '0';

	pBuffer[5] = '/';

	x = year (t);
	pBuffer[6] = (x / 1000) + '0';
	pBuffer[7] = ((x % 1000) / 100) + '0';
	pBuffer[8] = ((x % 100) / 10) + '0';
	pBuffer[9] = (x % 10) + '0';

	pBuffer[10] = '\0';

	return pBuffer;
}

#endif	// USE_ARDUINO_TIME_LIBRARY


/* This is a modified version of the floatToString posted by the Arduino forums
 * user "zitron" at http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1205038401.
 * This is slimmer than dstrtof() (350 vs. 1700 bytes!) and works well enough
 * for our needs, so here we go!
 *
 * NOTE: Precision is fixed to two digits, which is just what we need.
 */
//~ char *floatToString (double val, char *outstr) {
	//~ char temp[8];
	//~ unsigned long frac;

	//~ temp[0] = '\0';
	//~ outstr[0] = '\0';

	//~ if (val < 0.0){
		//~ strcpy (outstr, "-\0");  //print "-" sign
		//~ val *= -1;
	//~ }

	//~ val += 0.005;		// Round

	//~ strcat (outstr, itoa ((int) val, temp, 10));  //prints the integer part without rounding
	//~ strcat (outstr, ".\0");		// print the decimal point

	//~ frac = (val - (int) val) * 100;

	//~ if (frac < 10)
		//~ strcat (outstr, "0\0");    // print padding zeros

	//~ strcat (outstr, itoa (frac, temp, 10));  // print fraction part

	//~ return outstr;
//~ }

#ifdef ENABLE_THERMOMETER
PString& evaluate_temp_deg (void *data __attribute__ ((unused))) {
	Temperature& temp = thermometer.getTemp ();
	if (temp.valid)
		pBuffer.print (temp.celsius, 2);
	else
		pBuffer.print (PSTR_TO_F (NOT_AVAIL_STR));

	return pBuffer;
}

PString& evaluate_temp_fahr (void *data __attribute__ ((unused))) {
	Temperature& temp = thermometer.getTemp ();
	if (temp.valid)
		pBuffer.print (temp.toFahrenheit (), 2);
	else
		pBuffer.print (PSTR_TO_F (NOT_AVAIL_STR));

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


const char CHECKED_STRING[] PROGMEM = "checked";
const char SELECTED_STRING[] PROGMEM = "selected=\"true\"";

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
		switch (relays[relayNo - 1].state) {
			case RELAY_ON:
				pBuffer.print (F("ON"));
				break;
			case RELAY_OFF:
				pBuffer.print (F("OFF"));
				break;
			default:
				break;
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
PString& evaluate_relay_onoff_checked (void *data __attribute__ ((unused))) {
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

// See http://playground.arduino.cc/Code/AvailableMemory
PString& evaluate_free_ram (void *data __attribute__ ((unused))) {
#ifdef __arm__
	// We haven't found a reliable way for this on the Due, yet
	pBuffer.print (F("Unknown"));
#else
	extern int __heap_start, *__brkval;
	int v;

	pBuffer.print ((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
#endif

	return pBuffer;
}


// Max length of these is MAX_TAG_LEN (24)
#ifdef USE_ARDUINO_TIME_LIBRARY
const char subDateStr[] PROGMEM = "DATE";
const char subTimeStr[] PROGMEM = "TIME";
#endif
const char subMacAddrStr[] PROGMEM = "MACADDR";
const char subIPAddressStr[] PROGMEM = "NET_IP";
const char subNetmaskStr[] PROGMEM = "NET_MASK";
const char subGatewayStr[] PROGMEM = "NET_GW";
const char subNMDHCPStr[] PROGMEM = "NETMODE_DHCP_CHK";
const char subNMStaticStr[] PROGMEM = "NETMODE_STATIC_CHK";
const char subRelayOnStr[] PROGMEM = "RELAY_ON_CHK";
const char subRelayOffStr[] PROGMEM = "RELAY_OFF_CHK";
const char subRelay1StatusStr[] PROGMEM = "RELAY1_ST";
const char subRelay2StatusStr[] PROGMEM = "RELAY2_ST";
const char subRelay3StatusStr[] PROGMEM = "RELAY3_ST";
const char subRelay4StatusStr[] PROGMEM = "RELAY4_ST";
#ifdef ENABLE_THERMOMETER
const char subDegCStr[] PROGMEM = "DEGC";
const char subDegFStr[] PROGMEM = "DEGF";
const char subRelayTempStr[] PROGMEM = "RELAY_TEMP_CHK";
const char subRelayTempGTStr[] PROGMEM = "RELAY_TGT_CHK";
const char subRelayTempLTStr[] PROGMEM = "RELAY_TLT_CHK";
const char subRelayTempThresholdStr[] PROGMEM = "RELAY_THRES";
const char subRelayTempUnitsCStr[] PROGMEM = "RELAY_TEMPC_CHK";
const char subRelayTempUnitsFStr[] PROGMEM = "RELAY_TEMPF_CHK";
const char subRelayTempDelayStr[] PROGMEM = "RELAY_DELAY";
const char subRelayTempMarginStr[] PROGMEM = "RELAY_MARGIN";
#endif
const char subVerStr[] PROGMEM = "VERSION";
const char subUptimeStr[] PROGMEM = "UPTIME";
const char subFreeRAMStr[] PROGMEM = "FREERAM";

#ifdef USE_ARDUINO_TIME_LIBRARY
const ReplacementTag subDateVarSub PROGMEM = {subDateStr, evaluate_date, NULL};
const ReplacementTag subTimeVarSub PROGMEM =	{subTimeStr, evaluate_time, NULL};
#endif
const ReplacementTag subMacAddrVarSub PROGMEM = {subMacAddrStr, evaluate_mac_addr, NULL};
const ReplacementTag subIPAddressVarSub PROGMEM = {subIPAddressStr, evaluate_ip, NULL};
const ReplacementTag subNetmaskVarSub PROGMEM = {subNetmaskStr, evaluate_netmask, NULL};
const ReplacementTag subGatewayVarSub PROGMEM = {subGatewayStr, evaluate_gw, NULL};
const ReplacementTag subNMDHCPVarSub PROGMEM = {subNMDHCPStr, evaluate_netmode, reinterpret_cast<void *> (NETMODE_DHCP)};
const ReplacementTag subNMStaticVarSub PROGMEM = {subNMStaticStr, evaluate_netmode, reinterpret_cast<void *> (NETMODE_STATIC)};
const ReplacementTag subRelayOnVarSub PROGMEM = {subRelayOnStr, evaluate_relay_onoff_checked, reinterpret_cast<void *> (RELMD_ON)};
const ReplacementTag subRelayOffVarSub PROGMEM = {subRelayOffStr, evaluate_relay_onoff_checked, reinterpret_cast<void *> (RELMD_OFF)};
const ReplacementTag subRelay1StatusVarSub PROGMEM = {subRelay1StatusStr, evaluate_relay_status, reinterpret_cast<void *> (1)};
const ReplacementTag subRelay2StatusVarSub PROGMEM = {subRelay2StatusStr, evaluate_relay_status, reinterpret_cast<void *> (2)};
const ReplacementTag subRelay3StatusVarSub PROGMEM = {subRelay3StatusStr, evaluate_relay_status, reinterpret_cast<void *> (3)};
const ReplacementTag subRelay4StatusVarSub PROGMEM = {subRelay4StatusStr, evaluate_relay_status, reinterpret_cast<void *> (4)};
#ifdef ENABLE_THERMOMETER
const ReplacementTag subDegCVarSub PROGMEM = {subDegCStr, evaluate_temp_deg, NULL};
const ReplacementTag subDegFVarSub PROGMEM = {subDegFStr, evaluate_temp_fahr, NULL};
const ReplacementTag subRelayTempVarSub PROGMEM = {subRelayTempStr, evaluate_relay_temp_checked, NULL};
const ReplacementTag subRelayTempGTVarSub PROGMEM = {subRelayTempGTStr, evaluate_relay_temp_gtlt_checked, reinterpret_cast<void *> (RELMD_GT)};
const ReplacementTag subRelayTempLTVarSub PROGMEM = {subRelayTempLTStr, evaluate_relay_temp_gtlt_checked, reinterpret_cast<void *> (RELMD_LT)};
const ReplacementTag subRelayTempThresholdVarSub PROGMEM = {subRelayTempThresholdStr, evaluate_relay_temp_threshold, NULL};
const ReplacementTag subRelayTempUnitsCVarSub PROGMEM = {subRelayTempUnitsCStr, evaluate_relay_temp_units_c_checked, NULL};
const ReplacementTag subRelayTempUnitsFVarSub PROGMEM = {subRelayTempUnitsFStr, evaluate_relay_temp_units_f_checked, NULL};
const ReplacementTag subRelayTempDelayVarSub PROGMEM = {subRelayTempDelayStr, evaluate_relay_temp_delay, NULL};
const ReplacementTag subRelayTempMarginVarSub PROGMEM = {subRelayTempMarginStr, evaluate_relay_temp_margin, NULL};
#endif
const ReplacementTag subVerVarSub PROGMEM = {subVerStr, evaluate_version, NULL};
const ReplacementTag subUptimeVarSub PROGMEM = {subUptimeStr, evaluate_uptime, NULL};
const ReplacementTag subFreeRAMVarSub PROGMEM = {subFreeRAMStr, evaluate_free_ram, NULL};

const ReplacementTag * const substitutions[] PROGMEM = {
#ifdef USE_ARDUINO_TIME_LIBRARY
	&subDateVarSub,
	&subTimeVarSub,
#endif
	&subMacAddrVarSub,
	&subIPAddressVarSub,
	&subNetmaskVarSub,
	&subGatewayVarSub,
	&subNMDHCPVarSub,
	&subNMStaticVarSub,
	&subRelayOnVarSub,
	&subRelayOffVarSub,
	&subRelay1StatusVarSub,
	&subRelay2StatusVarSub,
	&subRelay3StatusVarSub,
	&subRelay4StatusVarSub,
#ifdef ENABLE_THERMOMETER
	&subDegCVarSub,
	&subDegFVarSub,
	&subRelayTempVarSub,
	&subRelayTempGTVarSub,
	&subRelayTempLTVarSub,
	&subRelayTempThresholdVarSub,
	&subRelayTempUnitsCVarSub,
	&subRelayTempUnitsFVarSub,
	&subRelayTempDelayVarSub,
	&subRelayTempMarginVarSub,
#endif
	&subVerVarSub,
	&subUptimeVarSub,
	&subFreeRAMVarSub,
	NULL
};


/******************************************************************************
 * MAIN STUFF                                                                 *
 ******************************************************************************/

#endif


void setup () {
	byte i;

	DSTART ();
	DPRINTLN (F("SmartStrip " PROGRAM_VERSION));
	DPRINTLN (F("Using Webbino " WEBBINO_VERSION));

	// Check and format EEPROM, in case
	checkAndFormatEEPROM ();

	for (i = 0; i < RELAYS_NO; i++) {
		relays[i].readOptions ();
		relays[i].effectState ();
		relayHysteresis[i] = false;     // Start with no hysteresis
	}

#if defined (WEBBINO_USE_ENC28J60) || defined (WEBBINO_USE_WIZ5100)
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

#if defined (WEBBINO_USE_ENC28J60) || defined (WEBBINO_USE_WIZ5100)
			bool ok = netint.begin (mac, ip, gw, gw /* FIXME: DNS */, mask);
#elif defined (WEBBINO_USE_ESP8266) || defined (WEBBINO_USE_DIGIFI)
			// This is incredibly not possible at the moment!
			//~ swSerial.begin (9600);
			//~ bool ok = netint.begin (FIXME);
			bool ok = false;		// Always fail
#endif
			if (!ok) {
				DPRINTLN (F("Failed to set static IP address"));
				while (1)
					;
			} else {
				DPRINTLN (F("Static IP setup done"));
			}
			break;
		}
		default:
			DPRINTLN (F("Unknown netmode, defaulting to DHCP"));
			// No break here
		case NETMODE_DHCP:
			DPRINTLN (F("Trying to get an IP address through DHCP"));
#if defined (WEBBINO_USE_ENC28J60) || defined (WEBBINO_USE_WIZ5100)
			bool ok = netint.begin (mac);
#elif defined (WEBBINO_USE_ESP8266)
			swSerial.begin (9600);
			bool ok = netint.begin (swSerial, WIFI_SSID, WIFI_PASSWORD);
#elif defined (WEBBINO_USE_DIGIFI)
			bool ok = netint.begin ();
#endif
			if (!ok) {
				DPRINTLN (F("Failed to get configuration from DHCP"));
				while (1)
					;
			} else {
				DPRINTLN (F("DHCP configuration done:"));
				DPRINT (F("- IP: "));
				DPRINTLN (netint.getIP ());
				DPRINT (F("- Netmask: "));
				DPRINTLN (netint.getNetmask ());
				DPRINT (F("- Default Gateway: "));
				DPRINTLN (netint.getGateway ());
			}
			break;
	}

	// Init webserver
	webserver.begin (netint, pages, substitutions);

#ifdef ENABLE_THERMOMETER
	thermometer.begin (THERMOMETER_PIN);
#endif

	// Signal we're ready!
 	DPRINTLN (F("SmartStrip is ready!"));

	/* Note that this might intrfere with Ethernet boards that use SPI,
	 * since pin 13 is SCK.
	 */
	pinMode (LED_BUILTIN, OUTPUT);
	for (int i = 0; i < 3; i++) {
		digitalWrite (LED_BUILTIN, HIGH);
		delay (100);
		digitalWrite (LED_BUILTIN, LOW);
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

			DPRINT (F("Temperature is now: "));
			DPRINT (temperature);
			DPRINTLN (F(" *C"));

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
				if (r.state != RELAY_ON) {
					r.switchState (RELAY_ON);
					r.writeOptions ();
				}
				break;
			case RELMD_OFF:
				if (r.state != RELAY_OFF) {
					r.switchState (RELAY_OFF);
					r.writeOptions ();
				}
				break;
#ifdef ENABLE_THERMOMETER
			case RELMD_GT:
				if (((!hysteresisEnabled && temperature > r.threshold) || (hysteresisEnabled && temperature > r.threshold + r.hysteresis / 10.0)) && r.state != RELAY_ON) {
					r.switchState (RELAY_ON);
					r.writeOptions ();
					hysteresisEnabled = true;
				} else if (temperature <= r.threshold && r.state != RELAY_OFF) {
					r.switchState (RELAY_OFF);
					r.writeOptions ();
				}
				break;
			case RELMD_LT:
				if (((!hysteresisEnabled && temperature < r.threshold) || (hysteresisEnabled && temperature < r.threshold - r.hysteresis / 10.0)) && r.state != RELAY_ON) {
					r.switchState (RELAY_ON);
					r.writeOptions ();
					hysteresisEnabled = true;
				} else if (temperature >= r.threshold && r.state != RELAY_OFF) {
					r.switchState (RELAY_OFF);
					r.writeOptions ();
				}
				break;
#endif
			default:
				DPRINT (F("Bad relay mode: "));
				DPRINTLN (r.mode);
				break;
		}
	}
}
