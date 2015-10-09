/***************************************************************************
 *   This file is part of SmartStrip.                                      *
 *                                                                         *
 *   Copyright (C) 2012-2015 by SukkoPera                                  *
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
#ifdef USE_ENC28J60
	#include <EtherCard.h>
#else
	#include <SPI.h>
	#include <Ethernet.h>
#endif

#include <EEPROM.h>
#include <avr/pgmspace.h>
#include "debug.h"
#include "Relay.h"
#include "enums.h"
#include "common.h"
#include "html.h"

// Other stuff
byte lastSelectedRelay;


// Instantiate the WebServer
WebServer webserver;

#ifdef ENABLE_THERMOMETER

#include <Thermometer.h>
#ifdef USE_DALLAS_THERMO
	#include <OneWire.h>
	#include <DallasTemperature.h>
#elif defined USE_DHT_THERMO
	#include <DHT.h>
#endif

// Instantiate the thermometer
Thermometer thermometer;

// Time the last temperature request was issued
unsigned long lastTemperatureRequest = 0;
#endif


Relay relays[RELAYS_NO] = {
	Relay (1, RELAY1_PIN),
	Relay (2, RELAY2_PIN),
	Relay (3, RELAY3_PIN),
	Relay (4, RELAY4_PIN)
};

static bool relayHysteresis[RELAYS_NO];


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
		EEPROM.put (EEPROM_MAC_B1_ADDR, DEFAULT_MAC_ADDRESS_B1);
		EEPROM.put (EEPROM_MAC_B2_ADDR, DEFAULT_MAC_ADDRESS_B2);
		EEPROM.put (EEPROM_MAC_B3_ADDR, DEFAULT_MAC_ADDRESS_B3);
		EEPROM.put (EEPROM_MAC_B4_ADDR, DEFAULT_MAC_ADDRESS_B4);
		EEPROM.put (EEPROM_MAC_B5_ADDR, DEFAULT_MAC_ADDRESS_B5);
		EEPROM.put (EEPROM_MAC_B6_ADDR, DEFAULT_MAC_ADDRESS_B6);
		EEPROM.put (EEPROM_NETMODE_ADDR, DEFAULT_NET_MODE);
		EEPROM.put (EEPROM_IP_B1_ADDR, DEFAULT_IP_ADDRESS_B1);
		EEPROM.put (EEPROM_IP_B2_ADDR, DEFAULT_IP_ADDRESS_B2);
		EEPROM.put (EEPROM_IP_B3_ADDR, DEFAULT_IP_ADDRESS_B3);
		EEPROM.put (EEPROM_IP_B4_ADDR, DEFAULT_IP_ADDRESS_B4);
		EEPROM.put (EEPROM_NETMASK_B1_ADDR, DEFAULT_NETMASK_B1);
		EEPROM.put (EEPROM_NETMASK_B2_ADDR, DEFAULT_NETMASK_B2);
		EEPROM.put (EEPROM_NETMASK_B3_ADDR, DEFAULT_NETMASK_B3);
		EEPROM.put (EEPROM_NETMASK_B4_ADDR, DEFAULT_NETMASK_B4);
		EEPROM.put (EEPROM_GATEWAY_B1_ADDR, DEFAULT_GATEWAY_ADDRESS_B1);
		EEPROM.put (EEPROM_GATEWAY_B2_ADDR, DEFAULT_GATEWAY_ADDRESS_B2);
		EEPROM.put (EEPROM_GATEWAY_B3_ADDR, DEFAULT_GATEWAY_ADDRESS_B3);
		EEPROM.put (EEPROM_GATEWAY_B4_ADDR, DEFAULT_GATEWAY_ADDRESS_B4);

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
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
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
		for (size_t i = 0; i < bufsize; ++i) {
			buffer[i] = my_strtoi (str, base);
			str = strstr_P (str, sep) + 1;
		}
		ret = true;
	}

	return ret;
}

void netconfig_func (HTTPRequestParser& request) {
	char *param;
	byte buf[6];

	param = request.get_get_parameter (F("mac"));
	if (strlen (param) > 0) {
		if (tokenize (param, PSTR ("%3A"), buf, 6, 16)) {		// ':' gets encoded to "%3A" when submitting the form
			EEPROM.put (EEPROM_MAC_B1_ADDR, buf[0]);
			EEPROM.put (EEPROM_MAC_B2_ADDR, buf[1]);
			EEPROM.put (EEPROM_MAC_B3_ADDR, buf[2]);
			EEPROM.put (EEPROM_MAC_B4_ADDR, buf[3]);
			EEPROM.put (EEPROM_MAC_B5_ADDR, buf[4]);
			EEPROM.put (EEPROM_MAC_B6_ADDR, buf[5]);
		}
	}

	param = request.get_get_parameter (F("mode"));
	if (strlen (param) > 0) {
		if (strcmp_P (param, PSTR ("dhcp")) == 0)
			EEPROM.put (EEPROM_NETMODE_ADDR, NETMODE_DHCP);
		else if (strcmp_P (param, PSTR ("static")) == 0)
			EEPROM.put (EEPROM_NETMODE_ADDR, NETMODE_STATIC);
	}

	param = request.get_get_parameter (F("ip"));
	if (strlen (param) > 0 && tokenize (param, PSTR ("."), buf, 4, 10)) {
		EEPROM.put (EEPROM_IP_B1_ADDR, buf[0]);
		EEPROM.put (EEPROM_IP_B2_ADDR, buf[1]);
		EEPROM.put (EEPROM_IP_B3_ADDR, buf[2]);
		EEPROM.put (EEPROM_IP_B4_ADDR, buf[3]);
	}

	param = request.get_get_parameter (F("mask"));
	if (strlen (param) > 0 && tokenize (param, PSTR ("."), buf, 4, 10)) {
		EEPROM.put (EEPROM_NETMASK_B1_ADDR, buf[0]);
		EEPROM.put (EEPROM_NETMASK_B2_ADDR, buf[1]);
		EEPROM.put (EEPROM_NETMASK_B3_ADDR, buf[2]);
		EEPROM.put (EEPROM_NETMASK_B4_ADDR, buf[3]);
	}

	param = request.get_get_parameter (F("gw"));
	if (strlen (param) > 0 && tokenize (param, PSTR ("."), buf, 4, 10)) {
		EEPROM.put (EEPROM_GATEWAY_B1_ADDR, buf[0]);
		EEPROM.put (EEPROM_GATEWAY_B2_ADDR, buf[1]);
		EEPROM.put (EEPROM_GATEWAY_B3_ADDR, buf[2]);
		EEPROM.put (EEPROM_GATEWAY_B4_ADDR, buf[3]);
	}

	DPRINTLN (F("Configuration saved"));
}

void relconfig_func (HTTPRequestParser& request) {
	char *param;

	param = request.get_get_parameter (F("delay"));
	if (strlen (param) > 0) {
		for (byte i = 0; i < RELAYS_NO; i++) {
			Relay& relay = relays[i];

			relay.delay = atoi (param);

			param = request.get_get_parameter (F("hyst"));
			if (strlen (param) > 0)
				relay.hysteresis = atoi (param) * 10;
		}
	}
}

void sck_func (HTTPRequestParser& request) {
	char *param;

	param = request.get_get_parameter (F("rel"));
	if (strlen (param) > 0) {
		int relayNo = atoi (param);
		if (relayNo >= 1 && relayNo <= RELAYS_NO) {
			/* Save the last selected relay for later. I know this is crap, but...
			 * See below.
			 */
			lastSelectedRelay = relayNo;

			Relay& relay = relays[relayNo - 1];

			param = request.get_get_parameter (F("mode"));
			if (strlen (param) > 0) {		// Only do something if we got this parameter
				if (strcmp_P (param, PSTR ("on")) == 0) {
					relay.mode = RELMD_ON;
				} else if (strcmp_P (param, PSTR ("off")) == 0) {
					relay.mode = RELMD_OFF;
				} else if (strcmp_P (param, PSTR ("temp")) == 0) {
					param = request.get_get_parameter (F("thres"));
					if (strcmp_P (param, PSTR ("gt")) == 0)
						relay.mode = RELMD_GT;
					else
						relay.mode = RELMD_LT;

					param = request.get_get_parameter (F("temp"));
					relay.threshold = atoi (param);

					param = request.get_get_parameter (F("units"));
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

static const Page aboutPage PROGMEM = {about_html_name, about_html, NULL};
static const Page indexPage PROGMEM = {index_html_name, index_html, NULL};
static const Page leftPage PROGMEM = {left_html_name, left_html, NULL};
static const Page netconfigPage PROGMEM = {network_html_name, network_html, netconfig_func};
static const Page relconfigPage PROGMEM = {relays_html_name, relays_html, relconfig_func};
static const Page sckPage PROGMEM = {sck_html_name, sck_html, sck_func};
static const Page welcomePage PROGMEM = {welcome_html_name, welcome_html, NULL};

static const Page * const pages[] PROGMEM = {
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
static char replaceBuffer[REP_BUFFER_LEN];

const char NOT_AVAIL_STR[] PROGMEM = "N/A";


static char *evaluate_date (void *data) {
#ifdef USE_ARDUINO_TIME_LIBRARY
	int x;

	time_t t = now ();

	x = hour (t);
	replaceBuffer[0] = (x % 10) + '0';
	replaceBuffer[1] = (x / 10) + '0';

	replaceBuffer[2] = ':';

	x = minute (t);
	replaceBuffer[3] = (x % 10) + '0';
	replaceBuffer[4] = (x / 10) + '0';

	replaceBuffer[5] = ':';

	x = second (t);
	replaceBuffer[6] = (x % 10) + '0';
	replaceBuffer[7] = (x / 10) + '0';

	replaceBuffer[8] = '\0';
#else
	strlcpy_P (replaceBuffer, NOT_AVAIL_STR, REP_BUFFER_LEN);
#endif

	return replaceBuffer;
}

static char *evaluate_time (void *data) {
#ifdef USE_ARDUINO_TIME_LIBRARY
	int x;

	time_t t = now ();

	x = day (t);
	replaceBuffer[0] = (x / 10) + '0';
	replaceBuffer[1] = (x % 10) + '0';

	replaceBuffer[2] = '/';

	x = month (t);
	replaceBuffer[3] = (x / 10) + '0';
	replaceBuffer[4] = (x % 10) + '0';

	replaceBuffer[5] = '/';

	x = year (t);
	replaceBuffer[6] = (x / 1000) + '0';
	replaceBuffer[7] = ((x % 1000) / 100) + '0';
	replaceBuffer[8] = ((x % 100) / 10) + '0';
	replaceBuffer[9] = (x % 10) + '0';

	replaceBuffer[10] = '\0';
#else
	strlcpy_P (replaceBuffer, NOT_AVAIL_STR, REP_BUFFER_LEN);
#endif

	return replaceBuffer;
}

/* This is a modified version of the floatToString posted by the Arduino forums
 * user "zitron" at http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1205038401.
 * This is slimmer than dstrtof() (350 vs. 1700 bytes!) and works well enough
 * for our needs, so here we go!
 *
 * NOTE: Precision is fixed to two digits, which is just what we need.
 */
char *floatToString (double val, char *outstr) {
	char temp[8];
	unsigned long frac;

	temp[0] = '\0';
	outstr[0] = '\0';

	if (val < 0.0){
		strcpy (outstr, "-\0");  //print "-" sign
		val *= -1;
	}

	val += 0.005;		// Round

	strcat (outstr, itoa ((int) val, temp, 10));  //prints the integer part without rounding
	strcat (outstr, ".\0");		// print the decimal point

	frac = (val - (int) val) * 100;

	if (frac < 10)
		strcat (outstr, "0\0");    // print padding zeros

	strcat (outstr, itoa (frac, temp, 10));  // print fraction part

	return outstr;
}

#ifdef ENABLE_THERMOMETER
static char *evaluate_temp_deg (void *data) {
	Temperature& temp = thermometer.getTemp ();
	if (temp.valid)
		floatToString (temp.celsius, replaceBuffer);
	else
		strlcpy_P (replaceBuffer, NOT_AVAIL_STR, REP_BUFFER_LEN);

	return replaceBuffer;
}

static char *evaluate_temp_fahr (void *data) {
	Temperature& temp = thermometer.getTemp ();
	if (temp.valid)
		floatToString (temp.toFahrenheit (), replaceBuffer);
	else
		strlcpy_P (replaceBuffer, NOT_AVAIL_STR, REP_BUFFER_LEN);

	return replaceBuffer;
}
#endif

static char *ip2str (const byte *buf) {
	replaceBuffer[0] = '\0';
	for (int i = 0; i < 3; i++) {
		itoa (buf[i], replaceBuffer + strlen (replaceBuffer), DEC);
		strcat_P (replaceBuffer, PSTR ("."));
	}
	itoa (buf[3], replaceBuffer + strlen (replaceBuffer), DEC);

	return replaceBuffer;
}

static char *evaluate_ip (void *data) {
 	return ip2str (webserver.getIP ());
}

static char *evaluate_netmask (void *data) {
	return ip2str (webserver.getNetmask ());
}

static char *evaluate_gw (void *data) {
	return ip2str (webserver.getGateway ());
}

static char int2hex (int i) {
	if (i < 10)
		return i + '0';
	else
		return i - 10 + 'A';
}

static char *evaluate_mac_addr (void *data) {
	byte tmp;

	EEPROM.get (EEPROM_MAC_B1_ADDR, tmp);
	replaceBuffer[0] = int2hex (tmp / 16);
	replaceBuffer[1] = int2hex (tmp % 16);

	replaceBuffer[2] = ':';

	EEPROM.get (EEPROM_MAC_B2_ADDR, tmp);
	replaceBuffer[3] = int2hex (tmp / 16);
	replaceBuffer[4] = int2hex (tmp % 16);

	replaceBuffer[5] = ':';

	EEPROM.get (EEPROM_MAC_B3_ADDR, tmp);
	replaceBuffer[7] = int2hex (tmp % 16);

	replaceBuffer[8] = ':';

	EEPROM.get (EEPROM_MAC_B4_ADDR, tmp);
	replaceBuffer[9] = int2hex (tmp / 16);
	replaceBuffer[10] = int2hex (tmp % 16);

	replaceBuffer[11] = ':';

	EEPROM.get (EEPROM_MAC_B5_ADDR, tmp);
	replaceBuffer[12] = int2hex (tmp / 16);
	replaceBuffer[13] = int2hex (tmp % 16);

	replaceBuffer[14] = ':';

	EEPROM.get (EEPROM_MAC_B6_ADDR, tmp);
	replaceBuffer[15] = int2hex (tmp / 16);
	replaceBuffer[16] = int2hex (tmp % 16);

	replaceBuffer[17] = '\0';

	return replaceBuffer;
}


const char CHECKED_STRING[] PROGMEM = "checked";
const char SELECTED_STRING[] PROGMEM = "selected=\"true\"";

static char *evaluate_netmode_dhcp_checked (void *data) {
	NetworkMode netmode;
  
	EEPROM.get (EEPROM_NETMODE_ADDR, netmode);

	if (netmode == NETMODE_DHCP)
		strlcpy_P (replaceBuffer, CHECKED_STRING, REP_BUFFER_LEN);
	else
		replaceBuffer[0] = '\0';

	return replaceBuffer;
}

static char *evaluate_netmode_static_checked (void *data) {
	NetworkMode netmode;
  
	EEPROM.get (EEPROM_NETMODE_ADDR, netmode);

	if (netmode == NETMODE_STATIC)
		strlcpy_P (replaceBuffer, CHECKED_STRING, REP_BUFFER_LEN);
	else
		replaceBuffer[0] = '\0';

	return replaceBuffer;
}

static char *evaluate_relay_status (void *data) {
	replaceBuffer[0] = '\0';
	int relayNo = reinterpret_cast<int> (data);

	if (relayNo >= 1 && relayNo <= RELAYS_NO) {
		switch (relays[relayNo - 1].state) {
			case RELAY_ON:
				strlcpy_P (replaceBuffer, PSTR ("ON"), REP_BUFFER_LEN);
				break;
			case RELAY_OFF:
				strlcpy_P (replaceBuffer, PSTR ("OFF"), REP_BUFFER_LEN);
				break;
			default:
				break;
		}
	}

	return replaceBuffer;
}

/* OK, this works in a quite crap way, but it's going to work as long as a
 * single instance of the web interface is being used, which is likely our
 * case.
 *
 * PS: lastSelectedRelay is saved in sck_func().
 */
static char *evaluate_relay_onoff_checked (void *data) {
	replaceBuffer[0] = '\0';

	if (lastSelectedRelay >= 1 && lastSelectedRelay <= RELAYS_NO) {
		int md = reinterpret_cast<int> (data);			// If we cast to RelayMode it won't compile, nevermind!
		if (relays[lastSelectedRelay - 1].mode == md)
			strlcpy_P (replaceBuffer, CHECKED_STRING, REP_BUFFER_LEN);
	}

	return replaceBuffer;
}

static char *evaluate_relay_temp_checked (void *data) {
	replaceBuffer[0] = '\0';

	if (lastSelectedRelay >= 1 && lastSelectedRelay <= RELAYS_NO) {
		if (relays[lastSelectedRelay - 1].mode == RELMD_GT || relays[lastSelectedRelay - 1].mode == RELMD_LT)
			strlcpy_P (replaceBuffer, CHECKED_STRING, REP_BUFFER_LEN);
	}

	return replaceBuffer;
}

static char *evaluate_relay_temp_gt_checked (void *data) {
	replaceBuffer[0] = '\0';

	if (lastSelectedRelay >= 1 && lastSelectedRelay <= RELAYS_NO) {
		if (relays[lastSelectedRelay - 1].mode == RELMD_GT)
			strlcpy_P (replaceBuffer, SELECTED_STRING, REP_BUFFER_LEN);
	}

	return replaceBuffer;
}

static char *evaluate_relay_temp_lt_checked (void *data) {
	replaceBuffer[0] = '\0';

	if (lastSelectedRelay >= 1 && lastSelectedRelay <= RELAYS_NO) {
		if (relays[lastSelectedRelay - 1].mode == RELMD_LT)
			strlcpy_P (replaceBuffer, SELECTED_STRING, REP_BUFFER_LEN);
	}

	return replaceBuffer;
}

static char *evaluate_relay_temp_threshold (void *data) {
	replaceBuffer[0] = '\0';

	if (lastSelectedRelay >= 1 && lastSelectedRelay <= RELAYS_NO)
		itoa (relays[lastSelectedRelay - 1].threshold, replaceBuffer, DEC);

	return replaceBuffer;
}

static char *evaluate_relay_temp_units_c_checked (void *data) {
	replaceBuffer[0] = '\0';

	if (lastSelectedRelay >= 1 && lastSelectedRelay <= RELAYS_NO) {
		if (relays[lastSelectedRelay - 1].units == TEMP_C)
			strlcpy_P (replaceBuffer, SELECTED_STRING, REP_BUFFER_LEN);
	}

	return replaceBuffer;
}

static char *evaluate_relay_temp_units_f_checked (void *data) {
	replaceBuffer[0] = '\0';

	if (lastSelectedRelay >= 1 && lastSelectedRelay <= RELAYS_NO) {
		if (relays[lastSelectedRelay - 1].units == TEMP_F)
			strlcpy_P (replaceBuffer, SELECTED_STRING, REP_BUFFER_LEN);
	}

	return replaceBuffer;
}

static char *evaluate_relay_temp_delay (void *data) {
	// Always use first relay's data
	itoa (relays[0].delay, replaceBuffer, DEC);

	return replaceBuffer;
}

static char *evaluate_relay_temp_margin (void *data) {
	// Always use first relay's data
	itoa (relays[0].hysteresis / 10, replaceBuffer, DEC);

	return replaceBuffer;
}

static char *evaluate_version (void *data) {
	strlcpy (replaceBuffer, PROGRAM_VERSION, REP_BUFFER_LEN);
	return replaceBuffer;
}

// FIXME :D
char *my_itoa (int val, char *s, int base, byte width = 0) {
	char *ret;

	if (width == 2 && val < 10) {
		s[0] = '0';
		itoa (val, s + 1, base);
		ret = s;
	} else {
		ret = itoa (val, s, base);
	}

	return ret;
}

// Wahahahah! Prolly the most advanced function of its kind!
// FIXME: Save some bytes removing temp vars.
// FIXME: Check that string does not overflow buffer (which is likely!)
static char *evaluate_uptime (void *data) {
	unsigned long uptime = millis () / 1000;
	byte d, h, m, s;

	d = uptime / 86400;
	uptime %= 86400;
	h = uptime / 3600;
	uptime %= 3600;
	m = uptime / 60;
	uptime %= 60;
	s = uptime;

	replaceBuffer[0] = '\0';

	if (d > 0) {
		itoa (d, replaceBuffer, DEC);
		strcat_P (replaceBuffer, d == 1 ? PSTR (" day, ") : PSTR (" days, "));
	}
#if 0
	if (h > 0) {
		itoa (h, replaceBuffer + strlen (replaceBuffer), DEC);
		strcat_P (replaceBuffer, h == 1 ? PSTR (" hour, ") : PSTR (" hours, "));
	}

	if (m > 0) {
		itoa (m, replaceBuffer + strlen (replaceBuffer), DEC);
		strcat_P (replaceBuffer, m == 1 ? PSTR (" minute, ") : PSTR (" minutes, "));
	}

	// We always have seconds Maybe we could avoid them if h > 1 or so.
	itoa (s, replaceBuffer + strlen (replaceBuffer), DEC);
	strcat_P (replaceBuffer, s == 1 ? PSTR (" second") : PSTR (" seconds"));
#else
	// Shorter format: "2 days, 4:12:22", saves 70 bytes and doesn't overflow :D
	my_itoa (h, replaceBuffer + strlen (replaceBuffer), DEC, 2);
	strcat_P (replaceBuffer, PSTR (":"));
	my_itoa (m, replaceBuffer + strlen (replaceBuffer), DEC, 2);
	strcat_P (replaceBuffer, PSTR (":"));
	my_itoa (s, replaceBuffer + strlen (replaceBuffer), DEC, 2);
#endif

	return replaceBuffer;
}

// See http://playground.arduino.cc/Code/AvailableMemory
static char *evaluate_free_ram (void *data) {
	extern int __heap_start, *__brkval;
	int v;

	itoa ((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval), replaceBuffer, DEC);

	return replaceBuffer;
}


// Max length of these is MAX_TAG_LEN (24)
static const char subDateStr[] PROGMEM = "DATE";
static const char subTimeStr[] PROGMEM = "TIME";
static const char subMacAddrStr[] PROGMEM = "MACADDR";
static const char subIPAddressStr[] PROGMEM = "NET_IP";
static const char subNetmaskStr[] PROGMEM = "NET_MASK";
static const char subGatewayStr[] PROGMEM = "NET_GW";
static const char subNMDHCPStr[] PROGMEM = "NETMODE_DHCP_CHK";
static const char subNMStaticStr[] PROGMEM = "NETMODE_STATIC_CHK";
static const char subRelayOnStr[] PROGMEM = "RELAY_ON_CHK";
static const char subRelayOffStr[] PROGMEM = "RELAY_OFF_CHK";
static const char subRelay1StatusStr[] PROGMEM = "RELAY1_ST";
static const char subRelay2StatusStr[] PROGMEM = "RELAY2_ST";
static const char subRelay3StatusStr[] PROGMEM = "RELAY3_ST";
static const char subRelay4StatusStr[] PROGMEM = "RELAY4_ST";
#ifdef ENABLE_THERMOMETER
static const char subDegCStr[] PROGMEM = "DEGC";
static const char subDegFStr[] PROGMEM = "DEGF";
static const char subRelayTempStr[] PROGMEM = "RELAY_TEMP_CHK";
static const char subRelayTempGTStr[] PROGMEM = "RELAY_TGT_CHK";
static const char subRelayTempLTStr[] PROGMEM = "RELAY_TLT_CHK";
static const char subRelayTempThresholdStr[] PROGMEM = "RELAY_THRES";
static const char subRelayTempUnitsCStr[] PROGMEM = "RELAY_TEMPC_CHK";
static const char subRelayTempUnitsFStr[] PROGMEM = "RELAY_TEMPF_CHK";
static const char subRelayTempDelayStr[] PROGMEM = "RELAY_DELAY";
static const char subRelayTempMarginStr[] PROGMEM = "RELAY_MARGIN";
#endif
static const char subVerStr[] PROGMEM = "VERSION";
static const char subUptimeStr[] PROGMEM = "UPTIME";
static const char subFreeRAMStr[] PROGMEM = "FREERAM";

static const var_substitution subDateVarSub PROGMEM = {subDateStr, evaluate_date, NULL};
static const var_substitution subTimeVarSub PROGMEM =	{subTimeStr, evaluate_time, NULL};
static const var_substitution subMacAddrVarSub PROGMEM = {subMacAddrStr, evaluate_mac_addr, NULL};
static const var_substitution subIPAddressVarSub PROGMEM = {subIPAddressStr, evaluate_ip, NULL};
static const var_substitution subNetmaskVarSub PROGMEM = {subNetmaskStr, evaluate_netmask, NULL};
static const var_substitution subGatewayVarSub PROGMEM = {subGatewayStr, evaluate_gw, NULL};
static const var_substitution subNMDHCPVarSub PROGMEM = {subNMDHCPStr, evaluate_netmode_dhcp_checked, NULL};
static const var_substitution subNMStaticVarSub PROGMEM = {subNMStaticStr, evaluate_netmode_static_checked, NULL};
static const var_substitution subRelayOnVarSub PROGMEM = {subRelayOnStr, evaluate_relay_onoff_checked, reinterpret_cast<void *> (RELMD_ON)};
static const var_substitution subRelayOffVarSub PROGMEM = {subRelayOffStr, evaluate_relay_onoff_checked, reinterpret_cast<void *> (RELMD_OFF)};
static const var_substitution subRelay1StatusVarSub PROGMEM = {subRelay1StatusStr, evaluate_relay_status, reinterpret_cast<void *> (1)};
static const var_substitution subRelay2StatusVarSub PROGMEM = {subRelay2StatusStr, evaluate_relay_status, reinterpret_cast<void *> (2)};
static const var_substitution subRelay3StatusVarSub PROGMEM = {subRelay3StatusStr, evaluate_relay_status, reinterpret_cast<void *> (3)};
static const var_substitution subRelay4StatusVarSub PROGMEM = {subRelay4StatusStr, evaluate_relay_status, reinterpret_cast<void *> (4)};
#ifdef ENABLE_THERMOMETER
static const var_substitution subDegCVarSub PROGMEM =	{subDegCStr, evaluate_temp_deg, NULL};
static const var_substitution subDegFVarSub PROGMEM = {subDegFStr, evaluate_temp_fahr, NULL};
static const var_substitution subRelayTempVarSub PROGMEM = {subRelayTempStr, evaluate_relay_temp_checked, NULL};
static const var_substitution subRelayTempGTVarSub PROGMEM = {subRelayTempGTStr, evaluate_relay_temp_gt_checked, NULL};
static const var_substitution subRelayTempLTVarSub PROGMEM = {subRelayTempLTStr, evaluate_relay_temp_lt_checked, NULL};
static const var_substitution subRelayTempThresholdVarSub PROGMEM = {subRelayTempThresholdStr, evaluate_relay_temp_threshold, NULL};
static const var_substitution subRelayTempUnitsCVarSub PROGMEM = {subRelayTempUnitsCStr, evaluate_relay_temp_units_c_checked, NULL};
static const var_substitution subRelayTempUnitsFVarSub PROGMEM = {subRelayTempUnitsFStr, evaluate_relay_temp_units_f_checked, NULL};
static const var_substitution subRelayTempDelayVarSub PROGMEM = {subRelayTempDelayStr, evaluate_relay_temp_delay, NULL};
static const var_substitution subRelayTempMarginVarSub PROGMEM = {subRelayTempMarginStr, evaluate_relay_temp_margin, NULL};
#endif
static const var_substitution subVerVarSub PROGMEM = {subVerStr, evaluate_version, NULL};
static const var_substitution subUptimeVarSub PROGMEM = {subUptimeStr, evaluate_uptime, NULL};
static const var_substitution subFreeRAMVarSub PROGMEM = {subFreeRAMStr, evaluate_free_ram, NULL};

static const var_substitution * const substitutions[] PROGMEM = {
	&subDateVarSub,
	&subTimeVarSub,
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
	byte mac[6];

	DSTART ();
	DPRINTLN (F("SmartStrip " PROGRAM_VERSION));

	// Check and format EEPROMAnything, in case
	checkAndFormatEEPROM ();

	for (int i = 0; i < RELAYS_NO; i++) {
		relays[i].readOptions ();
		relays[i].effectState ();
	}

	// Get MAC from EEPROMAnything and init network
	EEPROM.get (EEPROM_MAC_B1_ADDR, mac[0]);
	EEPROM.get (EEPROM_MAC_B2_ADDR, mac[1]);
	EEPROM.get (EEPROM_MAC_B3_ADDR, mac[2]);
	EEPROM.get (EEPROM_MAC_B4_ADDR, mac[3]);
	EEPROM.get (EEPROM_MAC_B5_ADDR, mac[4]);
	EEPROM.get (EEPROM_MAC_B6_ADDR, mac[5]);

	webserver.setPages (pages);
#ifdef ENABLE_TAGS
	webserver.setSubstitutions (substitutions);
#endif

  NetworkMode netmode;
  EEPROM.get (EEPROM_NETMODE_ADDR, netmode);
	switch (netmode) {
		case NETMODE_STATIC: {
			byte ip[4], mask[4], gw[4];

			EEPROM.get (EEPROM_IP_B1_ADDR, ip[0]);
			EEPROM.get (EEPROM_IP_B2_ADDR, ip[1]);
			EEPROM.get (EEPROM_IP_B3_ADDR, ip[2]);
			EEPROM.get (EEPROM_IP_B4_ADDR, ip[3]);

			EEPROM.get (EEPROM_NETMASK_B1_ADDR, mask[0]);
			EEPROM.get (EEPROM_NETMASK_B2_ADDR, mask[1]);
			EEPROM.get (EEPROM_NETMASK_B3_ADDR, mask[2]);
			EEPROM.get (EEPROM_NETMASK_B4_ADDR, mask[3]);

			EEPROM.get (EEPROM_GATEWAY_B1_ADDR, gw[0]);
			EEPROM.get (EEPROM_GATEWAY_B2_ADDR, gw[0]);
			EEPROM.get (EEPROM_GATEWAY_B3_ADDR, gw[0]);
			EEPROM.get (EEPROM_GATEWAY_B4_ADDR, gw[0]);

			if (!webserver.begin (mac, ip, mask, gw)) {
				DPRINTLN (F("Failed to set static IP address"));
			} else {
				DPRINTLN (F("Static IP setup done"));
			}
			break;
		}
		default:
		case NETMODE_DHCP:
			DPRINTLN (F("Trying to get an IP address through DHCP"));
			if (!webserver.begin (mac)) {
				DPRINTLN (F("Failed to get configuration from DHCP"));
			} else {
				DPRINTLN (F("DHCP configuration done"));
#if 0
				ether.printIp ("IP:\t", webserver.getIP ());
				ether.printIp ("Mask:\t", webserver.getNetmask ());
				ether.printIp ("GW:\t", webserver.getGateway ());
				Serial.println ();
#endif
			}
			break;
	}

#ifdef ENABLE_THERMOMETER
	thermometer.begin (THERMOMETER_PIN);
#endif

	for (byte i = 0; i < RELAYS_NO; i++)
		relayHysteresis[i] = false;			// Start with no hysteresis

// 	DPRINTLN ("setup() complete");
}

void loop () {
	webserver.processPacket ();

	for (byte i = 0; i < RELAYS_NO; i++) {
		Relay& r = relays[i];
		bool& hysteresisEnabled = relayHysteresis[i];

		switch (r.mode) {
			case RELMD_ON:
				if (r.state != RELAY_ON)
					r.switchState (RELAY_ON);
				break;
			case RELMD_OFF:
				if (r.state != RELAY_OFF)
					r.switchState (RELAY_OFF);
				break;
#ifdef ENABLE_THERMOMETER
			case RELMD_GT:
				if (thermometer.available && (millis () - lastTemperatureRequest > THERMO_READ_INTERVAL)) {
          Temperature& temp = thermometer.getTemp ();
					if (temp.valid) {
						if (((!hysteresisEnabled && temp.celsius > r.threshold) || (hysteresisEnabled && temp.celsius > r.threshold + r.hysteresis / 10.0)) && r.state != RELAY_ON) {
							r.switchState (RELAY_ON);
							hysteresisEnabled = true;
						} else if (temp.celsius <= r.threshold && r.state != RELAY_OFF) {
							r.switchState (RELAY_OFF);
						}
          }

					lastTemperatureRequest = millis ();
				}
				break;
			case RELMD_LT:
				if (thermometer.available && (millis () - lastTemperatureRequest > THERMO_READ_INTERVAL)) {
					Temperature& temp = thermometer.getTemp ();
					if (temp.valid) {
						if (((!hysteresisEnabled && temp.celsius < r.threshold) || (hysteresisEnabled && temp.celsius < r.threshold - r.hysteresis / 10.0)) && r.state != RELAY_ON) {
							r.switchState (RELAY_ON);
							hysteresisEnabled = true;
						} else if (temp.celsius >= r.threshold && r.state != RELAY_OFF) {
							r.switchState (RELAY_OFF);
						}
					}

						lastTemperatureRequest = millis ();
				}
				break;
#endif
			default:
				break;
		}
	}
}
