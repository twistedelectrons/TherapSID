#include <EEPROM.h>
#include <TimerOne.h>

#include "globals.h"
#include "leds.h"
#include "sid.h" // FIXME
#include "isr.h"
#include "preset.h"
#include "lfo.h"
#include "midi.h"
#include "util.hpp"

// FIXME deduplicate

/*

MEMORY MAPPING
(0-3960) 99 PRESETS = 40 bytes

3999 PRESET LAST

3998 MIDI IN MASTER CHANNEL
3997 MIDI OUT MASTER CHANNEL

3996 SEND LFO
3995 SEND ARP


*/


uint16_t Preset::fatten_pitch(uint16_t pitch) const {
	switch (fat_mode) {
		case FatMode::UNISONO:
			return pitch;
		case FatMode::OCTAVE_UP:
			if (pitch < 0xffff / 2) {
				return pitch * 2;
			}
			else {
				return pitch;
			}
		case FatMode::DETUNE_SLIGHT:
			return pitch - 15;
		case FatMode::DETUNE_MUCH:
			return pitch - 50;
		default:
			return pitch;
	}
}

void Preset::set_leds(int lastPot, int selectedLfo, bool show_filter_assign) {
	// assert(lastPot < TODO); // FIXME
	assert(selectedLfo < 3);

	if (!show_filter_assign) {
		for (int i=0; i<3; i++) {
			ledSet(4*i+1, voice[paraphonic ? 0 : i].reg_control & PresetVoice::PULSE);
			ledSet(4*i+2, voice[paraphonic ? 0 : i].reg_control & PresetVoice::TRI);
			ledSet(4*i+3, voice[paraphonic ? 0 : i].reg_control & PresetVoice::SAW);
			ledSet(4*i+4, voice[paraphonic ? 0 : i].reg_control & PresetVoice::NOISE);
		}
	}
	else {
		for (int i=0; i<3; i++)
			for (int k=0; k<4; k++) 
				ledSet(4*i+1+k, voice[paraphonic ? 0 : i].filter_enabled);
	}

	// FIXME this should probably depend on paraphonic
	ledSet(16, bitRead(voice[0].reg_control, 1));
	ledSet(17, bitRead(voice[0].reg_control, 2));
	ledSet(18, bitRead(voice[1].reg_control, 1));
	ledSet(19, bitRead(voice[1].reg_control, 2));
	ledSet(20, bitRead(voice[2].reg_control, 1));
	ledSet(21, bitRead(voice[2].reg_control, 2));

	switch(filter_mode) {
		case FilterMode::LOWPASS:
			ledSet(27, true);
			ledSet(28, false);
			ledSet(29, false);
			break;
		case FilterMode::BANDPASS:
			ledSet(27, false);
			ledSet(28, true);
			ledSet(29, false);
			break;
		case FilterMode::HIGHPASS:
			ledSet(27, false);
			ledSet(28, false);
			ledSet(29, true);
			break;
		case FilterMode::NOTCH:
			ledSet(27, true);
			ledSet(28, false);
			ledSet(29, true);
			break;
		case FilterMode::OFF:
			ledSet(27, false);
			ledSet(28, false);
			ledSet(29, false);
			break;
	}


	if (lastPot != 20) {
		// lastPot != none
		ledSet(13, preset_data.lfo_map[0][lastPot]);
		ledSet(14, preset_data.lfo_map[1][lastPot]);
		ledSet(15, preset_data.lfo_map[2][lastPot]);
	}
	else {
		// lastPot == none
		ledSet(13, 0);
		ledSet(14, 0);
		ledSet(15, 0);
	}


	for (int shape = 1; shape<=5; shape++) {
		ledSet(21 + shape, lfoShape[selectedLfo] == shape);
	}
	ledSet(30, retrig[selectedLfo]);
	ledSet(31, looping[selectedLfo]);
}

static int writeIndex;

static void writey(byte data) {

	EEPROM.update(writeIndex, data);
	writeIndex++;
}

