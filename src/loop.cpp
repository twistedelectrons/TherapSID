#include <EEPROM.h>

#include "globals.h"
#include "display.h"
#include "arp.h"
#include "midi.h"
#include "mux.h"
#include "preset.h"
#include "leds.h"
#include "paraphonic.h"
#include "lfo.h"
#include "sid.h"

static bool loadedAfterStartup; // we load the preset after 2sec (when SID is ready)
static bool saveModeFlash;

void loop() {

	// load the first preset after all the butts and pots have been scanned
	if (!loadedAfterStartup) {
		if (millis() > 1400) {
			load(preset);
			loadedAfterStartup = true;
			ledNumber(preset);
		}
	}
	if (saveModeTimer > 2000) {
		saveModeTimer = 0;
		saveModeFlash = !saveModeFlash;
		if (saveModeFlash) {
			digit(0, 99);
			digit(1, 99);
		} else {
			ledNumber(preset);
		}
	}

	if (fatShow) {
		digit(0, 12);
		digit(1, (int)fatMode + 1);
		fatShow = false;
		unShowFilterAssigns();
	}

	midiRead();
	if (arpCounter >= arpSpeed + 100) {
		arpCounter = 0;
		arpTick();
	}

	if (shape1PressedTimer > 10000) {
		shape1Pressed = false;
		shape1PressedTimer = 0;
		pa = !pa;
		paraChange();
	}

	if (dotTimer) {
		dotTimer--;
		if (!dotTimer) {
			mydisplay.setLed(0, 7, 6, 0);
			mydisplay.setLed(0, 7, 7, 0);
		}
	}
	// pa mode

	if ((gate) && (!pa)) {
		mux(15);
		key = map(analogRead(A2), 0, 1023, 12, 72);
	}

	// CV
	if ((PINC & _BV(0)) == 0) {
		cvActive[0] = true;
		mux(12);
		lfo[0] = analogRead(A2) >> 2;
	} else {
		cvActive[0] = false;
	}
	if ((PINC & _BV(1)) == 0) {
		cvActive[1] = true;
		mux(2);
		lfo[1] = analogRead(A2) >> 2;
	} else {
		cvActive[1] = false;
	}
	if ((PINC & _BV(2)) == 0) {
		cvActive[2] = true;
		mux(10);
		lfo[2] = analogRead(A2) >> 2;
	} else {
		cvActive[2] = false;
	}

	if (jumble) {
		load(1);
		jumble = 0;
	}

	if ((!saveMode) && (presetLast != preset)) {
		load(preset);
		presetLast = preset;
		EEPROM.update(3999, presetLast);
	}
	if (saveBounce)
		saveBounce--;
	if (frozen) {
		frozen--;
	}

	readMux(); // sidUpdate();

	lfoTick(); // sidUpdate();
	calculatePitch();
	sidUpdate();

	if (showPresetNumber) {
		ledNumber(preset);
		showPresetNumber = false;
	}
}
