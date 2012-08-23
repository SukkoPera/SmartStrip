#include <Arduino.h>
#include "EEPROMAnything.h"
#include "Panic.h"
#include "common.h"
#include "debug.h"
#include "Relay.h"

extern Panic panic;


const int Relay::optionAddress[RELAYS_NO] = {
	EEPROM_R1_PARAM_ADDR,
	EEPROM_R2_PARAM_ADDR,
	EEPROM_R3_PARAM_ADDR,
	EEPROM_R4_PARAM_ADDR
};

Relay::Relay (byte _id, byte _pin): id (_id), pin (_pin) {
	panic_assert (panic, _id >= 1 && _id <= RELAYS_NO);
	pinMode (_pin, OUTPUT);
}

void Relay::readOptions () {
	EEPROMAnything.readAnything (optionAddress[id - 1], dynamic_cast<RelayOptions&> (*this));
}

void Relay::writeOptions () {
	EEPROMAnything.writeAnything (optionAddress[id - 1], dynamic_cast<RelayOptions&> (*this));
}

void Relay::setDefaults () {
	mode = DEFAULT_RELAY_MODE;
	state = DEFAULT_RELAY_STATE;
	threshold = DEFAULT_RELAY_THRESHOLD;
	units = DEFAULT_RELAY_UNITS;
	hysteresis = DEFAULT_RELAY_HYSTERESIS;
	delay = DEFAULT_RELAY_DELAY;
}

void Relay::switchState (RelayState newState) {
	DPRINT (F("Turning "));
	DPRINT (state ? F("ON") : F("OFF"));
	DPRINT (F(" relay "));
	DPRINTLN (id, DEC);

	digitalWrite (pin, state = newState);
}

void Relay::effectState () {
	digitalWrite (pin, state);
}