static int ready() {

	byte data = EEPROM.read(writeIndex);
	if (jumble) {
		data = random(200);
	}
	writeIndex++;
	return ((data));
}

void save() {
	byte temp;
	writeIndex = preset * 40;

	bitWrite(preset_data.voice[0].reg_control, 0, 0);
	bitWrite(preset_data.voice[1].reg_control, 0, 0);
	bitWrite(preset_data.voice[2].reg_control, 0, 0);

	writey(preset_data.voice[0].reg_control);
	writey(preset_data.voice[1].reg_control);
	writey(preset_data.voice[2].reg_control);
	temp = 0;
	temp |= ((int)preset_data.fat_mode) & 0x3;

	writey(temp);
	writey(preset_data.voice[0].fine_base * 255);
	writey(preset_data.voice[1].fine_base * 255);
	writey(preset_data.voice[2].fine_base * 255);
	bitWrite(temp, 0, bitRead(preset_data.voice[0].tune_base, 0));
	bitWrite(temp, 1, bitRead(preset_data.voice[0].tune_base, 1));
	bitWrite(temp, 2, bitRead(preset_data.voice[0].tune_base, 2));
	bitWrite(temp, 3, bitRead(preset_data.voice[0].tune_base, 3));
	bitWrite(temp, 4, bitRead(preset_data.voice[0].tune_base, 4));
	bitWrite(temp, 5, bitRead(preset_data.voice[1].tune_base, 0));
	bitWrite(temp, 6, bitRead(preset_data.voice[1].tune_base, 1));
	bitWrite(temp, 7, bitRead(preset_data.voice[1].tune_base, 2));
	writey(temp);
	bitWrite(temp, 0, bitRead(preset_data.voice[1].tune_base, 3));
	bitWrite(temp, 1, bitRead(preset_data.voice[1].tune_base, 4));
	bitWrite(temp, 2, bitRead(preset_data.voice[2].tune_base, 0));
	bitWrite(temp, 3, bitRead(preset_data.voice[2].tune_base, 1));
	bitWrite(temp, 4, bitRead(preset_data.voice[2].tune_base, 2));
	bitWrite(temp, 5, bitRead(preset_data.voice[2].tune_base, 3));
	bitWrite(temp, 6, bitRead(preset_data.voice[2].tune_base, 4));
	// bitWrite(temp,7,preset_data.paraphonic);
	writey(temp);
	writey(preset_data.voice[0].pulsewidth_base >> 3); // 10
	writey(preset_data.voice[1].pulsewidth_base >> 3);
	writey(preset_data.voice[2].pulsewidth_base >> 3);
	writey(preset_data.voice[0].glide);
	writey(preset_data.voice[1].glide);
	writey(preset_data.voice[2].glide);
	writey((preset_data.voice[0].attack << 4) | preset_data.voice[0].decay);
	writey((preset_data.voice[0].sustain << 4) | preset_data.voice[0].release);
	writey((preset_data.voice[1].attack << 4) | preset_data.voice[1].decay);
	writey((preset_data.voice[1].sustain << 4) | preset_data.voice[1].release);
	writey((preset_data.voice[2].attack << 4) | preset_data.voice[2].decay); // 20
	writey((preset_data.voice[2].sustain << 4) | preset_data.voice[2].release);

	writey(preset_data.lfo[0].speed / 1.3 / 4);
	writey(preset_data.lfo[1].speed / 1.3 / 4);
	writey(preset_data.lfo[2].speed / 1.3 / 4);

	writey(preset_data.lfo[0].depth >> 2);
	writey(preset_data.lfo[1].depth >> 2);
	writey(preset_data.lfo[2].depth >> 2); // 27
	// FIXME loop
	bitWrite(temp, 0, preset_data.lfo_map[0][0]);
	bitWrite(temp, 1, preset_data.lfo_map[0][1]);
	bitWrite(temp, 2, preset_data.lfo_map[0][2]);
	bitWrite(temp, 3, preset_data.lfo_map[0][3]);
	bitWrite(temp, 4, preset_data.lfo_map[0][4]);
	bitWrite(temp, 5, preset_data.lfo_map[0][5]);
	bitWrite(temp, 6, preset_data.lfo_map[0][6]);
	bitWrite(temp, 7, preset_data.lfo_map[0][7]);
	writey(temp);
	bitWrite(temp, 0, preset_data.lfo_map[0][8]);
	bitWrite(temp, 1, preset_data.lfo_map[0][9]);
	bitWrite(temp, 2, preset_data.lfo_map[0][10]);
	bitWrite(temp, 3, preset_data.lfo_map[0][11]);
	bitWrite(temp, 4, preset_data.lfo_map[0][12]);
	bitWrite(temp, 5, preset_data.lfo_map[0][13]);
	bitWrite(temp, 6, preset_data.lfo_map[0][14]);
	bitWrite(temp, 7, preset_data.lfo_map[0][15]);
	writey(temp);
	bitWrite(temp, 0, preset_data.lfo_map[0][16]);
	bitWrite(temp, 1, preset_data.lfo_map[0][17]);
	bitWrite(temp, 2, preset_data.lfo_map[0][18]);
	bitWrite(temp, 3, preset_data.lfo_map[0][19]);
	bitWrite(temp, 4, preset_data.lfo_map[1][0]);
	bitWrite(temp, 5, preset_data.lfo_map[1][1]);
	bitWrite(temp, 6, preset_data.lfo_map[1][2]);
	bitWrite(temp, 7, preset_data.lfo_map[1][3]);
	writey(temp); // 30
	bitWrite(temp, 0, preset_data.lfo_map[1][4]);
	bitWrite(temp, 1, preset_data.lfo_map[1][5]);
	bitWrite(temp, 2, preset_data.lfo_map[1][6]);
	bitWrite(temp, 3, preset_data.lfo_map[1][7]);
	bitWrite(temp, 4, preset_data.lfo_map[1][8]);
	bitWrite(temp, 5, preset_data.lfo_map[1][9]);
	bitWrite(temp, 6, preset_data.lfo_map[1][10]);
	bitWrite(temp, 7, preset_data.lfo_map[1][11]);
	writey(temp); //
	bitWrite(temp, 0, preset_data.lfo_map[1][12]);
	bitWrite(temp, 1, preset_data.lfo_map[1][13]);
	bitWrite(temp, 2, preset_data.lfo_map[1][14]);
	bitWrite(temp, 3, preset_data.lfo_map[1][15]);
	bitWrite(temp, 4, preset_data.lfo_map[1][16]);
	bitWrite(temp, 5, preset_data.lfo_map[1][17]);
	bitWrite(temp, 6, preset_data.lfo_map[1][18]);
	bitWrite(temp, 7, preset_data.lfo_map[1][19]);
	writey(temp); //
	bitWrite(temp, 0, preset_data.lfo_map[2][0]);
	bitWrite(temp, 1, preset_data.lfo_map[2][1]);
	bitWrite(temp, 2, preset_data.lfo_map[2][2]);
	bitWrite(temp, 3, preset_data.lfo_map[2][3]);
	bitWrite(temp, 4, preset_data.lfo_map[2][4]);
	bitWrite(temp, 5, preset_data.lfo_map[2][5]);
	bitWrite(temp, 6, preset_data.lfo_map[2][6]);
	bitWrite(temp, 7, preset_data.lfo_map[2][7]);
	writey(temp); //
	bitWrite(temp, 0, preset_data.lfo_map[2][8]);
	bitWrite(temp, 1, preset_data.lfo_map[2][9]);
	bitWrite(temp, 2, preset_data.lfo_map[2][10]);
	bitWrite(temp, 3, preset_data.lfo_map[2][11]);
	bitWrite(temp, 4, preset_data.lfo_map[2][12]);
	bitWrite(temp, 5, preset_data.lfo_map[2][13]);
	bitWrite(temp, 6, preset_data.lfo_map[2][14]);
	bitWrite(temp, 7, preset_data.lfo_map[2][15]);
	writey(temp); //
	bitWrite(temp, 0, preset_data.lfo_map[2][16]);
	bitWrite(temp, 1, preset_data.lfo_map[2][17]);
	bitWrite(temp, 2, preset_data.lfo_map[2][18]);
	bitWrite(temp, 3, preset_data.lfo_map[2][19]);
	bitWrite(temp, 4, bitRead(lfoShape[0], 0));
	bitWrite(temp, 5, bitRead(lfoShape[0], 1));
	bitWrite(temp, 6, bitRead(lfoShape[0], 2));
	bitWrite(temp, 7, bitRead(lfoShape[1], 0));
	writey(temp); // 35
	bitWrite(temp, 0, bitRead(lfoShape[1], 1));
	bitWrite(temp, 1, bitRead(lfoShape[1], 2));
	bitWrite(temp, 2, bitRead(lfoShape[2], 0));
	bitWrite(temp, 3, bitRead(lfoShape[2], 1));
	bitWrite(temp, 4, bitRead(lfoShape[2], 2));
	bitWrite(temp, 5, retrig[0]);
	bitWrite(temp, 6, retrig[1]);
	bitWrite(temp, 7, retrig[2]);
	writey(temp); // 36
	bitWrite(temp, 0, looping[0]);
	bitWrite(temp, 1, looping[1]);
	bitWrite(temp, 2, looping[2]);
	bitWrite(temp, 3, bitRead((int)preset_data.filter_mode, 0));
	bitWrite(temp, 4, bitRead((int)preset_data.filter_mode, 1));
	bitWrite(temp, 5, bitRead((int)preset_data.filter_mode, 2));
	bitWrite(temp, 6, bitRead(arpRangeBase, 0));
	bitWrite(temp, 7, bitRead(arpRangeBase, 1));
	writey(temp); // 37
	bitWrite(temp, 0, bitRead(preset_data.resonance_base, 0));
	bitWrite(temp, 1, bitRead(preset_data.resonance_base, 1));
	bitWrite(temp, 2, bitRead(preset_data.resonance_base, 2));
	bitWrite(temp, 3, bitRead(preset_data.resonance_base, 3));
	bitWrite(temp, 4, bitRead(arpMode, 0));
	bitWrite(temp, 5, bitRead(arpMode, 1));
	bitWrite(temp, 6, bitRead(arpMode, 2));
	bitWrite(temp, 7, bitRead(arpMode, 3));
	writey(temp); //
	writey(preset_data.cutoff >> 2);

	writey(arpSpeedBase); // 40

	saveBounce = 1600;
	saveMode = false;
	ledNumber(preset);
}

