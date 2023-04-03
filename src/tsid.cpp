#include "globals.h"
#include "display.h"
#include "midi.h"
#include "mux.h"
#include "boot.h"
#include "isr.h"
#include "sid.h"
#include "ui_leds.h"

/*
 *
 *

code to convert HEX to SYSEX
cd /Users/a/Documents/bootloaderT&&cp -f /private/var/hex/TSID.ino.hex /Users/a/Documents/bootloaderT&&python
tools/hex2sysex/hex2sysex.py --syx -o firmware.syx TSID.ino.hex

TO DO:
Add setup mode (hold arp mode for 2 seconds), press again to exit (same as megaFM)
-MIDI channel learn (map input to ouput channel)
-in setup mode use a button (square 1?) to toggle LFO MIDI CC send on/off EEPROM 3996
-in setup mode uss a button (triangle 1?) to toggle ARP MIDI send on/off EEPROM 3995
-add 6 voice mode using 2 chips, I suggest we hold a waveform for several seconds for 3 voice paramode, hold it longer
for 6 (show P3, then P6 on display) -kill the voices when they are done singing by setting the freq to 0. Never tried
this (too dumb to figure out when the ADSR is finished), but I read it's a solution against the ghost notes (leaky VCA)

NEW in 2.0
Swapped filter mode and arp mode long hold function. hold filter mode for fat change (hold arp mode for setup mode)
Added PW limiter (never silent).. should we add option in setup to turn this on/off?
New MIDI engine (ported from MEGAfm)



*/

#include <digitalWriteFast.h>
#include <EEPROM.h>
#include <LedControl.h>

#include <TimerOne.h>

void setup() {

	Serial.begin(31250);

	preset_data.arp_speed_base = 100;

	// let sid chip wake up
	DDRD |= _BV(2); // SID1
	DDRD |= _BV(6); // SID2
	DDRD |= _BV(3); // LATCH

	DDRB = 255; // data port
	DDRC = 255; // address port

	// get the clock going for the SID
	init1MhzClock();

	pinMode(A6, OUTPUT); // sid reset
	sidReset();

	pinMode(A0, INPUT);     // mux inputs
	pinMode(A1, INPUT);     // mux inputs
	pinMode(A2, INPUT);     // mux inputs
	pinMode(A3, INPUT);     // butt inputs
	pinMode(A4, INPUT);     // butt inputs
	digitalWrite(A3, HIGH); // pullup
	digitalWrite(A4, HIGH); // pullup

	mux(9);

	// are we holding preset down button at startup?
	if ((PINA & _BV(4)) == 0) {
		sendDump();
	} else {
		mux(3);
		if ((PINA & _BV(4)) == 0) {
			recieveDump();
		}
	}

	pinMode(16, INPUT);
	digitalWrite(16, HIGH); // CV switch1
	pinMode(17, INPUT);
	digitalWrite(17, HIGH); // CV switch2
	pinMode(18, INPUT);
	digitalWrite(18, HIGH); // CV switch3

	pinMode(A7, INPUT);
	digitalWrite(A7, HIGH); // gate

	// turns on display
	mydisplay.shutdown(0, false);
	mydisplay.setIntensity(0, 1); // 15 = brightest

	showVersion();

	boot();
	delay(500);

	Timer1.initialize(100);      //
	Timer1.attachInterrupt(isr); // attach the service routine here

	DDRC = B11111000;

	volume = EEPROM.read(3991);
	if ((volume > 15) || (volume < 1))
		volume = 15; // let's avoid silent sids!!!
	volumeChanged = true;

	int preset_tmp;
	preset_tmp = EEPROM.read(3999);
	if ((preset_tmp > 99) || (preset_tmp < 1))
		preset_tmp = 1;
	preset = preset_tmp;

	masterChannel = EEPROM.read(3998);
	if ((masterChannel > 16) || (masterChannel < 1)) {
		masterChannel = 1;
	}
	masterChannelOut = EEPROM.read(3997);
	if ((masterChannelOut > 16) || (masterChannelOut < 1)) {
		masterChannelOut = 1;
	}

	if (EEPROM.read(3986) > 0) {
		armSID = false;
	} else {
		armSID = true;
	}
	if (EEPROM.read(3996) > 0) {
		sendLfo = false;
	} else {
		sendLfo = true;
	}
	if (EEPROM.read(3995) > 0) {
		sendArp = false;
	} else {
		sendArp = true;
	}

	if (EEPROM.read(3994) > 0) {
		modToLfo = true;
	} else {
		modToLfo = false;
	}

	if (EEPROM.read(3993) > 0) {
		aftertouchToLfo = true;
	} else {
		aftertouchToLfo = false;
	}

	if (EEPROM.read(3992) > 0) {
		velocityToLfo = true;
	} else {
		velocityToLfo = false;
	}

	if (EEPROM.read(3990) > 0) {
		pwLimit = true;
	} else {
		pwLimit = false;
	}

	voice1Channel = EEPROM.read(3989);
	if ((voice1Channel > 16) || (voice1Channel < 1)) {
		voice1Channel = 2;
	}

	voice2Channel = EEPROM.read(3988);
	if ((voice2Channel > 16) || (voice1Channel < 1)) {
		voice2Channel = 3;
	}

	voice3Channel = EEPROM.read(3987);
	if ((voice3Channel > 16) || (voice1Channel < 1)) {
		voice3Channel = 4;
	}

	setupMux();

	sidReset(); // present sustained Note at startup
}
