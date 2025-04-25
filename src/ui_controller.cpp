#include <Arduino.h>
#include "preset.h"
#include "util.hpp"
#include "ui_leds.h"
#include "ui_vars.h"
#include "globals.h"
#include "ui_controller.h"

static int scaleFine(int input) {
	input = map(input, 0, 1023, -50, 50);

	return (input);
}

static byte scale100(int input) {
	input = map(input, 0, 1023, 0, 100);
	if (input > 99)
		input = 99;
	return input;
}

void UiDisplayController::temp_7seg(int digit0, int digit1, int time) {
	last_changed_digit0 = digit0;
	last_changed_digit1 = digit1;
	last_changed_counter = time;
}

void UiDisplayController::update(int preset_number, const Preset& preset, const UiState& ui_state,
                                 byte turboMidiXrate) {
	update_leds(preset, ui_state);
	update_7seg(preset_number, preset, ui_state, turboMidiXrate);
}

void UiDisplayController::update_leds(const Preset& p, const UiState& ui_state) {
	assert(ui_state.lastPot <= 20);
	assert(ui_state.selectedLfo < 3);

	int n_unisono_voices;
	if (!p.paraphonic)
		n_unisono_voices = 3;
	else if (p.fat_mode != FatMode::PARA_2OP)
		n_unisono_voices = 1;
	else
		n_unisono_voices = 2;

	if (!ui_state.filterModeHeld) {
		for (int i = 0; i < 3; i++) {
			set_led(4 * i + 1, i < n_unisono_voices && p.voice[i].reg_control & PresetVoice::PULSE);
			set_led(4 * i + 2, i < n_unisono_voices && p.voice[i].reg_control & PresetVoice::TRI);
			set_led(4 * i + 3, i < n_unisono_voices && p.voice[i].reg_control & PresetVoice::SAW);
			set_led(4 * i + 4, i < n_unisono_voices && p.voice[i].reg_control & PresetVoice::NOISE);
		}
	} else {
		for (int i = 0; i < 3; i++)
			for (int k = 0; k < 4; k++)
				set_led(4 * i + 1 + k, i < n_unisono_voices && p.voice[i].filter_enabled);
	}

	set_led(16, 0 < n_unisono_voices && bitRead(p.voice[0].reg_control, 1));
	set_led(17, 0 < n_unisono_voices && bitRead(p.voice[0].reg_control, 2));
	set_led(18, 1 < n_unisono_voices && bitRead(p.voice[1].reg_control, 1));
	set_led(19, 1 < n_unisono_voices && bitRead(p.voice[1].reg_control, 2));
	set_led(20, 2 < n_unisono_voices && bitRead(p.voice[2].reg_control, 1));
	set_led(21, 2 < n_unisono_voices && bitRead(p.voice[2].reg_control, 2));

	switch (p.filter_mode) {
		case FilterMode::LOWPASS:
			set_led(27, true);
			set_led(28, false);
			set_led(29, false);
			break;
		case FilterMode::BANDPASS:
			set_led(27, false);
			set_led(28, true);
			set_led(29, false);
			break;
		case FilterMode::HIGHPASS:
			set_led(27, false);
			set_led(28, false);
			set_led(29, true);
			break;
		case FilterMode::NOTCH:
			set_led(27, true);
			set_led(28, false);
			set_led(29, true);
			break;
		case FilterMode::LB:
			set_led(27, true);
			set_led(28, true);
			set_led(29, false);
			break;
		case FilterMode::BH:
			set_led(27, false);
			set_led(28, true);
			set_led(29, true);
			break;
		case FilterMode::LBH:
			set_led(27, true);
			set_led(28, true);
			set_led(29, true);
			break;
		case FilterMode::OFF:
			set_led(27, false);
			set_led(28, false);
			set_led(29, false);
			break;
	}

		// display filter setup state / lfo mapping (default)
#if SIDCHIPS > 2
	if (ui_state.lastPot != POT_NONE) {
		// lastPot != none
		set_led(13, ui_state.filterSetupMode ? filterSetupBlink : p.lfo[0].mapping[ui_state.lastPot]);
		set_led(14, ui_state.filterSetupMode ? filterSetupBlink : p.lfo[1].mapping[ui_state.lastPot]);
		set_led(15, ui_state.filterSetupMode ? filterSetupBlink : p.lfo[2].mapping[ui_state.lastPot]);
	} else {
		// lastPot == none
		set_led(13, ui_state.filterSetupMode ? filterSetupBlink : false);
		set_led(14, ui_state.filterSetupMode ? filterSetupBlink : false);
		set_led(15, ui_state.filterSetupMode ? filterSetupBlink : false);
	}
#else
	if (ui_state.lastPot != POT_NONE) {
		// lastPot != none
		set_led(13, ui_state.filterSetupMode ? filterSetupBlink : p.lfo[0].mapping[ui_state.lastPot]);
		set_led(14, ui_state.filterSetupMode ? filterSetupBlink : p.lfo[1].mapping[ui_state.lastPot]);
		set_led(15, p.lfo[2].mapping[ui_state.lastPot]);
	} else {
		// lastPot == none
		set_led(13, ui_state.filterSetupMode ? filterSetupBlink : false);
		set_led(14, ui_state.filterSetupMode ? filterSetupBlink : false);
		set_led(15, false);
	}
#endif

	for (int shape = 1; shape <= 5; shape++) {
		if (lfoStep[ui_state.selectedLfo] < 10 && lfoSpeed[ui_state.selectedLfo]) {
			set_led(21 + shape, 0); // blink the lfo LED to thebeat if LFO is moving
		} else {
			set_led(21 + shape, p.lfo[ui_state.selectedLfo].shape == shape);
		}
	}
	set_led(30, p.lfo[ui_state.selectedLfo].retrig);
	set_led(31, p.lfo[ui_state.selectedLfo].looping);
}

