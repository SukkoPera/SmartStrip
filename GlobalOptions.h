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

#ifndef GLOBOPTS_H_INCLUDED
#define GLOBOPTS_H_INCLUDED

#include <Arduino.h>
#include <IPAddress.h>
#include <EEPROM.h>
#include "common.h"
#include "enums.h"
#include "debug.h"

// EEPROM offsets: Align this on 4 bytes for 32-bit platforms
#define EEPROM_MAGIC 0x53545030UL			// "STP0"
#define EEPROM_RELAY_PARAM_ADDR 4
#define EEPROM_RELAY_PARAM_SIZE 4			// Only using 2 ATM
#define EEPROM_MAC_ADDR 68
#define EEPROM_NETMODE_ADDR 74
#define EEPROM_IP_ADDR 78
#define EEPROM_NETMASK_ADDR 82
#define EEPROM_GATEWAY_ADDR 86
#define EEPROM_DNS_ADDR 90
#define EEPROM_HYSTERESIS 94
#define EEPROM_DELAY 96

#ifdef ENABLE_TIME
// Schedule size is about 12 byte per day * 7 days
// Note that these are used as EEPROM_R1_SCHEDULE_ADDR * relay_id
#define EEPROM_SCHEDULE_ADDR 100
#define EEPROM_SCHEDULE_SIZE 100
#endif


class GlobalOptions {
private:
	IPAddress getIpAddress (const uint16_t offset) {
		uint32_t addr;
		EEPROM.get (offset, addr);
		return IPAddress (addr);
	}

	void putIpAddress (const uint16_t offset, const IPAddress& addr) {
		EEPROM.put (offset, (const uint32_t&) addr);
	}

public:
	static uint16_t getRelayParamOffset (const byte relayId) {
		return EEPROM_RELAY_PARAM_ADDR + relayId * EEPROM_RELAY_PARAM_SIZE;
	}

#ifdef ENABLE_TIME
	static uint16_t getScheduleOffset (const byte relayId) {
		return EEPROM_SCHEDULE_ADDR + relayId * EEPROM_SCHEDULE_SIZE;
	}
#endif

	boolean begin () {
		boolean justFormatted = false;
		unsigned long magic;

#if defined (ARDUINO_ARCH_ESP32) || defined (ARDUINO_ARCH_ESP8266)
		EEPROM.begin (1024);			// ATmega328 EEPROM size must be enough
#endif

		EEPROM.get (0, magic);
		if (magic != EEPROM_MAGIC) {
			// Need to format EEPROM
			WDPRINT (F("Formatting EEPROM (Wrong Magic: 0x"));
			WDPRINT (magic, HEX);
			WDPRINTLN (F(")"));

			// Magic header
			EEPROM.put (0, EEPROM_MAGIC);

			// Network configuration
			setNetworkMode (DEFAULT_NET_MODE);
			byte mac[MAC_SIZE] = {DEFAULT_MAC_ADDRESS};
			setMacAddress (mac);
			setFixedIpAddress (IPAddress (DEFAULT_IP_ADDRESS));
			setNetmask (IPAddress (DEFAULT_NETMASK));
			setGatewayAddress (IPAddress (DEFAULT_GATEWAY_ADDRESS));
			setDnsAddress (IPAddress (DEFAULT_DNS_ADDRESS));

			// Relay options
#ifdef ENABLE_THERMOMETER
			setRelayDelay (DEFAULT_RELAY_DELAY);
			setRelayHysteresis (DEFAULT_RELAY_HYSTERESIS);
#endif

#if defined (ARDUINO_ARCH_ESP32) || defined (ARDUINO_ARCH_ESP8266)
			EEPROM.commit ();
#endif

			justFormatted = true;

			WDPRINTLN (F("EEPROM Format complete"));
		} else {
			WDPRINTLN (F("EEPROM OK"));
		}

		return justFormatted;
	}

	byte *getMacAddress (byte buf[MAC_SIZE]) {
		EEPROM.get (EEPROM_MAC_ADDR, buf);
		return buf;
	}

	void setMacAddress (byte buf[MAC_SIZE]) {
		EEPROM.put (EEPROM_MAC_ADDR, buf);
#if defined (ARDUINO_ARCH_ESP32) || defined (ARDUINO_ARCH_ESP8266)
		EEPROM.commit ();
#endif

	}

	NetworkMode getNetworkMode () {
		NetworkMode netmode;
		EEPROM.get (EEPROM_NETMODE_ADDR, netmode);
		return netmode;
	}

	void setNetworkMode (NetworkMode netmode) {
		EEPROM.put (EEPROM_NETMODE_ADDR, netmode);
#if defined (ARDUINO_ARCH_ESP32) || defined (ARDUINO_ARCH_ESP8266)
		EEPROM.commit ();
#endif
	}

	IPAddress getFixedIpAddress () {
		return getIpAddress (EEPROM_IP_ADDR);
	}

	void setFixedIpAddress (const IPAddress& addr) {
		putIpAddress (EEPROM_IP_ADDR, addr);
#if defined (ARDUINO_ARCH_ESP32) || defined (ARDUINO_ARCH_ESP8266)
		EEPROM.commit ();
#endif
	}

	IPAddress getNetmask () {
		return getIpAddress (EEPROM_NETMASK_ADDR);
	}

	void setNetmask (const IPAddress& addr) {
		putIpAddress (EEPROM_NETMASK_ADDR, addr);
#if defined (ARDUINO_ARCH_ESP32) || defined (ARDUINO_ARCH_ESP8266)
		EEPROM.commit ();
#endif
	}

	IPAddress getGatewayAddress () {
		return getIpAddress (EEPROM_GATEWAY_ADDR);
	}

	void setGatewayAddress (const IPAddress& addr) {
		putIpAddress (EEPROM_GATEWAY_ADDR, addr);
#if defined (ARDUINO_ARCH_ESP32) || defined (ARDUINO_ARCH_ESP8266)
		EEPROM.commit ();
#endif
	}

	IPAddress getDnsAddress () {
		return getIpAddress (EEPROM_DNS_ADDR);
	}

	void setDnsAddress (const IPAddress& addr) {
		putIpAddress (EEPROM_DNS_ADDR, addr);
#if defined (ARDUINO_ARCH_ESP32) || defined (ARDUINO_ARCH_ESP8266)
		EEPROM.commit ();
#endif
	}

	byte getRelayHysteresis () const {
		byte hysteresis;
		hysteresis = EEPROM.read (EEPROM_HYSTERESIS);
		return hysteresis;
	}

	void setRelayHysteresis (byte hysteresis) {
		EEPROM.put (EEPROM_HYSTERESIS, hysteresis);
#if defined (ARDUINO_ARCH_ESP32) || defined (ARDUINO_ARCH_ESP8266)
		EEPROM.commit ();
#endif
	}

	unsigned long getRelayDelay () {
		unsigned long delay;
		EEPROM.get (EEPROM_DELAY, delay);
		return delay;
	}

	void setRelayDelay (unsigned long delay) {
		EEPROM.put (EEPROM_DELAY, delay);
#if defined (ARDUINO_ARCH_ESP32) || defined (ARDUINO_ARCH_ESP8266)
		EEPROM.commit ();
#endif
	}
};

#endif
