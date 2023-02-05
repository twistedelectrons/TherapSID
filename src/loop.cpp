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

void setSidRegisters(Preset const& preset, ParamsAfterLfo const& params_after_lfo) {
	int pitches[] = { pitch1, pitch2, pitch3 };

	for (int i=0; i<2; i++) {
		for (int v=0; v<3; v++) {
			sid_chips[i].set_attack_decay(v, preset.voice[v].attack, preset.voice[v].decay);
			sid_chips[i].set_sustain_release(v, preset.voice[v].sustain, preset.voice[v].release);
			sid_chips[i].set_pulsewidth(v, params_after_lfo.pulsewidth[v]);

			sid_chips[i].set_freq(v, i==0 ? pitches[v] : preset.fatten_pitch(pitches[v]));

			sid_chips[i].set_reg_control(v, preset.voice[v].reg_control); // FIXME gate
		}

		sid_chips[i].set_resonance_and_filter_enable(params_after_lfo.resonance, preset.voice[0].filter_enabled, preset.voice[1].filter_enabled, preset.voice[2].filter_enabled, false /* TODO external filter? */);
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
	}
}

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
		digit(1, (int)preset_data.fat_mode + 1);
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
		preset_data.paraphonic = !preset_data.paraphonic;
	}

	if (dotTimer) {
		dotTimer--;
		if (!dotTimer) {
			mydisplay.setLed(0, 7, 6, 0);
			mydisplay.setLed(0, 7, 7, 0);
		}
	}

	if (gate && !preset_data.paraphonic) {
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

	readMux();

	ParamsAfterLfo params_after_lfo = lfoTick();
	calculatePitch();
	setSidRegisters(preset_data, params_after_lfo);
	// FIXME FIXME FIXME update sid values here!

	if (showPresetNumber) {
		ledNumber(preset);
		showPresetNumber = false;
	}
}
