#include <EEPROM.h>

#include "ui_vars.h"
#include "ui_leds.h"
#include "ui_controller.h"
#include "preset.h"
#include "display.h"
#include "globals.h"
#include "asid.h"
#include "sid.h"

// active sensing
#include "midi.h"

static UiDisplayController ui_display_controller;

static int activeSensingTimer = 0;
extern byte turboMidiXrate;

void saveFilterSetup() {
	EEPROM.update(EEPROM_ADDR_SID1_OFFSET, sid_chips[0].get_filtersetup_offset());
	EEPROM.update(EEPROM_ADDR_SID1_RANGE, sid_chips[0].get_filtersetup_range());
	EEPROM.update(EEPROM_ADDR_SID2_OFFSET, sid_chips[1].get_filtersetup_offset());
	EEPROM.update(EEPROM_ADDR_SID2_RANGE, sid_chips[1].get_filtersetup_range());
	EEPROM.update(EEPROM_ADDR_SID3_OFFSET, sid_chips[2].get_filtersetup_offset());
	EEPROM.update(EEPROM_ADDR_SID3_RANGE, sid_chips[2].get_filtersetup_range());
}

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
		if (resetDownTimer > 10000) {
			resetDown = 0;
			resetDownTimer = 0;

			// handling reset for filter setup mode in the synth engine
			if (ui_state.filterSetupMode && !asidState.enabled) {

				// store resetted filterSetupMode
				saveFilterSetup();

				// display "rC" and exit mode
				ui_display_controller.temp_7seg(DIGIT_R, DIGIT_C, 500);
				ui_state.filterSetupMode = false;

			} else {

				// re-activate filter calibration for synth engine
				for (byte chip = 0; chip <= SIDCHIPS - 1; chip++) {
					sid_chips[chip].enable_filtersetup(true);
				}

				jumble = 1;
			}
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

			// holding ARP Mode + LFO CHAIN button
			if (arpModeHeld && lfoButtPressed - 1 < SIDCHIPS) {
#if SID_FILTER_CALIBRATION == 1
				// enter filterSetupMode
				ui_state.filterSetupMode = true;

				// reset last pot and display "FC"
				ui_state.lastPot = POT_NONE;
				ui_display_controller.temp_7seg(DIGIT_F, DIGIT_C, 500);

#endif
			} else {
				if (ui_state.filterSetupMode && lfoButtPressed - 1 < SIDCHIPS) {

					// store filterSetupMode
					saveFilterSetup();

					// display "SC" and exit mode
					ui_display_controller.temp_7seg(DIGIT_S, DIGIT_C, 500);
					ui_state.filterSetupMode = false;

				} else {
					for (int i = 0; i < 20; i++) {
						preset_data.lfo[ui_state.selectedLfo].mapping[i] = 0;
					}
					ui_display_controller.temp_7seg(DIGIT_C, DIGIT_L, 500);
				}
			}

			lfoButtTimer = 0;
			lfoButtPressed = 0;
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

#if SID_FILTER_CALIBRATION == 1
	// filter setup mode indicator
	if (ui_state.filterSetupMode) {
		filterSetupTimer++;
		if (filterSetupTimer > 1600) {
			filterSetupBlink = !filterSetupBlink;
			filterSetupTimer = 0;
		}
	}
#endif
}

void force_display_update() { ui_display_controller.force_update(); }