void UiDisplayController::show_changed(int value) {
	if (value < 0)
		value = -value;
	if (value > 99)
		value = 99;

	temp_7seg(value / 10, value % 10, 250);
}

void UiDisplayController::show_byte(byte value, bool center) {
	if (center)
		value = (value - 0x7F < 0) ? (0x7F - value) : (value - 0x7F);

	temp_7seg(value / 16, value % 16, 250);
}

void UiDisplayController::show_filterSetupOffset(byte chip) {
	show_byte(sid_chips[chip].get_filtersetup_offset(), true);
}

void UiDisplayController::show_filterSetupRange(byte chip) { show_byte(sid_chips[chip].get_filtersetup_range(), true); }

void UiDisplayController::show_arp_mode(int arp_mode) {
	switch (arp_mode) {
		case 0: // OFF
			temp_7seg(DIGIT_O, DIGIT_F, 500);
			break;

		case 1: // UP
			temp_7seg(DIGIT_U, DIGIT_P, 500);
			break;

		case 2: // DOWN
			temp_7seg(DIGIT_D, DIGIT_O, 500);
			break;

		case 3: // UP/DOWN
			temp_7seg(DIGIT_U, DIGIT_D, 500);
			break;

		case 4: // RANDOM
			temp_7seg(DIGIT_R, DIGIT_D, 500);
			break;

		case 5: // OCTAVE
			temp_7seg(DIGIT_O, DIGIT_C, 500);
			break;
		case 6: // TRILL (+1 SEMITONE)
			temp_7seg(DIGIT_T, 1, 500);
			break;

		case 7: // TRILL (+2 SEMITONES)
			temp_7seg(DIGIT_T, 2, 500);
			break;
	}
}

