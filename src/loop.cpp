#include "globals.h"
#include "arp.h"
#include "midi.h"
#include "mux.h"
#include "preset.h"
#include "lfo.h"
#include "sid.h"
#include "ui.h"
#include <EEPROM.h>
#include "ui_vars.h"

static bool loadedAfterStartup; // we load the preset after 2sec (when SID is ready)

static int calc_pitch(int note, float frac) {

	static const int32_t sidScale[] = {
	    // dropped by half a semi so that fine value 0 (knob at noon) is concert
	    133,   140,   149,   158,   168,   177,   188,   199,   210,   223,   118,   251,   266,   282,   299,
	    316,   336,   355,   377,   399,   422,   448,   475,   503,   533,   564,   598,   633,   671,   711,
	    754,   798,   846,   897,   948,   1006,  1066,  1130,  1197,  1268,  1344,  1423,  1508,  1597,  1693,
	    1793,  1900,  2012,  2133,  2259,  2393,  2536,  2687,  2847,  3016,  3195,  3386,  3587,  3800,  4026,
	    4265,  4519,  4788,  5073,  5374,  5694,  6032,  6392,  6772,  7174,  7601,  8053,  8532,  9039,  9577,
	    10147, 10749, 11389, 12066, 12783, 13543, 14349, 15203, 16106, 17064, 18078, 19155, 20310, 21500, 22778,
	    24133, 25567, 27088, 28699, 30357, 32213, 34129, 36158, 38308, 40587, 43000, 45557, 48266, 51136, 54176,
	    57398, 60811, 32213, 34129, 36158, 38308, 40587, 43000, 45557, 48266, 51136, 54176, 57398, 60811, 32213,
	    34129, 36158, 38308, 40587, 43000, 45557, 48266, 51136, 54176, 57398, 60811};

	if (note > 127) {
		note = 127;
	} else if (note - 13 < 0) {
		note = 13;
	}

	float fine = frac / 2 + 0.5;
	fine *= .90;

	return (sidScale[note - 12 - 1] + (sidScale[note - 12 + 2] - sidScale[note - 12]) * fine);
}

static void calculatePitch() {
	int lfo_tune[3] = {lfoTune1 + lfoTune2 + lfoTune3, lfoTune4 + lfoTune5 + lfoTune6, lfoTune7 + lfoTune8 + lfoTune9};

	float lfo_fine[3] = {lfoFine1 + lfoFine2 + lfoFine3, lfoFine4 + lfoFine5 + lfoFine6,
	                     lfoFine7 + lfoFine8 + lfoFine9};

	float bends[3] = {bend1, bend2, bend3};

	for (int oper = 0; oper < 6; oper++) {
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

		glide[oper].destination_pitch =
		    calc_pitch(key + preset_data.voice[voice_knob_idx].tune_base + lfo_tune[voice_knob_idx],
		               preset_data.voice[voice_knob_idx].fine_base + lfo_fine[voice_knob_idx] + my_bend / 0.9);
	}
}
void setSidRegisters(Preset const& preset, ParamsAfterLfo const& params_after_lfo) {
	for (int i = 0; i < 2; i++) {
		sid_chips[i].set_arm_sid_mode(armSID);

		for (int v = 0; v < 3; v++) {
			int oper = v + 3 * i;
			int pv = voice_index[oper];

			sid_chips[i].set_attack_decay(v, preset.voice[pv].attack, preset.voice[pv].decay);
			sid_chips[i].set_sustain_release(v, preset.voice[pv].sustain, preset.voice[pv].release);
			sid_chips[i].set_pulsewidth(v, params_after_lfo.pulsewidth[pv]);

			auto pitch = glide[oper].current_pitch();
			sid_chips[i].set_freq(v, i == 0 ? pitch : preset.fatten_pitch(pitch));

			auto gate = voice_state.gate(oper) || control_voltage_note.has_value();
			sid_chips[i].set_reg_control(v, preset.voice[pv].reg_control | (gate ? 0x01 : 0x00));
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
		}

		sid_chips[i].send_next_update_pair();
	}
}

void loop() {

	if (ui_state.saveFlag) {
		saveBounce = 1600;
		save();
		ui_state.saveMode = false;
		ui_state.saveFlag = false;
	}
	if (volumeChanged) {
		// update volume
		sid_chips[0].set_volume(volume);
		sid_chips[1].set_volume(volume);
		EEPROM.update(EEPROM_ADDR_MASTER_VOLUME, volume);
		volumeChanged = false;
	}

	//  load the first preset after all the butts and pots have been scanned
	if (!loadedAfterStartup) {
		if (millis() > 1400) {
			load(preset);
			loadedAfterStartup = true;
		}
	}

	ui_loop();

	midiRead();
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
	if ((PINC & _BV(2)) == 0) {
		cvActive[2] = true;
		mux(10);
		lfo[2] = analogRead(A2) >> 2;
	} else {
		cvActive[2] = false;
	}

	readMux();

	static byte voice_index_000000[6] = {0, 0, 0, 0, 0, 0};
	static byte voice_index_000111[6] = {0, 0, 0, 1, 1, 1};
	static byte voice_index_012012[6] = {0, 1, 2, 0, 1, 2};
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
			voice_state.set_n_individual_voices(6);
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
