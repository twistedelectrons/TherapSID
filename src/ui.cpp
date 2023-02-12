#include <EEPROM.h>

#include "ui_vars.h"
#include "ui_leds.h"
#include "preset.h"
#include "display.h"
#include "globals.h"

static int presetScrollTimer;
static bool saveModeFlash;

// FIXME use
void show_arp_mode() {
	// don't get in the way of startup preset display
	if (millis() > 2000)
		frozen = 500;

	switch (preset_data.arp_mode) {
		case 0: // OFF
			digit(0, 0);
			digit(1, 12);
			break;

		case 1: // UP
			digit(0, 13);
			digit(1, 14);
			break;

		case 2: // DOWN
			digit(0, 15);
			digit(1, 0);
			break;

		case 3: // UP/DOWN
			digit(0, 13);
			digit(1, 15);
			break;

		case 4: // UP/DOWN
			digit(0, 16);
			digit(1, 15);
			break;
	}
}

void ui_loop() {

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
		digit(1, (int)preset_data.fat_mode + 1);
		fatShow = false;
	}

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

	if (showPresetNumber) {
		ledNumber(preset);
		showPresetNumber = false;
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

	if (filterModeHeld) {
		arpModeCounter++;
		if (arpModeCounter > 25000) {
			fatChanged = true;
			arpModeCounter = 0;
			preset_data.fat_mode = static_cast<FatMode>(((int)preset_data.fat_mode + 1) % 4);
			fatShow = true;
		}
	}

	if (saveMode) {
		saveModeTimer++;
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
				showPresetNumber = true;
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
				showPresetNumber = true;
				scrolled = true;
			}
		}
	}

	if (lfoButtPressed) {
		lfoButtTimer++;
		if (lfoButtTimer == 6000) {

			for (int i = 0; i < 20; i++) {
				preset_data.lfo_map[selectedLfo][i] = 0;
			}
			digit(0, 10);
			digit(1, 11);

			lfoButtTimer = 0;
			lfoButtPressed = false;
		}
	} // delete LFO stuff
}