void UiDisplayController::update_7seg(int preset_number, const Preset& preset, const UiState& ui_state,
                                      byte turboMidiXrate) {

	if (preset.fat_mode != old_preset.fat_mode)
		temp_7seg(DIGIT_F, (int)preset.fat_mode + 1, 2000);

	if (preset.arp_mode != old_preset.arp_mode)
		show_arp_mode(preset.arp_mode);

	for (int i = 0; i < 3; i++) {
		const auto& v = preset.voice[i];
		const auto& ov = old_preset.voice[i];
		if (v.pulsewidth_base != ov.pulsewidth_base)
			show_changed(scale100(v.pulsewidth_base / 2));

		if (v.tune_base != ov.tune_base)
			show_changed(v.tune_base - 12);

		if (v.fine_base != ov.fine_base)
			show_changed(scaleFine(v.fine_base * 1023.f));

		if (v.glide != ov.glide)
			show_changed(v.glide);

		if (v.attack != ov.attack)
			show_changed(v.attack);

		if (v.decay != ov.decay)
			show_changed(v.decay);

		if (v.sustain != ov.sustain)
			show_changed(v.sustain);

		if (v.release != ov.release)
			show_changed(v.release);
	}

	// display filter setup pot changes
#if SIDCHIPS > 2
	if (ui_state.filterSetupMode) {
#else
	if (ui_state.filterSetupMode && ui_state.lastPot != 13 && ui_state.lastPot != 14) {
#endif
		// detect pot offset
		byte chip = 0xFF; // NONE

		if (ui_state.lastPot == 9)
			chip = 0;
		if (ui_state.lastPot == 11)
			chip = 1;
		if (ui_state.lastPot == 13)
			chip = 2;

		if (chip != 0xFF)
			show_filterSetupOffset(chip);

		// detect pot range
		chip = 0xFF; // NONE

		if (ui_state.lastPot == 10)
			chip = 0;
		if (ui_state.lastPot == 12)
			chip = 1;
		if (ui_state.lastPot == 14)
			chip = 2;

		if (chip != 0xFF)
			show_filterSetupRange(chip);

	} else {
		for (int i = 0; i < 3; i++) {
			const auto& l = preset.lfo[i];
			const auto& ol = old_preset.lfo[i];

			if (l.speed != ol.speed)
				show_changed(scale100(l.speed / 1.3f));

			if (l.depth != ol.depth)
				show_changed(scale100(l.depth));
		}
	}

	// display filter setup current values
	if (ui_state.filterSetupMode && filterSetupShowOfSid) {

		byte chip = filterSetupShowOfSid - 1;

		if (filterSetupShowIndex) {
			show_filterSetupRange(chip);
		} else {
			show_filterSetupOffset(chip);
		}

		// next index
		filterSetupShowIndex = !filterSetupShowIndex;
		filterSetupShowOfSid = 0;
	}

	if (preset.cutoff != old_preset.cutoff)
		show_changed(scale100(preset.cutoff));

	if (preset.resonance_base != old_preset.resonance_base)
		show_changed(preset.resonance_base);

	if (preset.arp_speed_base != old_preset.arp_speed_base)
		show_changed(scale100(1023 - 4 * preset.arp_speed_base));

	if (preset.arp_range_base != old_preset.arp_range_base)
		show_changed(preset.arp_range_base);

	if (preset.paraphonic != old_preset.paraphonic) {
		if (preset_data.paraphonic == true) {
			temp_7seg(DIGIT_P, DIGIT_A, 500);
		} else {
			temp_7seg(DIGIT_O, DIGIT_F, 500);
		} // "PA" or "OF"
	}

	if (old_preset_number != preset_number) {
		// un-show whatever what was show_changed() before.
		temp_7seg(DONTCARE, DONTCARE, 0);
	}

	old_preset = preset;
	old_preset_number = preset_number;

	if (old_turbomidi_x_rate != turboMidiXrate) {
		temp_7seg(DIGIT_T, turboMidiXrate, 500);
		old_turbomidi_x_rate = turboMidiXrate;
	}

	int digit0, digit1;
	switch (ui_state.midiSetup) {
		case 1:
			digit0 = masterChannel / 10;
			digit1 = masterChannel % 10;
			break;
		case 2:
			digit0 = masterChannelOut / 10;
			digit1 = masterChannelOut % 10;
			break;
		case 3:
			digit0 = 99;
			digit1 = 99;
			break;
		default:
			if (last_changed_counter && !ui_state.saveMode) {
				last_changed_counter--;
				digit0 = last_changed_digit0;
				digit1 = last_changed_digit1;
			} else {
				if (!ui_state.saveMode || millis() % 512 > 31) {
					digit0 = preset_number / 10;
					digit1 = preset_number % 10;
				} else {
					digit0 = 99;
					digit1 = 99;
				}
			}
	}

	if (digit0 != old_digit0) {
		old_digit0 = digit0;
		digit(0, digit0);
	}
	if (digit1 != old_digit1) {
		old_digit1 = digit1;
		digit(1, digit1);
	}
}

void UiDisplayController::set_led(int index, bool value) {
	if (leds[index] != value) {
		leds[index] = value;
		ledSet(index, value);
	}
}

void UiDisplayController::force_update() {
	for (byte i = 0; i < sizeof(leds); i++) {
		leds[i] = false;
		ledSet(i, false);
	}
	old_digit0 = old_digit1 = -1;
}
