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

#include "common.h"

#include <Webbino.h>
#ifdef USE_ENC28J60
	#include <EtherCard.h>
#else
	#include <SPI.h>
	#include <Ethernet.h>
#endif

#include <avr/pgmspace.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROMAnything.h>
#include <Panic.h>
// #include <Time.h>
#include "Thermometer.h"
#include "Relay.h"
#include "enums.h"
#include "html.h"

// Other stuff
Panic panic;
byte lastSelectedRelay;


// Instantiate the WebServer
WebServer webserver;

// Instantiate the thermometer
Thermometer thermometer;

bool justStarted;


Relay relays[RELAYS_NO] = {
	Relay (1, RELAY1_PIN),
	Relay (2, RELAY2_PIN),
	Relay (3, RELAY3_PIN),
	Relay (4, RELAY4_PIN)
};

void checkAndFormatEEPROM () {
	unsigned long magic;

	// The casts here are very important
	magic = ((unsigned long) EEPROMAnything.read (0)) << 24;
	magic |= ((unsigned long) EEPROMAnything.read (1)) << 16;
	magic |= ((unsigned long) EEPROMAnything.read (2)) << 8;
	magic |= (unsigned long) EEPROMAnything.read (3);

	if (magic != EEPROM_MAGIC) {
		// Need to format EEPROMAnything
		DPRINT (F("Formatting EEPROMAnything (Wrong Magic: 0x"));
		DPRINT (magic >> 16, HEX);			// It seems we can't print a long in a single pass
		DPRINT (magic, HEX);
		DPRINTLN (F(")"));

		// Magic header
		EEPROMAnything.write (0, (EEPROM_MAGIC >> 24) & 0xFF);
		EEPROMAnything.write (1, (EEPROM_MAGIC >> 16) & 0xFF);
		EEPROMAnything.write (2, (EEPROM_MAGIC >> 8) & 0xFF);
		EEPROMAnything.write (3, EEPROM_MAGIC & 0xFF);

		// Relay data
		for (int i = 0; i < RELAYS_NO; i++) {
			relays[i].setDefaults ();
			relays[i].writeOptions ();
		}

		// Network configuration
		EEPROMAnything.write (EEPROM_MAC_B1_ADDR, DEFAULT_MAC_ADDRESS_B1);
		EEPROMAnything.write (EEPROM_MAC_B2_ADDR, DEFAULT_MAC_ADDRESS_B2);
		EEPROMAnything.write (EEPROM_MAC_B3_ADDR, DEFAULT_MAC_ADDRESS_B3);
		EEPROMAnything.write (EEPROM_MAC_B4_ADDR, DEFAULT_MAC_ADDRESS_B4);
		EEPROMAnything.write (EEPROM_MAC_B5_ADDR, DEFAULT_MAC_ADDRESS_B5);
		EEPROMAnything.write (EEPROM_MAC_B6_ADDR, DEFAULT_MAC_ADDRESS_B6);
		EEPROMAnything.write (EEPROM_NETMODE_ADDR, DEFAULT_NET_MODE);
		EEPROMAnything.write (EEPROM_IP_B1_ADDR, DEFAULT_IP_ADDRESS_B1);
		EEPROMAnything.write (EEPROM_IP_B2_ADDR, DEFAULT_IP_ADDRESS_B2);
		EEPROMAnything.write (EEPROM_IP_B3_ADDR, DEFAULT_IP_ADDRESS_B3);
		EEPROMAnything.write (EEPROM_IP_B4_ADDR, DEFAULT_IP_ADDRESS_B4);
		EEPROMAnything.write (EEPROM_NETMASK_B1_ADDR, DEFAULT_NETMASK_B1);
		EEPROMAnything.write (EEPROM_NETMASK_B2_ADDR, DEFAULT_NETMASK_B2);
		EEPROMAnything.write (EEPROM_NETMASK_B3_ADDR, DEFAULT_NETMASK_B3);
		EEPROMAnything.write (EEPROM_NETMASK_B4_ADDR, DEFAULT_NETMASK_B4);
		EEPROMAnything.write (EEPROM_GATEWAY_B1_ADDR, DEFAULT_GATEWAY_ADDRESS_B1);
		EEPROMAnything.write (EEPROM_GATEWAY_B2_ADDR, DEFAULT_GATEWAY_ADDRESS_B2);
		EEPROMAnything.write (EEPROM_GATEWAY_B3_ADDR, DEFAULT_GATEWAY_ADDRESS_B3);
		EEPROMAnything.write (EEPROM_GATEWAY_B4_ADDR, DEFAULT_GATEWAY_ADDRESS_B4);

		DPRINTLN (F("EEPROMAnything Format complete"));
	} else {
		DPRINTLN (F("EEPROMAnything OK"));
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

void config_func (HTTPRequestParser& request) {
	char *param;
	byte buf[6];

	param = request.get_get_parameter (F("mac"));
	if (strlen (param) > 0) {
		if (tokenize (param, PSTR ("%3A"), buf, 6, 16)) {		// ':' gets encoded to "%3A" when submitting the form
			EEPROMAnything.write (EEPROM_MAC_B1_ADDR, buf[0]);
			EEPROMAnything.write (EEPROM_MAC_B2_ADDR, buf[1]);
			EEPROMAnything.write (EEPROM_MAC_B3_ADDR, buf[2]);
			EEPROMAnything.write (EEPROM_MAC_B4_ADDR, buf[3]);
			EEPROMAnything.write (EEPROM_MAC_B5_ADDR, buf[4]);
			EEPROMAnything.write (EEPROM_MAC_B6_ADDR, buf[5]);
		}
	}
	
	param = request.get_get_parameter (F("mode"));
	if (strlen (param) > 0) {
		if (strcmp_P (param, PSTR ("dhcp")) == 0)
			EEPROMAnything.write (EEPROM_NETMODE_ADDR, NETMODE_DHCP);
		else if (strcmp_P (param, PSTR ("static")) == 0)
			EEPROMAnything.write (EEPROM_NETMODE_ADDR, NETMODE_STATIC);
	}

	param = request.get_get_parameter (F("ip"));
	if (strlen (param) > 0 && tokenize (param, PSTR ("."), buf, 4, 10)) {
		EEPROMAnything.write (EEPROM_IP_B1_ADDR, buf[0]);
		EEPROMAnything.write (EEPROM_IP_B2_ADDR, buf[1]);
		EEPROMAnything.write (EEPROM_IP_B3_ADDR, buf[2]);
		EEPROMAnything.write (EEPROM_IP_B4_ADDR, buf[3]);
	}

	param = request.get_get_parameter (F("mask"));
	if (strlen (param) > 0 && tokenize (param, PSTR ("."), buf, 4, 10)) {
		EEPROMAnything.write (EEPROM_NETMASK_B1_ADDR, buf[0]);
		EEPROMAnything.write (EEPROM_NETMASK_B2_ADDR, buf[1]);
		EEPROMAnything.write (EEPROM_NETMASK_B3_ADDR, buf[2]);
		EEPROMAnything.write (EEPROM_NETMASK_B4_ADDR, buf[3]);
	}

	param = request.get_get_parameter (F("gw"));
	if (strlen (param) > 0 && tokenize (param, PSTR ("."), buf, 4, 10)) {
		EEPROMAnything.write (EEPROM_GATEWAY_B1_ADDR, buf[0]);
		EEPROMAnything.write (EEPROM_GATEWAY_B2_ADDR, buf[1]);
		EEPROMAnything.write (EEPROM_GATEWAY_B3_ADDR, buf[2]);
		EEPROMAnything.write (EEPROM_GATEWAY_B4_ADDR, buf[3]);
	}

	Serial.println (F("Configuration saved"));
}

void sck_func (HTTPRequestParser& request) {
	char *param;

	param = request.get_get_parameter (F("rel"));
	if (strlen (param) > 0) {
		int relay = atoi (param);
		panic_assert (panic, relay >= 1 && relay <= RELAYS_NO);

		param = request.get_get_parameter (F("mode"));
		if (strlen (param) > 0) {		// Only do something if we got this parameter
			if (strcmp_P (param, PSTR ("on")) == 0) {
				relays[relay - 1].mode = RELMD_ON;
			} else if (strcmp_P (param, PSTR ("off")) == 0) {
				relays[relay - 1].mode = RELMD_OFF;
			} else if (strcmp_P (param, PSTR ("temp")) == 0) {
				param = request.get_get_parameter (F("thres"));
				if (strcmp_P (param, PSTR ("gt")) == 0)
					relays[relay - 1].mode = RELMD_GT;
				else
					relays[relay - 1].mode = RELMD_LT;
			}
		}

		/* Save the last selected relay for later. I know this is crap, but...
		 * See below.
		 */
		lastSelectedRelay = relay;
	}
}

static Page aboutPage PROGMEM = {about_html_name, about_html, NULL};
static Page configPage PROGMEM = {config_html_name, config_html, config_func};
static Page indexPage PROGMEM = {index_html_name, index_html, NULL};
static Page leftPage PROGMEM = {left_html_name, left_html, NULL};
static Page sckPage PROGMEM = {sck_html_name, sck_html, sck_func};

static Page *pages[] PROGMEM = {
	&aboutPage,
	&configPage,
	&indexPage,
	&leftPage,
	&sckPage,
 	NULL
};


/******************************************************************************
 * DEFINITION OF TAGS                                                         *
 ******************************************************************************/

#ifdef ENABLE_TAGS

#define REP_BUFFER_LEN 32
static char replaceBuffer[REP_BUFFER_LEN];

const char NOT_AVAIL_STR[] PROGMEM = "N/A";


static char *evaluate_date () {
#ifdef USE_UNIX
	char *tmp;
	time_t tt;
	struct tm now;

	time (&tt);
	localtime_r (&tt, &now);
	strftime (replaceBuffer, REP_BUFFER_LEN, "%d/%m/%Y", &now);
#elif defined USE_ARDUINO_TIME_LIBRARY
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

static char *evaluate_time () {
#ifdef USE_UNIX
	char *tmp;
	time_t tt;
	struct tm now;

	time (&tt);
	localtime_r (&tt, &now);
	strftime (replaceBuffer, REP_BUFFER_LEN, "%H:%M:%S", &now);
#elif defined USE_ARDUINO_TIME_LIBRARY
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

static char *evaluate_temp_deg () {
	Temperature& temp = thermometer.getTemp ();
	if (temp.valid) {
		//dtostrf (temp.celsius, -1, 2, replaceBuffer);
		floatToString (temp.celsius, replaceBuffer);
	} else {
		strlcpy_P (replaceBuffer, NOT_AVAIL_STR, REP_BUFFER_LEN);
	}
	
	return replaceBuffer;
}

static char *evaluate_temp_fahr () {
	Temperature& temp = thermometer.getTemp ();
	if (temp.valid) {
		//dtostrf (temp.toFahrenheit (), -1, 2, replaceBuffer);
		floatToString (temp.celsius, replaceBuffer);
	} else
		strlcpy_P (replaceBuffer, NOT_AVAIL_STR, REP_BUFFER_LEN);
	
	return replaceBuffer;
}

static char *ip2str (const byte *buf) {
#if 0
	itoa (buf[0], replaceBuffer, DEC);
	strcat_P (replaceBuffer, PSTR ("."));
	itoa (buf[1], replaceBuffer + strlen (replaceBuffer), DEC);
	strcat_P (replaceBuffer, PSTR ("."));
	itoa (buf[2], replaceBuffer + strlen (replaceBuffer), DEC);
	strcat_P (replaceBuffer, PSTR ("."));
	itoa (buf[3], replaceBuffer + strlen (replaceBuffer), DEC);
#else
	// Saves 10 bytes :D
	replaceBuffer[0] = '\0';
	for (int i = 0; i < 3; i++) {
		itoa (buf[i], replaceBuffer + strlen (replaceBuffer), DEC);
		strcat_P (replaceBuffer, PSTR ("."));
	}
	itoa (buf[3], replaceBuffer + strlen (replaceBuffer), DEC);
#endif
	
	return replaceBuffer;
}

static char *evaluate_ip () {
 	return ip2str (webserver.getIP ());
}

static char *evaluate_netmask () {
	return ip2str (webserver.getNetmask ());
}

static char *evaluate_gw () {
	return ip2str (webserver.getGateway ());
}

static char int2hex (int i) {
	if (i < 10)
		return i + '0';
	else
		return i - 10 + 'A';
}

static char *evaluate_mac_addr () {
	byte tmp;

	tmp = EEPROMAnything.read (EEPROM_MAC_B1_ADDR);
	replaceBuffer[0] = int2hex (tmp / 16);
	replaceBuffer[1] = int2hex (tmp % 16);

	replaceBuffer[2] = ':';

	tmp = EEPROMAnything.read (EEPROM_MAC_B2_ADDR);
	replaceBuffer[3] = int2hex (tmp / 16);
	replaceBuffer[4] = int2hex (tmp % 16);

	replaceBuffer[5] = ':';

	tmp = EEPROMAnything.read (EEPROM_MAC_B3_ADDR);
	replaceBuffer[6] = int2hex (tmp / 16);
	replaceBuffer[7] = int2hex (tmp % 16);

	replaceBuffer[8] = ':';

	tmp = EEPROMAnything.read (EEPROM_MAC_B4_ADDR);
	replaceBuffer[9] = int2hex (tmp / 16);
	replaceBuffer[10] = int2hex (tmp % 16);

	replaceBuffer[11] = ':';

	tmp = EEPROMAnything.read (EEPROM_MAC_B5_ADDR);
	replaceBuffer[12] = int2hex (tmp / 16);
	replaceBuffer[13] = int2hex (tmp % 16);

	replaceBuffer[14] = ':';

	tmp = EEPROMAnything.read (EEPROM_MAC_B6_ADDR);
	replaceBuffer[15] = int2hex (tmp / 16);
	replaceBuffer[16] = int2hex (tmp % 16);

	replaceBuffer[17] = '\0';

	return replaceBuffer;
}


const char CHECKED_STRING[] PROGMEM = "checked";
const char SELECTED_STRING[] PROGMEM = "selected=\"true\"";

static char *evaluate_netmode_dhcp_checked () {
	int netmode = EEPROMAnything.read (EEPROM_NETMODE_ADDR);

	if (netmode == NETMODE_DHCP)
		strlcpy_P (replaceBuffer, CHECKED_STRING, REP_BUFFER_LEN);
	else
		replaceBuffer[0] = '\0';
	
	return replaceBuffer;	
}

static char *evaluate_netmode_static_checked () {
	int netmode = EEPROMAnything.read (EEPROM_NETMODE_ADDR);

	if (netmode == NETMODE_STATIC)
		strlcpy_P (replaceBuffer, CHECKED_STRING, REP_BUFFER_LEN);
	else
		replaceBuffer[0] = '\0';

	return replaceBuffer;
}

/* OK, this works in a quite crap way, but it's going to work as long as a
 * single instance of the web interface is being used, which is likely our
 * case.
 *
 * PS: lastSelectedRelay is saved in sck_func().
 */
static char *evaluate_relay_on_checked () {
	replaceBuffer[0] = '\0';

	if (lastSelectedRelay >= 1 && lastSelectedRelay <= 4) {
		if (relays[lastSelectedRelay - 1].mode == RELMD_ON)
			strlcpy_P (replaceBuffer, CHECKED_STRING, REP_BUFFER_LEN);
	}

	return replaceBuffer;
}

static char *evaluate_relay_off_checked () {
	replaceBuffer[0] = '\0';

	if (lastSelectedRelay >= 1 && lastSelectedRelay <= 4) {
		if (relays[lastSelectedRelay - 1].mode == RELMD_OFF)
			strlcpy_P (replaceBuffer, CHECKED_STRING, REP_BUFFER_LEN);
	}

	return replaceBuffer;
}

static char *evaluate_relay_temp_checked () {
	replaceBuffer[0] = '\0';

	if (lastSelectedRelay >= 1 && lastSelectedRelay <= 4) {
		if (relays[lastSelectedRelay - 1].mode == RELMD_GT || relays[lastSelectedRelay - 1].mode == RELMD_LT)
			strlcpy_P (replaceBuffer, CHECKED_STRING, REP_BUFFER_LEN);
	}

	return replaceBuffer;
}

static char *evaluate_relay_temp_gt_checked () {
	replaceBuffer[0] = '\0';

	if (lastSelectedRelay >= 1 && lastSelectedRelay <= 4) {
		if (relays[lastSelectedRelay - 1].mode == RELMD_GT)
			strlcpy_P (replaceBuffer, SELECTED_STRING, REP_BUFFER_LEN);
	}

	return replaceBuffer;
}

static char *evaluate_relay_temp_lt_checked () {
	replaceBuffer[0] = '\0';

	if (lastSelectedRelay >= 1 && lastSelectedRelay <= 4) {
		if (relays[lastSelectedRelay - 1].mode == RELMD_LT)
			strlcpy_P (replaceBuffer, SELECTED_STRING, REP_BUFFER_LEN);
	}

	return replaceBuffer;
}

static char *evaluate_version () {
	strlcpy (replaceBuffer, PROGRAM_VERSION, REP_BUFFER_LEN);
	return replaceBuffer;
}

// Wahahahah! Prolly the most advanced function of its kind!
// FIXME: Save some bytes removing temp vars.
// FIXME: Check that string does not overflow buffer (which is likely!)
static char *evaluate_uptime () {
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
	itoa (h, replaceBuffer + strlen (replaceBuffer), DEC);
	strcat_P (replaceBuffer, PSTR (":"));
	itoa (m, replaceBuffer + strlen (replaceBuffer), DEC);
	strcat_P (replaceBuffer, PSTR (":"));
	itoa (s, replaceBuffer + strlen (replaceBuffer), DEC);
#endif
	
	return replaceBuffer;
}

static char *evaluate_free_ram () {
	extern int __heap_start, *__brkval;
	int v;

	itoa ((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval), replaceBuffer, DEC);

	return replaceBuffer;
}


// Max length of these is MAX_TAG_LEN (24)
static const char subDateStr[] PROGMEM = "DATE";
static const char subTimeStr[] PROGMEM = "TIME";
static const char subDegCStr[] PROGMEM = "DEGC";
static const char subDegFStr[] PROGMEM = "DEGF";
static const char subMacAddrStr[] PROGMEM = "MACADDR";
static const char subIPAddressStr[] PROGMEM = "NET_IP";
static const char subNetmaskStr[] PROGMEM = "NET_MASK";
static const char subGatewayStr[] PROGMEM = "NET_GW";
static const char subNMDHCPStr[] PROGMEM = "NETMODE_DHCP_CHK";
static const char subNMStaticStr[] PROGMEM = "NETMODE_STATIC_CHK";
static const char subRelayOnStr[] PROGMEM = "RELAY_ON_CHK";
static const char subRelayOffStr[] PROGMEM = "RELAY_OFF_CHK";
static const char subRelayTempStr[] PROGMEM = "RELAY_TEMP_CHK";
static const char subRelayTempGTStr[] PROGMEM = "RELAY_TGT_CHK";
static const char subRelayTempLTStr[] PROGMEM = "RELAY_TLT_CHK";
static const char subVerStr[] PROGMEM = "VERSION";
static const char subUptimeStr[] PROGMEM = "UPTIME";
static const char subFreeRAMStr[] PROGMEM = "FREERAM";

static var_substitution subDateVarSub PROGMEM = {subDateStr, evaluate_date};
static var_substitution subTimeVarSub PROGMEM =	{subTimeStr, evaluate_time};
static var_substitution subDegCVarSub PROGMEM =	{subDegCStr, evaluate_temp_deg};
static var_substitution subDegFVarSub PROGMEM = {subDegFStr, evaluate_temp_fahr};
static var_substitution subMacAddrVarSub PROGMEM = {subMacAddrStr, evaluate_mac_addr};
static var_substitution subIPAddressVarSub PROGMEM = {subIPAddressStr, evaluate_ip};
static var_substitution subNetmaskVarSub PROGMEM = {subNetmaskStr, evaluate_netmask};
static var_substitution subGatewayVarSub PROGMEM = {subGatewayStr, evaluate_gw};
static var_substitution subNMDHCPVarSub PROGMEM = {subNMDHCPStr, evaluate_netmode_dhcp_checked};
static var_substitution subNMStaticVarSub PROGMEM = {subNMStaticStr, evaluate_netmode_static_checked};
static var_substitution subRelayOnVarSub PROGMEM = {subRelayOnStr, evaluate_relay_on_checked};
static var_substitution subRelayOffVarSub PROGMEM = {subRelayOffStr, evaluate_relay_off_checked};
static var_substitution subRelayTempVarSub PROGMEM = {subRelayTempStr, evaluate_relay_temp_checked};
static var_substitution subRelayTempGTVarSub PROGMEM = {subRelayTempGTStr, evaluate_relay_temp_gt_checked};
static var_substitution subRelayTempLTVarSub PROGMEM = {subRelayTempLTStr, evaluate_relay_temp_lt_checked};
static var_substitution subVerVarSub PROGMEM = {subVerStr, evaluate_version};
static var_substitution subUptimeVarSub PROGMEM = {subUptimeStr, evaluate_uptime};
static var_substitution subFreeRAMVarSub PROGMEM = {subFreeRAMStr, evaluate_free_ram};
	
static var_substitution *substitutions[] PROGMEM = {
	&subDateVarSub,
	&subTimeVarSub,
	&subDegCVarSub,
	&subDegFVarSub,
	&subMacAddrVarSub,
	&subIPAddressVarSub,
	&subNetmaskVarSub,
	&subGatewayVarSub,
	&subNMDHCPVarSub,
	&subNMStaticVarSub,
	&subRelayOnVarSub,
	&subRelayOffVarSub,
	&subRelayTempVarSub,
	&subRelayTempGTVarSub,
	&subRelayTempLTVarSub,
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

	Serial.begin (9600);
	Serial.println (F("SmartStrip " PROGRAM_VERSION));

	// Check and format EEPROMAnything, in case
	checkAndFormatEEPROM ();

	for (int i = 0; i < RELAYS_NO; i++) {
		relays[i].readOptions ();
		relays[i].effectState ();
	}

	// Get MAC from EEPROMAnything and init network
	mac[0] = EEPROMAnything.read (EEPROM_MAC_B1_ADDR);
	mac[1] = EEPROMAnything.read (EEPROM_MAC_B2_ADDR);
	mac[2] = EEPROMAnything.read (EEPROM_MAC_B3_ADDR);
	mac[3] = EEPROMAnything.read (EEPROM_MAC_B4_ADDR);
	mac[4] = EEPROMAnything.read (EEPROM_MAC_B5_ADDR);
	mac[5] = EEPROMAnything.read (EEPROM_MAC_B6_ADDR);

	webserver.setPages (pages);
#ifdef ENABLE_TAGS
	webserver.setSubstitutions (substitutions);
#endif

	switch (EEPROMAnything.read (EEPROM_NETMODE_ADDR)) {
		case NETMODE_STATIC: {
			byte ip[4], mask[4], gw[4];
			
			ip[0] = EEPROMAnything.read (EEPROM_IP_B1_ADDR);
			ip[1] = EEPROMAnything.read (EEPROM_IP_B2_ADDR);
			ip[2] = EEPROMAnything.read (EEPROM_IP_B3_ADDR);
			ip[3] = EEPROMAnything.read (EEPROM_IP_B4_ADDR);

			mask[0] = EEPROMAnything.read (EEPROM_NETMASK_B1_ADDR);
			mask[1] = EEPROMAnything.read (EEPROM_NETMASK_B2_ADDR);
			mask[2] = EEPROMAnything.read (EEPROM_NETMASK_B3_ADDR);
			mask[3] = EEPROMAnything.read (EEPROM_NETMASK_B4_ADDR);
			
			gw[0] = EEPROMAnything.read (EEPROM_GATEWAY_B1_ADDR);
			gw[1] = EEPROMAnything.read (EEPROM_GATEWAY_B2_ADDR);
			gw[2] = EEPROMAnything.read (EEPROM_GATEWAY_B3_ADDR);
			gw[3] = EEPROMAnything.read (EEPROM_GATEWAY_B4_ADDR);

			if (!webserver.begin (mac, ip, mask, gw))
				panic.panic (F("Failed to set static IP address"));
			else
				Serial.println (F("Static IP setup done"));
			break;
		}
		default:
		case NETMODE_DHCP:
			Serial.println (F("Trying to get an IP address through DHCP"));
			if (!webserver.begin (mac)) {
				panic.panic (F("Failed to get configuration from DHCP"));
			} else {
				Serial.println (F("DHCP configuration done"));
#if 0
				ether.printIp ("IP:\t", webserver.getIP ());
				ether.printIp ("Mask:\t", webserver.getNetmask ());
				ether.printIp ("GW:\t", webserver.getGateway ());
				Serial.println ();
#endif
			}
			break;
	}

	thermometer.setup ();

	justStarted = true;
}

#define THRES 34
#define THYST 1.0

void loop () {
	webserver.processPacket ();
	thermometer.loop ();

	for (int i = 0; i < RELAYS_NO; i++) {
		Relay& r = relays[i];
		
		switch (r.mode) {
			case RELMD_ON:
				if (r.state != RELAY_ON)
					r.switchState (RELAY_ON);
				break;
			case RELMD_OFF:
				if (r.state != RELAY_OFF)
					r.switchState (RELAY_OFF);
				break;
			case RELMD_GT: {
				Temperature& temp = thermometer.getTemp ();
				if (temp.valid) {
					if (((justStarted && temp.celsius > THRES) || (!justStarted && temp.celsius > THRES + THYST)) && r.state != RELAY_ON) {
						r.switchState (RELAY_ON);
						justStarted = false;
					} else if (temp.celsius <= THRES && r.state != RELAY_OFF) {
						r.switchState (RELAY_OFF);
					}
				}
				break;
			} case RELMD_LT: {
				Temperature& temp = thermometer.getTemp ();
				if (temp.valid) {
					if (((justStarted && temp.celsius < THRES) || (!justStarted && temp.celsius < THRES - THYST)) && r.state != RELAY_ON) {
						r.switchState (RELAY_ON);
						justStarted = false;
					} else if (temp.celsius >= THRES && r.state != RELAY_OFF) {
						r.switchState (RELAY_OFF);
					}
				}
				break;
			} default:
// 				DPRINT ("Relay ");
// 				DPRINT (r.id);
// 				DPRINT (" mode is ");
// 				DPRINT (r.mode);
// 				DPRINT (" and state is ");
// 				DPRINTLN (r.state);

				panic_assert_not_reached (panic);
				break;
		}
	}
}