void load(byte number) {
	held = 0;
	lfo[0] = lfo[1] = lfo[2] = 0;

	loadTimer = 800;

	arpCounter = 0;
	Serial1.end();
	Timer1.stop(); //

	sidReset();

	//sidSend(4, sid[0]);
	//sidSend(11, sid[0]);
	//sidSend(18, sid[0]); //FIXME

	// held=0;arpCount=0;
	writeIndex = number * 40;
	byte temp;

	preset_data.voice[0].reg_control = ready();
	preset_data.voice[1].reg_control = ready();
	preset_data.voice[2].reg_control = ready();
	if (jumble) {

		byte dice = random(0, 5);
		if (dice == 0) {
			preset_data.voice[0].reg_control = B00000000;
		} else if (dice == 1) {
			preset_data.voice[0].reg_control = B01000000;
		} else if (dice == 2) {
			preset_data.voice[0].reg_control = B00100000;
		} else if (dice == 3) {
			preset_data.voice[0].reg_control = B00010000;
		} else if (dice == 4) {
			preset_data.voice[0].reg_control = B10000000;
		}

		dice = random(0, 5);
		if (dice == 0) {
			preset_data.voice[1].reg_control = B00000000;
		} else if (dice == 1) {
			preset_data.voice[1].reg_control = B01000000;
		} else if (dice == 2) {
			preset_data.voice[1].reg_control = B00100000;
		} else if (dice == 3) {
			preset_data.voice[1].reg_control = B00010000;
		} else if (dice == 4) {
			preset_data.voice[1].reg_control = B10000000;
		}

		dice = random(0, 5);
		if ((!dice) && (!preset_data.voice[0].reg_control) && (!preset_data.voice[1].reg_control))
			dice++;
		if (dice == 0) {
			preset_data.voice[2].reg_control = B00000000;
		} else if (dice == 1) {
			preset_data.voice[2].reg_control = B01000000;
		} else if (dice == 2) {
			preset_data.voice[2].reg_control = B00100000;
		} else if (dice == 3) {
			preset_data.voice[2].reg_control = B00010000;
		} else if (dice == 4) {
			preset_data.voice[2].reg_control = B10000000;
		}

		dice = random(0, 5);
		if (dice == 0) {
			bitWrite(preset_data.voice[0].reg_control, 1, 1);
		}
		if (dice == 1) {
			bitWrite(preset_data.voice[0].reg_control, 2, 1);
		}
		if (dice == 2) {
			bitWrite(preset_data.voice[1].reg_control, 1, 1);
		}
		if (dice == 3) {
			bitWrite(preset_data.voice[1].reg_control, 2, 1);
		}
		if (dice == 4) {
			bitWrite(preset_data.voice[2].reg_control, 1, 1);
		}
		if (dice == 5) {
			bitWrite(preset_data.voice[2].reg_control, 2, 1);
		}
	}

	temp = ready();
	preset_data.fat_mode = uint2FatMode(temp & 0x3);

	preset_data.voice[0].fine_base = ready();
	preset_data.voice[0].fine_base /= 255;
	preset_data.voice[1].fine_base = ready();
	preset_data.voice[1].fine_base /= 255;
	preset_data.voice[2].fine_base = ready();
	preset_data.voice[2].fine_base /= 255;

	temp = ready();
	bitWrite(preset_data.voice[0].tune_base, 0, bitRead(temp, 0));
	bitWrite(preset_data.voice[0].tune_base, 1, bitRead(temp, 1));
	bitWrite(preset_data.voice[0].tune_base, 2, bitRead(temp, 2));
	bitWrite(preset_data.voice[0].tune_base, 3, bitRead(temp, 3));
	bitWrite(preset_data.voice[0].tune_base, 4, bitRead(temp, 4));
	bitWrite(preset_data.voice[1].tune_base, 0, bitRead(temp, 5));
	bitWrite(preset_data.voice[1].tune_base, 1, bitRead(temp, 6));
	bitWrite(preset_data.voice[1].tune_base, 2, bitRead(temp, 7));

	temp = ready();
	bitWrite(preset_data.voice[1].tune_base, 3, bitRead(temp, 0));
	bitWrite(preset_data.voice[1].tune_base, 4, bitRead(temp, 1));
	bitWrite(preset_data.voice[2].tune_base, 0, bitRead(temp, 2));
	bitWrite(preset_data.voice[2].tune_base, 1, bitRead(temp, 3));
	bitWrite(preset_data.voice[2].tune_base, 2, bitRead(temp, 4));
	bitWrite(preset_data.voice[2].tune_base, 3, bitRead(temp, 5));
	bitWrite(preset_data.voice[2].tune_base, 4, bitRead(temp, 6));

	shape1Pressed = false;
	shape1PressedTimer = 0;

	preset_data.voice[0].tune_base = constrain(preset_data.voice[0].tune_base, 0, 24);
	preset_data.voice[1].tune_base = constrain(preset_data.voice[1].tune_base, 0, 24);
	preset_data.voice[2].tune_base = constrain(preset_data.voice[2].tune_base, 0, 24);

	preset_data.voice[0].pulsewidth_base = ready() << 3;
	preset_data.voice[1].pulsewidth_base = ready() << 3;
	preset_data.voice[2].pulsewidth_base = ready() << 3;

	preset_data.voice[0].glide = constrain(ready(), 0, 63);
	preset_data.voice[1].glide = constrain(ready(), 0, 63);
	preset_data.voice[2].glide = constrain(ready(), 0, 63);

	if (jumble) {
		if (random(20) > 15) {
			preset_data.voice[0].glide = random(63);
		} else {
			preset_data.voice[0].glide = 0;
		}
	}
	if (jumble) {
		if (random(20) > 15) {
			preset_data.voice[1].glide = random(63);
		} else {
			preset_data.voice[1].glide = 0;
		}
	}
	if (jumble) {
		if (random(20) > 15) {
			preset_data.voice[2].glide = random(63);
		} else {
			preset_data.voice[2].glide = 0;
		}
	}

	// FIXME loop
	temp = ready();
	preset_data.voice[0].decay = temp & 0x0F;
	preset_data.voice[0].attack = (temp >> 4) & 0x0F;

	temp = ready();
	preset_data.voice[0].release = temp & 0x0F;
	preset_data.voice[0].sustain = (temp >> 4) & 0x0F;

	temp = ready();
	preset_data.voice[1].decay = temp & 0x0F;
	preset_data.voice[1].attack = (temp >> 4) & 0x0F;

	temp = ready();
	preset_data.voice[1].release = temp & 0x0F;
	preset_data.voice[1].sustain = (temp >> 4) & 0x0F;

	temp = ready();
	preset_data.voice[2].decay = temp & 0x0F;
	preset_data.voice[2].attack = (temp >> 4) & 0x0F;

	temp = ready();
	preset_data.voice[2].release = temp & 0x0F;
	preset_data.voice[2].sustain = (temp >> 4) & 0x0F;

	a4 = preset_data.voice[2].attack << 4;
	d4 = preset_data.voice[2].decay << 4;
	s4 = preset_data.voice[2].sustain << 4;
	r4 = preset_data.voice[2].release << 4;

	preset_data.lfo[0].speed = (ready() << 2) * 1.3;
	preset_data.lfo[1].speed = (ready() << 2) * 1.3;
	preset_data.lfo[2].speed = (ready() << 2) * 1.3;

	preset_data.lfo[0].depth = ready() << 2;
	preset_data.lfo[1].depth = ready() << 2;
	preset_data.lfo[2].depth = ready() << 2;

	// FIXME loop!
	temp = ready();
	preset_data.lfo_map[0][0] = bitRead(temp, 0);
	preset_data.lfo_map[0][1] = bitRead(temp, 1);
	preset_data.lfo_map[0][2] = bitRead(temp, 2);
	preset_data.lfo_map[0][3] = bitRead(temp, 3);
	preset_data.lfo_map[0][4] = bitRead(temp, 4);
	preset_data.lfo_map[0][5] = bitRead(temp, 5);
	preset_data.lfo_map[0][6] = bitRead(temp, 6);
	preset_data.lfo_map[0][7] = bitRead(temp, 7);

	temp = ready();
	preset_data.lfo_map[0][8] = bitRead(temp, 0);
	preset_data.lfo_map[0][9] = bitRead(temp, 1);
	preset_data.lfo_map[0][10] = bitRead(temp, 2);
	preset_data.lfo_map[0][11] = bitRead(temp, 3);
	preset_data.lfo_map[0][12] = bitRead(temp, 4);
	preset_data.lfo_map[0][13] = bitRead(temp, 5);
	preset_data.lfo_map[0][14] = bitRead(temp, 6);
	preset_data.lfo_map[0][15] = bitRead(temp, 7);

	temp = ready();
	preset_data.lfo_map[0][16] = bitRead(temp, 0);
	preset_data.lfo_map[0][17] = bitRead(temp, 1);
	preset_data.lfo_map[0][18] = bitRead(temp, 2);
	preset_data.lfo_map[0][19] = bitRead(temp, 3);
	preset_data.lfo_map[1][0] = bitRead(temp, 4);
	preset_data.lfo_map[1][1] = bitRead(temp, 5);
	preset_data.lfo_map[1][2] = bitRead(temp, 6);
	preset_data.lfo_map[1][3] = bitRead(temp, 7);

	temp = ready();
	preset_data.lfo_map[1][4] = bitRead(temp, 0);
	preset_data.lfo_map[1][5] = bitRead(temp, 1);
	preset_data.lfo_map[1][6] = bitRead(temp, 2);
	preset_data.lfo_map[1][7] = bitRead(temp, 3);
	preset_data.lfo_map[1][8] = bitRead(temp, 4);
	preset_data.lfo_map[1][9] = bitRead(temp, 5);
	preset_data.lfo_map[1][10] = bitRead(temp, 6);
	preset_data.lfo_map[1][11] = bitRead(temp, 7);

	temp = ready();
	preset_data.lfo_map[1][12] = bitRead(temp, 0);
	preset_data.lfo_map[1][13] = bitRead(temp, 1);
	preset_data.lfo_map[1][14] = bitRead(temp, 2);
	preset_data.lfo_map[1][15] = bitRead(temp, 3);
	preset_data.lfo_map[1][16] = bitRead(temp, 4);
	preset_data.lfo_map[1][17] = bitRead(temp, 5);
	preset_data.lfo_map[1][18] = bitRead(temp, 6);
	preset_data.lfo_map[1][19] = bitRead(temp, 7);

	temp = ready();
	preset_data.lfo_map[2][0] = bitRead(temp, 0);
	preset_data.lfo_map[2][1] = bitRead(temp, 1);
	preset_data.lfo_map[2][2] = bitRead(temp, 2);
	preset_data.lfo_map[2][3] = bitRead(temp, 3);
	preset_data.lfo_map[2][4] = bitRead(temp, 4);
	preset_data.lfo_map[2][5] = bitRead(temp, 5);
	preset_data.lfo_map[2][6] = bitRead(temp, 6);
	preset_data.lfo_map[2][7] = bitRead(temp, 7);

	temp = ready();
	preset_data.lfo_map[2][8] = bitRead(temp, 0);
	preset_data.lfo_map[2][9] = bitRead(temp, 1);
	preset_data.lfo_map[2][10] = bitRead(temp, 2);
	preset_data.lfo_map[2][11] = bitRead(temp, 3);
	preset_data.lfo_map[2][12] = bitRead(temp, 4);
	preset_data.lfo_map[2][13] = bitRead(temp, 5);
	preset_data.lfo_map[2][14] = bitRead(temp, 6);
	preset_data.lfo_map[2][15] = bitRead(temp, 7);

	temp = ready();
	preset_data.lfo_map[2][16] = bitRead(temp, 0);
	preset_data.lfo_map[2][17] = bitRead(temp, 1);
	preset_data.lfo_map[2][18] = bitRead(temp, 2);
	preset_data.lfo_map[2][19] = bitRead(temp, 3);
	bitWrite(lfoShape[0], 0, bitRead(temp, 4));
	bitWrite(lfoShape[0], 1, bitRead(temp, 5));
	bitWrite(lfoShape[0], 2, bitRead(temp, 6));
	bitWrite(lfoShape[1], 0, bitRead(temp, 7));

	if (jumble) {
		for (int i = 0; i < 20; i++) {

			preset_data.lfo_map[0][i] = 0;
			preset_data.lfo_map[1][i] = 0;
			preset_data.lfo_map[2][i] = 0;
		}

		for (int i = 0; i < 20; i++) {
			if (random(20) > 15) {
				preset_data.lfo_map[0][random(19)] = 1;
			}
			if (random(20) > 15) {
				preset_data.lfo_map[1][random(19)] = 1;
			}
			if (random(20) > 15) {
				preset_data.lfo_map[2][random(19)] = 1;
			}
		}
	}
	temp = ready();
	bitWrite(lfoShape[1], 1, bitRead(temp, 0));
	bitWrite(lfoShape[1], 2, bitRead(temp, 1));
	bitWrite(lfoShape[2], 0, bitRead(temp, 2));
	bitWrite(lfoShape[2], 1, bitRead(temp, 3));
	bitWrite(lfoShape[2], 2, bitRead(temp, 4));
	retrig[0] = bitRead(temp, 5);
	retrig[1] = bitRead(temp, 6);
	retrig[2] = bitRead(temp, 7);

	temp = ready();
	looping[0] = bitRead(temp, 0);
	looping[1] = bitRead(temp, 1);
	looping[2] = bitRead(temp, 2);

	preset_data.filter_mode = uint2FilterMode((temp >> 3) & 0x7);
	bitWrite(arpRangeBase, 0, bitRead(temp, 6));
	bitWrite(arpRangeBase, 1, bitRead(temp, 7));


	temp = ready();
	preset_data.resonance_base = temp & 0x0F;
	arpMode = (temp >> 4) & 0x0F;

	arpRound = 0;
	arpCount = 0;
	preset_data.cutoff = ready() << 2;
	arpSpeedBase = ready();
	if (arpSpeedBase == 0)
		arpSpeedBase = 1;

	lastMovedPot(lastPot);

	updateFilter();

	bitWrite(preset_data.voice[0].reg_control, 0, 0);
	bitWrite(preset_data.voice[1].reg_control, 0, 0);
	bitWrite(preset_data.voice[2].reg_control, 0, 0);

	if (jumble)
		arpMode = 0;

	preset_data.set_leds(lastPot, selectedLfo, filterModeHeld);

	Timer1.initialize(100);      //
	Timer1.attachInterrupt(isr); // attach the service routine here

	// UPDATE EDITOR

	sendCC(2, preset_data.voice[0].pulsewidth_base >> 1);
	sendCC(10, preset_data.voice[1].pulsewidth_base >> 1);
	sendCC(18, preset_data.voice[2].pulsewidth_base >> 1);

	sendCC(3, map(preset_data.voice[0].tune_base, 0, 25, 0, 1023));
	sendCC(11, map(preset_data.voice[1].tune_base, 0, 25, 0, 1023));
	sendCC(19, map(preset_data.voice[2].tune_base, 0, 25, 0, 1023));

	sendCC(4, int(preset_data.voice[0].fine_base * 1023));
	sendCC(12, int(preset_data.voice[1].fine_base * 1023));
	sendCC(20, int(preset_data.voice[2].fine_base * 1023));

	sendCC(5, preset_data.voice[0].glide << 4);
	sendCC(13, preset_data.voice[1].glide << 4);
	sendCC(21, preset_data.voice[2].glide << 4);

	sendCC(6, preset_data.voice[0].attack << 6);
	sendCC(14, preset_data.voice[1].attack << 6);
	sendCC(22, preset_data.voice[2].attack << 6);
	sendCC(7, preset_data.voice[0].decay << 6);
	sendCC(15, preset_data.voice[1].decay << 6);
	sendCC(23, preset_data.voice[2].decay << 6);
	sendCC(8, preset_data.voice[0].sustain << 6);
	sendCC(16, preset_data.voice[1].sustain << 6);
	sendCC(24, preset_data.voice[2].sustain << 6);
	sendCC(9, preset_data.voice[0].release << 6);
	sendCC(17, preset_data.voice[1].release << 6);
	sendCC(25, preset_data.voice[2].release << 6);

	sendCC(26, preset_data.lfo[0].speed / 1.3);
	sendCC(28, preset_data.lfo[1].speed / 1.3);
	sendCC(30, preset_data.lfo[2].speed / 1.3);

	sendCC(27, preset_data.lfo[0].depth);
	sendCC(29, preset_data.lfo[1].depth);
	sendCC(31, preset_data.lfo[2].depth);

	sendCC(34, arpStepBase << 2);
	sendCC(36, arpRangeBase << 8);
	sendCC(35, arpSpeedBase << 2);

	sendCC(59, preset_data.cutoff);
	sendCC(33, preset_data.resonance_base << 6);
}

void saveChannels() {

	EEPROM.update(3998, masterChannel);
	EEPROM.update(3997, masterChannelOut);
}
