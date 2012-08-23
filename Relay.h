#ifndef _RELAY_H_
#define _RELAY_H_

#include "enums.h"
#include "common.h"

	
class Relay: public RelayOptions {
	static const int optionAddress[RELAYS_NO];

public:	
	byte id;
	byte pin;
	
	Relay (byte _id, byte _pin);

	void readOptions ();
	void writeOptions ();
	void setDefaults ();

	void switchState (RelayState newState);
	void effectState ();
};
	
#endif
