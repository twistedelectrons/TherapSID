#include <EEPROM.h>

#include "ui_vars.h"
#include "ui_leds.h"
#include "ui_controller.h"
#include "preset.h"
#include "display.h"
#include "globals.h"
#include "asid.h"

// active sensing
#include "midi.h"

static UiDisplayController ui_display_controller;

static int activeSensingTimer = 0;
extern byte turboMidiXrate;

void ui_loop() {

	if (shape1PressedTimer > 10000) {
		shape1Pressed = false;
		shape1PressedTimer = 0;
		preset_data.paraphonic = !preset_data.paraphonic;
		if (preset_data.paraphonic)
			preset_data.arp_mode = 0;
	}

	dotTick();

	if (jumble && !asidState.enabled) {
		load(1);
		jumble = 0;
	}

	if (!ui_state.saveMode && presetLast != preset) {
		loadTimer = 800;
		load(preset);
		presetLast = preset;
		EEPROM.update(EEPROM_ADDR_PRESET_LAST, presetLast);
	}
	if (saveBounce)
		saveBounce--;
	if (frozen) {
		frozen--;
	}

	if (autoChordChanged) {
		autoChordChanged = false;

		if (autoChord && !clearAutoChord) {
			ui_display_controller.temp_7seg(DIGIT_C, DIGIT_H, 500);
		} else {
			ui_display_controller.temp_7seg(DIGIT_O, DIGIT_F, 500);
		}

	} // show CH or OF when autochord is toggled

	ui_display_controller.update(preset, preset_data, ui_state, turboMidiXrate);
}

void ui_tick() {
	if (shape1Pressed) {
		shape1PressedTimer++;
	}

	if (resetDown) {
		resetDownTimer++;
		if (resetDownTimer > 16000) {
			resetDown = 0;
			resetDownTimer = 0;
			jumble = 1;
		}
	}

	if (loadTimer)
		loadTimer--;

	if ((presetUp || presetDown)) {
		if (presetUp && !presetDown) {
			presetScrollTimer += 6;
			if (presetScrollTimer > presetScrollSpeed) {
				presetScrollTimer = 0;
				if (presetScrollSpeed > 3001) {
					presetScrollSpeed -= 1000;
				}
				preset++;
				if (preset > PRESET_NUMBER_MAX) {
					preset = PRESET_NUMBER_MIN;
				}
				scrolled = true;
			}
		} else if (!presetUp && presetDown) {
			presetScrollTimer += 6;
			if (presetScrollTimer > presetScrollSpeed) {
				presetScrollTimer = 0;
				if (presetScrollSpeed > 3001) {
					presetScrollSpeed -= 1000;
				}
				preset--;
				if (preset < PRESET_NUMBER_MIN) {
					preset = PRESET_NUMBER_MAX;
				}
				scrolled = true;
			}
		}
	}

	if (lfoButtPressed) {
		lfoButtTimer++;
		if (lfoButtTimer == 6000) {

			for (int i = 0; i < 20; i++) {
				preset_data.lfo[ui_state.selectedLfo].mapping[i] = 0;
			}
			ui_display_controller.temp_7seg(DIGIT_C, DIGIT_L, 500);

			lfoButtTimer = 0;
			lfoButtPressed = false;
		}
	} // delete LFO stuff

	// If Active Sensing not already queued up, check if it should.
	if (!activeSensing) {
		// 150 ms apart is within spec
		if (activeSensingTimer++ > 10 * 150) {
			activeSensing = true;
			activeSensingTimer = 0;
		}
	}
}

void force_display_update() { ui_display_controller.force_update(); }