#include "globals.h"
#include "arp.h"
#include "midi.h"
#include "mux.h"
#include "preset.h"
#include "lfo.h"
#include "sid.h"
#include "ui.h"
#include "asid.h"
#include <EEPROM.h>

static bool loadedAfterStartup; // we load the preset after 2sec (when SID is ready)

static int calc_pitch(float note_exact) {

	static const uint16_t sidScale[] = {
	    // From C-1 to A#7 (A=440 Hz)
	    137,   145,   154,   163,   173,   183,   194,   206,   218,   231,   244,   259,   274,   291,   308,   326,
	    346,   366,   388,   411,   435,   461,   489,   518,   549,   581,   616,   652,   691,   732,   776,   822,
	    871,   923,   978,   1036,  1097,  1163,  1232,  1305,  1383,  1465,  1552,  1644,  1742,  1845,  1955,  2071,
	    2195,  2325,  2463,  2610,  2765,  2930,  3104,  3288,  3484,  3691,  3910,  4143,  4389,  4650,  4927,  5220,
	    5530,  5859,  6207,  6577,  6968,  7382,  7821,  8286,  8779,  9301,  9854,  10440, 11060, 11718, 12415, 13153,
	    13935, 14764, 15642, 16572, 17557, 18601, 19708, 20879, 22121, 23436, 24830, 26306, 27871, 29528, 31284, 33144,
	    35115, 37203, 39415, 41759, 44242, 46873, 49660, 52613, 55741, 59056, 62567};

	byte note = (int)note_exact;
	float frac = note_exact - note;

	// Compensate for Tune knob being offset by 12, and clamp pitch&bend upwards.
	note = max(0, note - 12);
	if (note >= sizeof(sidScale) / (sizeof(*sidScale)) - 1) {
		note = sizeof(sidScale) / (sizeof(*sidScale)) - 1;
		frac = 0.0;
	}

	// Pitch is note plus distance to next
	return (sidScale[note] + (sidScale[note + 1] - sidScale[note]) * frac);
}

static void calculatePitch() {
	int lfo_tune[3] = {lfoTune1 + lfoTune2 + lfoTune3, lfoTune4 + lfoTune5 + lfoTune6, lfoTune7 + lfoTune8 + lfoTune9};

	float lfo_fine[3] = {lfoFine1 + lfoFine2 + lfoFine3, lfoFine4 + lfoFine5 + lfoFine6,
	                     lfoFine7 + lfoFine8 + lfoFine9};

	float bends[3] = {bend1, bend2, bend3};

	for (int oper = 0; oper < SIDVOICES_TOTAL; oper++) {
		int oper_mod3 = oper < 3 ? oper : (oper - 3);
		int voice_knob_idx = voice_index[oper];
		int key;
		if (preset_data.arp_mode) {
			key = arp_output_note;
		} else if (control_voltage_note.has_value()) {
			key = *control_voltage_note;
		} else {
			key = voice_state.key(oper);
		}

		float my_bend = voice_state.has_individual_override(oper) ? bends[oper_mod3] : bend;

		float note_exact = key + preset_data.voice[voice_knob_idx].tune_base + lfo_tune[voice_knob_idx] + my_bend +
		                   (preset_data.voice[voice_knob_idx].fine_base - 0.5) + lfo_fine[voice_knob_idx];

		glide[oper].destination_pitch = calc_pitch(note_exact);
	}
}
void setSidRegisters(Preset const& preset, ParamsAfterLfo const& params_after_lfo) {
	for (int i = 0; i < SIDCHIPS; i++) {
		for (int v = 0; v < 3; v++) {
			int oper = v + 3 * i;
			int pv = voice_index[oper];

			sid_chips[i].set_attack_decay(v, preset.voice[pv].attack, preset.voice[pv].decay);
			sid_chips[i].set_sustain_release(v, preset.voice[pv].sustain, preset.voice[pv].release);
			sid_chips[i].set_pulsewidth(v, params_after_lfo.pulsewidth[pv]);

			auto pitch = glide[oper].current_pitch();
			sid_chips[i].set_freq(v, i == 0 ? pitch : preset.fatten_pitch(pitch, i));

			// Limit Unison and Octave to two SIDs
			if (!((i == 2) &&
			      ((preset_data.fat_mode == FatMode::UNISONO) || (preset_data.fat_mode == FatMode::OCTAVE_UP)))) {
				auto gate = voice_state.gate(oper) || control_voltage_note.has_value();
				sid_chips[i].set_reg_control(v, preset.voice[pv].reg_control | (gate ? 0x01 : 0x00));
			}
		}

		// disable voice->filter routing if voice is off or filter is off.
		sid_chips[i].set_resonance_and_filter_enable(
		    params_after_lfo.resonance,
		    preset.voice[voice_index[3 * i + 0]].wants_filter() && preset.filter_mode != FilterMode::OFF,
		    preset.voice[voice_index[3 * i + 1]].wants_filter() && preset.filter_mode != FilterMode::OFF,
		    preset.voice[voice_index[3 * i + 2]].wants_filter() && preset.filter_mode != FilterMode::OFF,
		    preset.filter_mode != FilterMode::OFF);
		sid_chips[i].set_filter_cutoff(params_after_lfo.cutoff);

		switch (preset.filter_mode) {
			case FilterMode::OFF:
				sid_chips[i].set_filter_mode(0);
				break;
			case FilterMode::LOWPASS:
				sid_chips[i].set_filter_mode(Sid::LOWPASS);
				break;
			case FilterMode::HIGHPASS:
				sid_chips[i].set_filter_mode(Sid::HIGHPASS);
				break;
			case FilterMode::BANDPASS:
				sid_chips[i].set_filter_mode(Sid::BANDPASS);
				break;
			case FilterMode::NOTCH:
				sid_chips[i].set_filter_mode(Sid::LOWPASS | Sid::HIGHPASS);
				break;
			case FilterMode::LB:
				sid_chips[i].set_filter_mode(Sid::LOWPASS | Sid::BANDPASS);
				break;
			case FilterMode::BH:
				sid_chips[i].set_filter_mode(Sid::BANDPASS | Sid::HIGHPASS);
				break;
			case FilterMode::LBH:
				sid_chips[i].set_filter_mode(Sid::LOWPASS | Sid::BANDPASS | Sid::HIGHPASS);
				break;
		}

		sid_chips[i].send_next_update_pair();
	}
}

