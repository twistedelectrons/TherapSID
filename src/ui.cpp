#include <EEPROM.h>

#include "ui_vars.h"
#include "ui_leds.h"
#include "ui_controller.h"
#include "preset.h"
#include "display.h"
#include "globals.h"

static UiDisplayController ui_display_controller;

void ui_loop() {

	if (shape1PressedTimer > 10000) {
		shape1Pressed = false;
		shape1PressedTimer = 0;
		preset_data.paraphonic = !preset_data.paraphonic;
		if (preset_data.paraphonic)
			preset_data.arp_mode = 0;
	}

	dotTick();

	if (jumble) {
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
		ui_display_controller.temp_7seg(10, 19, 200);
	} // show CH when autochord is toggled

	ui_display_controller.update(preset, preset_data, ui_state);
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
			ui_display_controller.temp_7seg(10, 11, 500);

			lfoButtTimer = 0;
			lfoButtPressed = false;
		}
	} // delete LFO stuff
}
