#include <EEPROM.h>
#include <TimerOne.h>

#include "isr.h"
#include "preset.h"
#include "midi.h"
#include "util.hpp"
#include "globals.h"

static FilterMode uint2FilterMode(uint8_t i) {
	if (i < 5) {
		return static_cast<FilterMode>(i);
	} else {
		return FilterMode::OFF;
	}
}

static FatMode uint2FatMode(uint8_t i) {
	if (i < 6) {
		return static_cast<FatMode>(i);
	} else {
		return FatMode::UNISONO;
	}
}

uint16_t Preset::fatten_pitch(uint16_t pitch) const {
	switch (fat_mode) {
		case FatMode::UNISONO:
			return pitch;
		case FatMode::OCTAVE_UP:
			if (pitch < 0xffff / 2) {
				return pitch * 2;
			} else {
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

bool read_bit4000(int bit_index) { // read any bit from the upper end of the EEPROM (4000+)
	const int byte_index = (bit_index / 8) + 4000;
	const int bit_offset = bit_index % 8;
	return (EEPROM.read(byte_index) >> bit_offset) & 1;
}

void write_bit4000(int bit_index, bool bit_value) { // write any bit in the upper end of the EEPROM (4000+)
	const int byte_index = (bit_index / 8) + 4000;
	const int bit_offset = bit_index % 8;
	byte byte_value = EEPROM.read(byte_index);
	if (bit_value) {
		byte_value |= (1 << bit_offset);
	} else {
		byte_value &= ~(1 << bit_offset);
	}
	EEPROM.write(byte_index, byte_value);
}

void save() {

	for (int i = 0; i < 3; i++) {
		write_bit4000(((preset - 1) * 3) + i, preset_data.voice[i].filter_enabled);
	}

	byte temp;
	writeIndex = EEPROM_ADDR_PRESET(preset);

	bitWrite(preset_data.voice[0].reg_control, 0, 0);
	bitWrite(preset_data.voice[1].reg_control, 0, 0);
	bitWrite(preset_data.voice[2].reg_control, 0, 0);

	writey(preset_data.voice[0].reg_control);
	writey(preset_data.voice[1].reg_control);
	writey(preset_data.voice[2].reg_control);
	temp = 0;
	temp |= ((int)preset_data.fat_mode) & 0x7;

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
	bitWrite(temp, 0, preset_data.lfo[0].mapping[0]);
	bitWrite(temp, 1, preset_data.lfo[0].mapping[1]);
	bitWrite(temp, 2, preset_data.lfo[0].mapping[2]);
	bitWrite(temp, 3, preset_data.lfo[0].mapping[3]);
	bitWrite(temp, 4, preset_data.lfo[0].mapping[4]);
	bitWrite(temp, 5, preset_data.lfo[0].mapping[5]);
	bitWrite(temp, 6, preset_data.lfo[0].mapping[6]);
	bitWrite(temp, 7, preset_data.lfo[0].mapping[7]);
	writey(temp);
	bitWrite(temp, 0, preset_data.lfo[0].mapping[8]);
	bitWrite(temp, 1, preset_data.lfo[0].mapping[9]);
	bitWrite(temp, 2, preset_data.lfo[0].mapping[10]);
	bitWrite(temp, 3, preset_data.lfo[0].mapping[11]);
	bitWrite(temp, 4, preset_data.lfo[0].mapping[12]);
	bitWrite(temp, 5, preset_data.lfo[0].mapping[13]);
	bitWrite(temp, 6, preset_data.lfo[0].mapping[14]);
	bitWrite(temp, 7, preset_data.lfo[0].mapping[15]);
	writey(temp);
	bitWrite(temp, 0, preset_data.lfo[0].mapping[16]);
	bitWrite(temp, 1, preset_data.lfo[0].mapping[17]);
	bitWrite(temp, 2, preset_data.lfo[0].mapping[18]);
	bitWrite(temp, 3, preset_data.lfo[0].mapping[19]);
	bitWrite(temp, 4, preset_data.lfo[1].mapping[0]);
	bitWrite(temp, 5, preset_data.lfo[1].mapping[1]);
	bitWrite(temp, 6, preset_data.lfo[1].mapping[2]);
	bitWrite(temp, 7, preset_data.lfo[1].mapping[3]);
	writey(temp); // 30
	bitWrite(temp, 0, preset_data.lfo[1].mapping[4]);
	bitWrite(temp, 1, preset_data.lfo[1].mapping[5]);
	bitWrite(temp, 2, preset_data.lfo[1].mapping[6]);
	bitWrite(temp, 3, preset_data.lfo[1].mapping[7]);
	bitWrite(temp, 4, preset_data.lfo[1].mapping[8]);
	bitWrite(temp, 5, preset_data.lfo[1].mapping[9]);
	bitWrite(temp, 6, preset_data.lfo[1].mapping[10]);
	bitWrite(temp, 7, preset_data.lfo[1].mapping[11]);
	writey(temp); //
	bitWrite(temp, 0, preset_data.lfo[1].mapping[12]);
	bitWrite(temp, 1, preset_data.lfo[1].mapping[13]);
	bitWrite(temp, 2, preset_data.lfo[1].mapping[14]);
	bitWrite(temp, 3, preset_data.lfo[1].mapping[15]);
	bitWrite(temp, 4, preset_data.lfo[1].mapping[16]);
	bitWrite(temp, 5, preset_data.lfo[1].mapping[17]);
	bitWrite(temp, 6, preset_data.lfo[1].mapping[18]);
	bitWrite(temp, 7, preset_data.lfo[1].mapping[19]);
	writey(temp); //
	bitWrite(temp, 0, preset_data.lfo[2].mapping[0]);
	bitWrite(temp, 1, preset_data.lfo[2].mapping[1]);
	bitWrite(temp, 2, preset_data.lfo[2].mapping[2]);
	bitWrite(temp, 3, preset_data.lfo[2].mapping[3]);
	bitWrite(temp, 4, preset_data.lfo[2].mapping[4]);
	bitWrite(temp, 5, preset_data.lfo[2].mapping[5]);
	bitWrite(temp, 6, preset_data.lfo[2].mapping[6]);
	bitWrite(temp, 7, preset_data.lfo[2].mapping[7]);
	writey(temp); //
	bitWrite(temp, 0, preset_data.lfo[2].mapping[8]);
	bitWrite(temp, 1, preset_data.lfo[2].mapping[9]);
	bitWrite(temp, 2, preset_data.lfo[2].mapping[10]);
	bitWrite(temp, 3, preset_data.lfo[2].mapping[11]);
	bitWrite(temp, 4, preset_data.lfo[2].mapping[12]);
	bitWrite(temp, 5, preset_data.lfo[2].mapping[13]);
	bitWrite(temp, 6, preset_data.lfo[2].mapping[14]);
	bitWrite(temp, 7, preset_data.lfo[2].mapping[15]);
	writey(temp); //
	bitWrite(temp, 0, preset_data.lfo[2].mapping[16]);
	bitWrite(temp, 1, preset_data.lfo[2].mapping[17]);
	bitWrite(temp, 2, preset_data.lfo[2].mapping[18]);
	bitWrite(temp, 3, preset_data.lfo[2].mapping[19]);
	bitWrite(temp, 4, bitRead(preset_data.lfo[0].shape, 0));
	bitWrite(temp, 5, bitRead(preset_data.lfo[0].shape, 1));
	bitWrite(temp, 6, bitRead(preset_data.lfo[0].shape, 2));
	bitWrite(temp, 7, bitRead(preset_data.lfo[1].shape, 0));
	writey(temp); // 35
	bitWrite(temp, 0, bitRead(preset_data.lfo[1].shape, 1));
	bitWrite(temp, 1, bitRead(preset_data.lfo[1].shape, 2));
	bitWrite(temp, 2, bitRead(preset_data.lfo[2].shape, 0));
	bitWrite(temp, 3, bitRead(preset_data.lfo[2].shape, 1));
	bitWrite(temp, 4, bitRead(preset_data.lfo[2].shape, 2));
	bitWrite(temp, 5, preset_data.lfo[0].retrig);
	bitWrite(temp, 6, preset_data.lfo[1].retrig);
	bitWrite(temp, 7, preset_data.lfo[2].retrig);
	writey(temp); // 36
	bitWrite(temp, 0, preset_data.lfo[0].looping);
	bitWrite(temp, 1, preset_data.lfo[1].looping);
	bitWrite(temp, 2, preset_data.lfo[2].looping);
	bitWrite(temp, 3, bitRead((int)preset_data.filter_mode, 0));
	bitWrite(temp, 4, bitRead((int)preset_data.filter_mode, 1));
	bitWrite(temp, 5, bitRead((int)preset_data.filter_mode, 2));
	bitWrite(temp, 6, bitRead(preset_data.arp_range_base, 0));
	bitWrite(temp, 7, bitRead(preset_data.arp_range_base, 1));
	writey(temp); // 37
	bitWrite(temp, 0, bitRead(preset_data.resonance_base, 0));
	bitWrite(temp, 1, bitRead(preset_data.resonance_base, 1));
	bitWrite(temp, 2, bitRead(preset_data.resonance_base, 2));
	bitWrite(temp, 3, bitRead(preset_data.resonance_base, 3));
	bitWrite(temp, 4, bitRead(preset_data.arp_mode, 0));
	bitWrite(temp, 5, bitRead(preset_data.arp_mode, 1));
	bitWrite(temp, 6, bitRead(preset_data.arp_mode, 2));
	bitWrite(temp, 7, bitRead(preset_data.arp_mode, 3));
	writey(temp); //
	writey(preset_data.cutoff >> 2);

	writey(preset_data.arp_speed_base); // 40
}

void load(byte number) {

	for (int i = 0; i < 3; i++) {
		preset_data.voice[i].filter_enabled = read_bit4000(((preset - 1) * 3) + i);
	}

	lfo[0] = lfo[1] = lfo[2] = 0;

	arpCounter = 0;
	Serial1.end();
	Timer1.stop(); //

	writeIndex = EEPROM_ADDR_PRESET(number);
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
	preset_data.fat_mode = uint2FatMode(temp & 0x7);

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

	temp = ready();
	preset_data.lfo[0].mapping[0] = bitRead(temp, 0);
	preset_data.lfo[0].mapping[1] = bitRead(temp, 1);
	preset_data.lfo[0].mapping[2] = bitRead(temp, 2);
	preset_data.lfo[0].mapping[3] = bitRead(temp, 3);
	preset_data.lfo[0].mapping[4] = bitRead(temp, 4);
	preset_data.lfo[0].mapping[5] = bitRead(temp, 5);
	preset_data.lfo[0].mapping[6] = bitRead(temp, 6);
	preset_data.lfo[0].mapping[7] = bitRead(temp, 7);

	temp = ready();
	preset_data.lfo[0].mapping[8] = bitRead(temp, 0);
	preset_data.lfo[0].mapping[9] = bitRead(temp, 1);
	preset_data.lfo[0].mapping[10] = bitRead(temp, 2);
	preset_data.lfo[0].mapping[11] = bitRead(temp, 3);
	preset_data.lfo[0].mapping[12] = bitRead(temp, 4);
	preset_data.lfo[0].mapping[13] = bitRead(temp, 5);
	preset_data.lfo[0].mapping[14] = bitRead(temp, 6);
	preset_data.lfo[0].mapping[15] = bitRead(temp, 7);

	temp = ready();
	preset_data.lfo[0].mapping[16] = bitRead(temp, 0);
	preset_data.lfo[0].mapping[17] = bitRead(temp, 1);
	preset_data.lfo[0].mapping[18] = bitRead(temp, 2);
	preset_data.lfo[0].mapping[19] = bitRead(temp, 3);
	preset_data.lfo[1].mapping[0] = bitRead(temp, 4);
	preset_data.lfo[1].mapping[1] = bitRead(temp, 5);
	preset_data.lfo[1].mapping[2] = bitRead(temp, 6);
	preset_data.lfo[1].mapping[3] = bitRead(temp, 7);

	temp = ready();
	preset_data.lfo[1].mapping[4] = bitRead(temp, 0);
	preset_data.lfo[1].mapping[5] = bitRead(temp, 1);
	preset_data.lfo[1].mapping[6] = bitRead(temp, 2);
	preset_data.lfo[1].mapping[7] = bitRead(temp, 3);
	preset_data.lfo[1].mapping[8] = bitRead(temp, 4);
	preset_data.lfo[1].mapping[9] = bitRead(temp, 5);
	preset_data.lfo[1].mapping[10] = bitRead(temp, 6);
	preset_data.lfo[1].mapping[11] = bitRead(temp, 7);

	temp = ready();
	preset_data.lfo[1].mapping[12] = bitRead(temp, 0);
	preset_data.lfo[1].mapping[13] = bitRead(temp, 1);
	preset_data.lfo[1].mapping[14] = bitRead(temp, 2);
	preset_data.lfo[1].mapping[15] = bitRead(temp, 3);
	preset_data.lfo[1].mapping[16] = bitRead(temp, 4);
	preset_data.lfo[1].mapping[17] = bitRead(temp, 5);
	preset_data.lfo[1].mapping[18] = bitRead(temp, 6);
	preset_data.lfo[1].mapping[19] = bitRead(temp, 7);

	temp = ready();
	preset_data.lfo[2].mapping[0] = bitRead(temp, 0);
	preset_data.lfo[2].mapping[1] = bitRead(temp, 1);
	preset_data.lfo[2].mapping[2] = bitRead(temp, 2);
	preset_data.lfo[2].mapping[3] = bitRead(temp, 3);
	preset_data.lfo[2].mapping[4] = bitRead(temp, 4);
	preset_data.lfo[2].mapping[5] = bitRead(temp, 5);
	preset_data.lfo[2].mapping[6] = bitRead(temp, 6);
	preset_data.lfo[2].mapping[7] = bitRead(temp, 7);

	temp = ready();
	preset_data.lfo[2].mapping[8] = bitRead(temp, 0);
	preset_data.lfo[2].mapping[9] = bitRead(temp, 1);
	preset_data.lfo[2].mapping[10] = bitRead(temp, 2);
	preset_data.lfo[2].mapping[11] = bitRead(temp, 3);
	preset_data.lfo[2].mapping[12] = bitRead(temp, 4);
	preset_data.lfo[2].mapping[13] = bitRead(temp, 5);
	preset_data.lfo[2].mapping[14] = bitRead(temp, 6);
	preset_data.lfo[2].mapping[15] = bitRead(temp, 7);

	temp = ready();
	preset_data.lfo[2].mapping[16] = bitRead(temp, 0);
	preset_data.lfo[2].mapping[17] = bitRead(temp, 1);
	preset_data.lfo[2].mapping[18] = bitRead(temp, 2);
	preset_data.lfo[2].mapping[19] = bitRead(temp, 3);
	bitWrite(preset_data.lfo[0].shape, 0, bitRead(temp, 4));
	bitWrite(preset_data.lfo[0].shape, 1, bitRead(temp, 5));
	bitWrite(preset_data.lfo[0].shape, 2, bitRead(temp, 6));
	bitWrite(preset_data.lfo[1].shape, 0, bitRead(temp, 7));

	if (jumble) {
		for (int i = 0; i < 20; i++) {

			preset_data.lfo[0].mapping[i] = 0;
			preset_data.lfo[1].mapping[i] = 0;
			preset_data.lfo[2].mapping[i] = 0;
		}

		for (int i = 0; i < 20; i++) {
			if (random(20) > 15) {
				preset_data.lfo[0].mapping[random(19)] = 1;
			}
			if (random(20) > 15) {
				preset_data.lfo[1].mapping[random(19)] = 1;
			}
			if (random(20) > 15) {
				preset_data.lfo[2].mapping[random(19)] = 1;
			}
		}
	}
	temp = ready();
	bitWrite(preset_data.lfo[1].shape, 1, bitRead(temp, 0));
	bitWrite(preset_data.lfo[1].shape, 2, bitRead(temp, 1));
	bitWrite(preset_data.lfo[2].shape, 0, bitRead(temp, 2));
	bitWrite(preset_data.lfo[2].shape, 1, bitRead(temp, 3));
	bitWrite(preset_data.lfo[2].shape, 2, bitRead(temp, 4));
	preset_data.lfo[0].retrig = bitRead(temp, 5);
	preset_data.lfo[1].retrig = bitRead(temp, 6);
	preset_data.lfo[2].retrig = bitRead(temp, 7);

	temp = ready();
	preset_data.lfo[0].looping = bitRead(temp, 0);
	preset_data.lfo[1].looping = bitRead(temp, 1);
	preset_data.lfo[2].looping = bitRead(temp, 2);

	preset_data.filter_mode = uint2FilterMode((temp >> 3) & 0x7);
	bitWrite(preset_data.arp_range_base, 0, bitRead(temp, 6));
	bitWrite(preset_data.arp_range_base, 1, bitRead(temp, 7));

	temp = ready();
	preset_data.resonance_base = temp & 0x0F;
	preset_data.arp_mode = (temp >> 4) & 0x0F;

	arpRound = 0;
	arpCount = 0;
	preset_data.cutoff = ready() << 2;
	preset_data.arp_speed_base = ready();
	if (preset_data.arp_speed_base == 0)
		preset_data.arp_speed_base = 1;

	bitWrite(preset_data.voice[0].reg_control, 0, 0);
	bitWrite(preset_data.voice[1].reg_control, 0, 0);
	bitWrite(preset_data.voice[2].reg_control, 0, 0);

	if (jumble)
		preset_data.arp_mode = 0;

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
	sendCC(36, preset_data.arp_range_base << 8);
	sendCC(35, preset_data.arp_speed_base << 2);

	sendCC(59, preset_data.cutoff);
	sendCC(33, preset_data.resonance_base << 6);
}

void saveChannels() {

	EEPROM.update(EEPROM_ADDR_MIDI_IN_CH_MASTER, masterChannel);
	EEPROM.update(EEPROM_ADDR_MIDI_OUT_CH_MASTER, masterChannelOut);
}
