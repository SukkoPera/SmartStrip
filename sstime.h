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

#ifndef SSTIME_H_INCLUDED
#define SSTIME_H_INCLUDED

#include "common.h"

const byte UTC_OFFSET = +1;

#ifdef ENABLE_TIME
#include <TimeLib.h>

byte dstOffset (byte d, byte m, unsigned int y, byte h) {
	/* This function returns the DST offset for the current UTC time.
	 * This is valid for the EU, for other places see
	 * http://www.webexhibits.org/daylightsaving/i.html
	 *
	 * Results have been checked for 2012-2030 (but should be correct
	 * since 1996 to 2099) against the following references:
	 * - http://www.uniquevisitor.it/magazine/ora-legale-italia.php
	 * - http://www.calendario-365.it/ora-legale-orario-invernale.html
	 */

	// Day in March that DST starts on, at 1 am
	byte dstOn = (31 - (5 * y / 4 + 4) % 7);

	// Day in October that DST ends  on, at 2 am
	byte dstOff = (31 - (5 * y / 4 + 1) % 7);

	if ((m > 3 && m < 10) ||
		(m == 3 && (d > dstOn || (d == dstOn && h >= 1))) ||
		(m == 10 && (d < dstOff || (d == dstOff && h <= 1))))
		return 1;
	else
		return 0;
}

#ifdef TIMESRC_NTP

#include <NTPClient.h>

#if defined (WEBBINO_USE_ENC28J60_UIP) || defined (WEBBINO_USE_WIZ5100) || \
    defined (WEBBINO_USE_WIZ5500)
EthernetUDP ntpUDP;
#elif defined (WEBBINO_USE_ESP8266) || defined (WEBBINO_USE_ESP8266_STANDALONE) || \
      defined (WEBBINO_USE_WIFI) || defined (WEBBINO_USE_WIFI101)  || \
      defined (WEBBINO_USE_FISHINO)
#include <WiFiUdp.h>
WiFiUDP ntpUDP;
#else
#error "NTPClient is unsupported on WEBBINO_USE_DIGIFI and WEBBINO_USE_ENC28J60"
#endif

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient (ntpUDP, "pool.ntp.org", 0, 60000UL * 5);    // TZ and DST will be compensated in timeProvider()

// FIXME: I think this should return 0 if NTP update failed, but current library
// has no means of detecting such a case
time_t timeProvider () {
	Serial.println ("timeProvider() called");
	timeClient.update ();
	//timeClient.setTimeOffset ((UTC_OFFSET + dstOffset ()) * SECS_PER_HOUR);
	time_t t = timeClient.getEpochTime ();

	TimeElements tm;
	breakTime (t, tm);
	t += (UTC_OFFSET + dstOffset (tm.Day, tm.Month, tm.Year + 1970, tm.Hour)) * SECS_PER_HOUR;

	return t;
}

#elif defined (TIMESRC_STM32_RTC)

#include <RTClock.h>    // Library for STM32-builtin RTC

RTClock rt (RTCSEL_LSE); // initialise

time_t timeProvider () {
  time_t t = rt.getTime ();

  // Apply DST and TZ corrections
  TimeElements tm;
  breakTime (t, tm);
  t += (UTC_OFFSET + dstOffset (tm.Day, tm.Month, tm.Year + 1970, tm.Hour)) * SECS_PER_HOUR;

  return t;
}

/* Provide a facility to set time through serial:
 * On Linux, you can use "date +T%s > /dev/ttyACM0" (UTC time zone)
 */

/*  code to process time sync messages from the serial port   */
//~ const char* TIME_HEADER = "T";   // Header tag for serial time sync message

boolean processSyncMessage (const time_t& pctime) {
	const time_t DEFAULT_TIME = 1577836800; // Jan 1 2020
	boolean ret = false;

	if (pctime < DEFAULT_TIME) { // check the value is a valid time
		DPRINT (F("Invalid time for RTC"));
	} else {
		DPRINT (F("Setting RTC time to "));
		DPRINTLN (pctime);

		rt.setTime (pctime);    // set the RTC and the system time to the received value
		//~ setTime (pctime);

		ret = true;
	}

	return ret;
}

#else

#warning "Please enable a method to get proper time!"

time_t timeProvider () {
	return 0;
}

#endif

#endif	// ENABLE_TIME

#endif