void loop() {

	// If ASID is on, just run that and ignore everything else
	if (asidState.enabled) {
		midiRead();

		// Read some muxes between MIDI bytes to improve button responsiveness
		for (byte i = 0; i < 3; i++) {
			readMux();
		}

		// Held down RESET (normally 'random') will exit
		if (jumble) {
			jumble = 0;
			asidState.enabled = false;
			force_display_update();
			load(preset);
			volumeChanged = true;
		}

		return;
	}

	// Keep arp octave <= range knob
	if (arpRound > arpRange) {
		arpRound = arpRange;
	}

	if (volumeChanged) {
		// update volume
		for (int i = 0; i < SIDCHIPS; i++) {
			sid_chips[i].set_volume(volume);
		}
		EEPROM.update(EEPROM_ADDR_MASTER_VOLUME, volume);
		volumeChanged = false;
	}

	//  load the first preset after all the butts and pots have been scanned
	if (!loadedAfterStartup) {
		if (millis() > 1400) {
			load(preset);
			loadedAfterStartup = true;

			// init filter setup
			for (int i = 0; i < SIDCHIPS; i++) {

				sid_chips[i].set_filtersetup_offset(filterSetupSidOffset[i]);
				sid_chips[i].set_filtersetup_range(filterSetupSidRange[i]);
			}
		}
	}

	ui_loop();

	midiRead();

	if (asidState.enabled) {
		// A MIDI message might enable ASID mode - and at that point we want to
		// be in full control of the SIDs, so don't do more stuff if so.
		return;
	}

	if (arpCounter >= arpSpeed + 100) {
		arpCounter = 0;
		arpTick();
	}

	bool control_voltage_gate = (PINA & _BV(7)) == 0;
	if (control_voltage_gate && !preset_data.is_polyphonic()) {
		mux(15);
		control_voltage_note = map(analogRead(A2), 0, 1023, 12, 72);
	} else {
		control_voltage_note = nullopt;
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
#if SIDCHIPS <= 2
	if ((PINC & _BV(2)) == 0) {
		cvActive[2] = true;
		mux(10);
		lfo[2] = analogRead(A2) >> 2;
	} else {
#endif
		cvActive[2] = false;
#if SIDCHIPS <= 2
	}
#endif

	readMux();

#if SIDCHIPS <= 2
	static byte voice_index_000000[6] = {0, 0, 0, 0, 0, 0};
	static byte voice_index_000111[6] = {0, 0, 0, 1, 1, 1};
	static byte voice_index_012012[6] = {0, 1, 2, 0, 1, 2};
#else
	static byte voice_index_000000[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	static byte voice_index_000111[9] = {0, 0, 0, 0, 1, 1, 1, 1, 1};
	static byte voice_index_012012[9] = {0, 1, 2, 0, 1, 2, 0, 1, 2};
#endif
	if (!preset_data.paraphonic) {
		if (preset_data.fat_mode == FatMode::MORE_VOICES) {
			voice_state.set_n_individual_voices(2);
			voice_index = voice_index_012012;
		} else {
			voice_state.set_n_individual_voices(1);
			voice_index = voice_index_012012;
		}
	} else {
		if (preset_data.fat_mode == FatMode::MORE_VOICES) {
			voice_state.set_n_individual_voices(SIDVOICES_TOTAL);
			voice_index = voice_index_000000;
		} else if (preset_data.fat_mode == FatMode::PARA_2OP) {
			voice_state.set_n_individual_voices(3);
			voice_index = voice_index_000111;
		} else {
			voice_state.set_n_individual_voices(3);
			voice_index = voice_index_000000;
		}
	}
	ParamsAfterLfo params_after_lfo = lfoTick();
	calculatePitch();
	setSidRegisters(preset_data, params_after_lfo);
}
