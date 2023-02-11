#include "globals.h"
#include "display.h"
#include "arp.h"
#include "midi.h"
#include "mux.h"
#include "preset.h"
#include "leds.h"
#include "lfo.h"
#include "sid.h"
#include "ui.h"

static bool loadedAfterStartup; // we load the preset after 2sec (when SID is ready)

static int calc_pitch(int note, float frac) {
	static const int32_t sidScale[] = {
		137,   145,   154,   163,   173,   183,   194,   205,   217,   230,   122,   259,   274,   291,   308,
		326,   346,   366,   388,   411,   435,   461,   489,   518,   549,   581,   616,   652,   691,   732,
		776,   822,   871,   923,   976,   1036,  1097,  1163,  1232,  1305,  1383,  1465,  1552,  1644,  1742,
		1845,  1955,  2071,  2195,  2325,  2463,  2610,  2765,  2930,  3104,  3288,  3484,  3691,  3910,  4143,
		4389,  4650,  4927,  5220,  5530,  5859,  6207,  6577,  6968,  7382,  7821,  8286,  8779,  9301,  9854,
		10440, 11060, 11718, 12415, 13153, 13935, 14764, 15642, 16572, 17557, 18601, 19709, 20897, 22121, 23436,
		24830, 26306, 27871, 29528, 31234, 33144, 35115, 37203, 39415, 41759, 44242, 46873, 49660, 52613, 55741,
		59056, 62567, 33144, 35115, 37203, 39415, 41759, 44242, 46873, 49660, 52613, 55741, 59056, 62567, 33144,
		35115, 37203, 39415, 41759, 44242, 46873, 49660, 52613, 55741, 59056, 62567,
	};

	if (note > 127) {
		note = 127;
	} else if (note - 13 < 0) {
		note = 13;
	}

	float fine = frac / 2 + 0.5;
	fine *= .90;

	return sidScale[note - 12 - 1] + (sidScale[note - 12 + 2] - sidScale[note - 12]) * fine;
}

static void calculatePitch() {
	int lfo_tune[3] = {
		lfoTune1 + lfoTune2 + lfoTune3,
		lfoTune4 + lfoTune5 + lfoTune6,
		lfoTune7 + lfoTune8 + lfoTune9
	};

	float lfo_fine[3] = {
		lfoFine1 + lfoFine2 + lfoFine3,
		lfoFine4 + lfoFine5 + lfoFine6,
		lfoFine7 + lfoFine8 + lfoFine9
	};

	for (int oper = 0; oper < 6; oper++) {
		int voice_knob_idx = preset_data.paraphonic ? 0 : (oper % 3);
		glide[oper].destination_pitch = calc_pitch(
			voice_state.key(oper) + preset_data.voice[voice_knob_idx].tune_base + lfo_tune[voice_knob_idx],
			preset_data.voice[voice_knob_idx].fine_base + lfo_fine[voice_knob_idx] + bend / 0.9
		);
	}
}
void setSidRegisters(Preset const& preset, ParamsAfterLfo const& params_after_lfo) {
	for (int i=0; i<2; i++) {
		for (int v=0; v<3; v++) {
			int pv = preset_data.paraphonic ? 0 : v;
			int oper = v + 3*i;
			sid_chips[i].set_attack_decay(v, preset.voice[pv].attack, preset.voice[pv].decay);
			sid_chips[i].set_sustain_release(v, preset.voice[pv].sustain, preset.voice[pv].release);
			sid_chips[i].set_pulsewidth(v, params_after_lfo.pulsewidth[pv]);

			auto pitch = glide[oper].current_pitch();
			sid_chips[i].set_freq(v, i==0 ? pitch : preset.fatten_pitch(pitch));

			sid_chips[i].set_reg_control(v, preset.voice[pv].reg_control | (voice_state.gate(oper) ? 1 : 0) );
		}

		// disable voice->filter routing if voice is off or filter is off.
		sid_chips[i].set_resonance_and_filter_enable(
			params_after_lfo.resonance,
			preset.voice[0].filter_enabled && preset.filter_mode != FilterMode::OFF && preset.voice[0].shape() != 0,
			preset.voice[1].filter_enabled && preset.filter_mode != FilterMode::OFF && preset.voice[1].shape() != 0,
			preset.voice[2].filter_enabled && preset.filter_mode != FilterMode::OFF && preset.voice[2].shape() != 0,
			preset.filter_mode != FilterMode::OFF
		);
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
	//trace(41, 111);
	// load the first preset after all the butts and pots have been scanned
	if (!loadedAfterStartup) {
		if (millis() > 1400) {
			//trace(53, 111);
			load(preset);
			//trace(53, 222);
			loadedAfterStartup = true;
			ledNumber(preset);
		}
	}

	ui_loop();
	//trace(41, 222);

	midiRead();
	//trace(41, 333);
	if (arpCounter >= arpSpeed + 100) {
		arpCounter = 0;
		arpTick();
	}
	//trace(41, 444);

	if (gate && !preset_data.paraphonic) {
		mux(15);
		//key = map(analogRead(A2), 0, 1023, 12, 72); // FIXME FIXME FIXME reenable CV input
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
	//trace(41, 555);

	readMux();
	//trace(41, 666);

	voice_state.set_n_individual_voices(preset_data.paraphonic ? 3 : 1);
	//trace(41, 777);
	ParamsAfterLfo params_after_lfo = lfoTick();
	//trace(41, 888);
	calculatePitch();
	//trace(41, 999);
	setSidRegisters(preset_data, params_after_lfo);
	//trace(41, 909);
}
