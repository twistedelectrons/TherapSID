#include <EEPROM.h>

#include "ui_vars.h"
#include "ui_leds.h"
#include "ui_controller.h"
#include "preset.h"
#include "display.h"
#include "globals.h"

static int presetScrollTimer;

static UiDisplayController ui_display_controller;

void ui_loop() {

	ui_display_controller.update(preset, preset_data, ui_state);

	if (shape1PressedTimer > 10000) {
		shape1Pressed = false;
		shape1PressedTimer = 0;
		preset_data.paraphonic = !preset_data.paraphonic;
	}

	dotTick();

	if (jumble) {
		load(1);
		jumble = 0;
	}

	if ((!saveMode) && (presetLast != preset)) {
		loadTimer = 800;
		load(preset);
		presetLast = preset;
		EEPROM.update(3999, presetLast);
	}
	if (saveBounce)
		saveBounce--;
	if (frozen) {
		frozen--;
	}
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

	if (arpModeHeld) {
		arpModeCounter++;
		if (arpModeCounter > 16000) {
			fatChanged = true;
			arpModeCounter = 0;
			preset_data.fat_mode = static_cast<FatMode>(((int)preset_data.fat_mode + 1) % 4);
		}
	}

	if (presetUp || presetDown) {
		if (presetUp && !presetDown) {
			presetScrollTimer += 4;
			if (presetScrollTimer > presetScrollSpeed) {
				presetScrollTimer = 0;
				if (presetScrollSpeed > 1001) {
					presetScrollSpeed -= 1000;
				}
				preset++;
				if (preset > 99) {
					preset = 1;
				}
				scrolled = true;
			}
		} else if (!presetUp && presetDown) {
			presetScrollTimer += 4;
			if (presetScrollTimer > presetScrollSpeed) {
				presetScrollTimer = 0;
				if (presetScrollSpeed > 1001) {
					presetScrollSpeed -= 1000;
				}
				preset--;
				if (preset < 1) {
					preset = 99;
				}
				scrolled = true;
			}
		}
	}

	if (lfoButtPressed) {
		lfoButtTimer++;
		if (lfoButtTimer == 6000) {

			for (int i = 0; i < 20; i++) {
				preset_data.lfo_map[ui_state.selectedLfo][i] = 0;
			}
			ui_display_controller.temp_7seg(10, 11, 500);

			lfoButtTimer = 0;
			lfoButtPressed = false;
		}
	} // delete LFO stuff
}
