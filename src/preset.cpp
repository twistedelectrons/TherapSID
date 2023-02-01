#include <EEPROM.h>
#include <TimerOne.h>

#include "globals.h"
#include "leds.h"
#include "sid.h" // FIXME
#include "isr.h"
#include "preset.h"
#include "lfo.h"
#include "midi.h"

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

static Preset my_preset; // FIXME

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

	bitWrite(my_preset.voice[0].reg_control, 0, 0);
	bitWrite(my_preset.voice[1].reg_control, 0, 0);
	bitWrite(my_preset.voice[2].reg_control, 0, 0);

	writey(my_preset.voice[0].reg_control);
	writey(my_preset.voice[1].reg_control);
	writey(my_preset.voice[2].reg_control);
	temp = 0;
	temp |= ((int)fatMode) & 0x3;

	writey(temp);
	writey(fineBase1 * 255);
	writey(fineBase2 * 255);
	writey(fineBase3 * 255);
	bitWrite(temp, 0, bitRead(tuneBase1, 0));
	bitWrite(temp, 1, bitRead(tuneBase1, 1));
	bitWrite(temp, 2, bitRead(tuneBase1, 2));
	bitWrite(temp, 3, bitRead(tuneBase1, 3));
	bitWrite(temp, 4, bitRead(tuneBase1, 4));
	bitWrite(temp, 5, bitRead(tuneBase2, 0));
	bitWrite(temp, 6, bitRead(tuneBase2, 1));
	bitWrite(temp, 7, bitRead(tuneBase2, 2));
	writey(temp);
	bitWrite(temp, 0, bitRead(tuneBase2, 3));
	bitWrite(temp, 1, bitRead(tuneBase2, 4));
	bitWrite(temp, 2, bitRead(tuneBase3, 0));
	bitWrite(temp, 3, bitRead(tuneBase3, 1));
	bitWrite(temp, 4, bitRead(tuneBase3, 2));
	bitWrite(temp, 5, bitRead(tuneBase3, 3));
	bitWrite(temp, 6, bitRead(tuneBase3, 4));
	// bitWrite(temp,7,preset_data.paraphonic);
	writey(temp);
	writey(pw1Base >> 3); // 10
	writey(pw2Base >> 3);
	writey(pw3Base >> 3);
	writey(glide1);
	writey(glide2);
	writey(glide3);
	writey((a1 << 4) | d1);
	writey((s1 << 4) | r1);
	writey((a2 << 4) | d2);
	writey((s2 << 4) | r2);
	writey((a3 << 4) | d3); // 20
	writey((s3 << 4) | r3);

	int tempy = lfoSpeedBase[0];
	tempy /= 1.3;
	tempy = tempy >> 2;
	writey(tempy);

	tempy = lfoSpeedBase[1];
	tempy /= 1.3;
	tempy = tempy >> 2;
	writey(tempy);

	tempy = lfoSpeedBase[2];
	tempy /= 1.3;
	tempy = tempy >> 2;
	writey(tempy);

	writey(lfoDepthBase[0] >> 2);
	writey(lfoDepthBase[1] >> 2);
	writey(lfoDepthBase[2] >> 2); // 27
	bitWrite(temp, 0, lfoAss[0][0]);
	bitWrite(temp, 1, lfoAss[0][1]);
	bitWrite(temp, 2, lfoAss[0][2]);
	bitWrite(temp, 3, lfoAss[0][3]);
	bitWrite(temp, 4, lfoAss[0][4]);
	bitWrite(temp, 5, lfoAss[0][5]);
	bitWrite(temp, 6, lfoAss[0][6]);
	bitWrite(temp, 7, lfoAss[0][7]);
	writey(temp);
	bitWrite(temp, 0, lfoAss[0][8]);
	bitWrite(temp, 1, lfoAss[0][9]);
	bitWrite(temp, 2, lfoAss[0][10]);
	bitWrite(temp, 3, lfoAss[0][11]);
	bitWrite(temp, 4, lfoAss[0][12]);
	bitWrite(temp, 5, lfoAss[0][13]);
	bitWrite(temp, 6, lfoAss[0][14]);
	bitWrite(temp, 7, lfoAss[0][15]);
	writey(temp);
	bitWrite(temp, 0, lfoAss[0][16]);
	bitWrite(temp, 1, lfoAss[0][17]);
	bitWrite(temp, 2, lfoAss[0][18]);
	bitWrite(temp, 3, lfoAss[0][19]);
	bitWrite(temp, 4, lfoAss[1][0]);
	bitWrite(temp, 5, lfoAss[1][1]);
	bitWrite(temp, 6, lfoAss[1][2]);
	bitWrite(temp, 7, lfoAss[1][3]);
	writey(temp); // 30
	bitWrite(temp, 0, lfoAss[1][4]);
	bitWrite(temp, 1, lfoAss[1][5]);
	bitWrite(temp, 2, lfoAss[1][6]);
	bitWrite(temp, 3, lfoAss[1][7]);
	bitWrite(temp, 4, lfoAss[1][8]);
	bitWrite(temp, 5, lfoAss[1][9]);
	bitWrite(temp, 6, lfoAss[1][10]);
	bitWrite(temp, 7, lfoAss[1][11]);
	writey(temp); //
	bitWrite(temp, 0, lfoAss[1][12]);
	bitWrite(temp, 1, lfoAss[1][13]);
	bitWrite(temp, 2, lfoAss[1][14]);
	bitWrite(temp, 3, lfoAss[1][15]);
	bitWrite(temp, 4, lfoAss[1][16]);
	bitWrite(temp, 5, lfoAss[1][17]);
	bitWrite(temp, 6, lfoAss[1][18]);
	bitWrite(temp, 7, lfoAss[1][19]);
	writey(temp); //
	bitWrite(temp, 0, lfoAss[2][0]);
	bitWrite(temp, 1, lfoAss[2][1]);
	bitWrite(temp, 2, lfoAss[2][2]);
	bitWrite(temp, 3, lfoAss[2][3]);
	bitWrite(temp, 4, lfoAss[2][4]);
	bitWrite(temp, 5, lfoAss[2][5]);
	bitWrite(temp, 6, lfoAss[2][6]);
	bitWrite(temp, 7, lfoAss[2][7]);
	writey(temp); //
	bitWrite(temp, 0, lfoAss[2][8]);
	bitWrite(temp, 1, lfoAss[2][9]);
	bitWrite(temp, 2, lfoAss[2][10]);
	bitWrite(temp, 3, lfoAss[2][11]);
	bitWrite(temp, 4, lfoAss[2][12]);
	bitWrite(temp, 5, lfoAss[2][13]);
	bitWrite(temp, 6, lfoAss[2][14]);
	bitWrite(temp, 7, lfoAss[2][15]);
	writey(temp); //
	bitWrite(temp, 0, lfoAss[2][16]);
	bitWrite(temp, 1, lfoAss[2][17]);
	bitWrite(temp, 2, lfoAss[2][18]);
	bitWrite(temp, 3, lfoAss[2][19]);
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
	bitWrite(temp, 3, bitRead((int)filterMode, 0));
	bitWrite(temp, 4, bitRead((int)filterMode, 1));
	bitWrite(temp, 5, bitRead((int)filterMode, 2));
	bitWrite(temp, 6, bitRead(arpRangeBase, 0));
	bitWrite(temp, 7, bitRead(arpRangeBase, 1));
	writey(temp); // 37
	bitWrite(temp, 0, bitRead(resBase, 0));
	bitWrite(temp, 1, bitRead(resBase, 1));
	bitWrite(temp, 2, bitRead(resBase, 2));
	bitWrite(temp, 3, bitRead(resBase, 3));
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

	my_preset.voice[0].reg_control = ready();
	my_preset.voice[1].reg_control = ready();
	my_preset.voice[2].reg_control = ready();
	if (jumble) {

		byte dice = random(0, 5);
		if (dice == 0) {
			my_preset.voice[0].reg_control = B00000000;
		} else if (dice == 1) {
			my_preset.voice[0].reg_control = B01000000;
		} else if (dice == 2) {
			my_preset.voice[0].reg_control = B00100000;
		} else if (dice == 3) {
			my_preset.voice[0].reg_control = B00010000;
		} else if (dice == 4) {
			my_preset.voice[0].reg_control = B10000000;
		}

		dice = random(0, 5);
		if (dice == 0) {
			my_preset.voice[1].reg_control = B00000000;
		} else if (dice == 1) {
			my_preset.voice[1].reg_control = B01000000;
		} else if (dice == 2) {
			my_preset.voice[1].reg_control = B00100000;
		} else if (dice == 3) {
			my_preset.voice[1].reg_control = B00010000;
		} else if (dice == 4) {
			my_preset.voice[1].reg_control = B10000000;
		}

		dice = random(0, 5);
		if ((!dice) && (!my_preset.voice[0].reg_control) && (!my_preset.voice[1].reg_control))
			dice++;
		if (dice == 0) {
			my_preset.voice[2].reg_control = B00000000;
		} else if (dice == 1) {
			my_preset.voice[2].reg_control = B01000000;
		} else if (dice == 2) {
			my_preset.voice[2].reg_control = B00100000;
		} else if (dice == 3) {
			my_preset.voice[2].reg_control = B00010000;
		} else if (dice == 4) {
			my_preset.voice[2].reg_control = B10000000;
		}

		dice = random(0, 5);
		if (dice == 0) {
			bitWrite(my_preset.voice[0].reg_control, 1, 1);
		}
		if (dice == 1) {
			bitWrite(my_preset.voice[0].reg_control, 2, 1);
		}
		if (dice == 2) {
			bitWrite(my_preset.voice[1].reg_control, 1, 1);
		}
		if (dice == 3) {
			bitWrite(my_preset.voice[1].reg_control, 2, 1);
		}
		if (dice == 4) {
			bitWrite(my_preset.voice[2].reg_control, 1, 1);
		}
		if (dice == 5) {
			bitWrite(my_preset.voice[2].reg_control, 2, 1);
		}
	}

	temp = ready();
	fatMode = uint2FatMode(temp & 0x3);
	updateFatMode();

	fineBase1 = ready();
	fineBase1 /= 255;
	fineBase2 = ready();
	fineBase2 /= 255;
	fineBase3 = ready();
	fineBase3 /= 255;

	temp = ready();
	bitWrite(tuneBase1, 0, bitRead(temp, 0));
	bitWrite(tuneBase1, 1, bitRead(temp, 1));
	bitWrite(tuneBase1, 2, bitRead(temp, 2));
	bitWrite(tuneBase1, 3, bitRead(temp, 3));
	bitWrite(tuneBase1, 4, bitRead(temp, 4));
	bitWrite(tuneBase2, 0, bitRead(temp, 5));
	bitWrite(tuneBase2, 1, bitRead(temp, 6));
	bitWrite(tuneBase2, 2, bitRead(temp, 7));

	temp = ready();
	bitWrite(tuneBase2, 3, bitRead(temp, 0));
	bitWrite(tuneBase2, 4, bitRead(temp, 1));
	bitWrite(tuneBase3, 0, bitRead(temp, 2));
	bitWrite(tuneBase3, 1, bitRead(temp, 3));
	bitWrite(tuneBase3, 2, bitRead(temp, 4));
	bitWrite(tuneBase3, 3, bitRead(temp, 5));
	bitWrite(tuneBase3, 4, bitRead(temp, 6));

	shape1Pressed = false;
	shape1PressedTimer = 0;

	tuneBase1 = constrain(tuneBase1, 0, 24);
	tuneBase2 = constrain(tuneBase2, 0, 24);
	tuneBase3 = constrain(tuneBase3, 0, 24);

	pw1Base = ready();
	pw1Base = pw1Base << 3;
	pw2Base = ready();
	pw2Base = pw2Base << 3;
	pw3Base = ready();
	pw3Base = pw3Base << 3;

	glide1 = ready();
	glide1 = constrain(glide1, 0, 63);
	glide2 = ready();
	glide2 = constrain(glide2, 0, 63);
	glide3 = ready();
	glide3 = constrain(glide3, 0, 63);

	if (jumble) {
		if (random(20) > 15) {
			glide1 = random(63);
		} else {
			glide1 = 0;
		}
	}
	if (jumble) {
		if (random(20) > 15) {
			glide2 = random(63);
		} else {
			glide2 = 0;
		}
	}
	if (jumble) {
		if (random(20) > 15) {
			glide3 = random(63);
		} else {
			glide3 = 0;
		}
	}

	// FIXME loop
	temp = ready();
	d1 = temp & 0x0F;
	a1 = (temp >> 4) & 0x0F;

	temp = ready();
	r1 = temp & 0x0F;
	s1 = (temp >> 4) & 0x0F;

	temp = ready();
	d2 = temp & 0x0F;
	a2 = (temp >> 4) & 0x0F;

	temp = ready();
	r2 = temp & 0x0F;
	s2 = (temp >> 4) & 0x0F;

	temp = ready();
	d3 = temp & 0x0F;
	a3 = (temp >> 4) & 0x0F;

	temp = ready();
	r3 = temp & 0x0F;
	s3 = (temp >> 4) & 0x0F;

	a4 = a3 << 4;
	d4 = d3 << 4;
	s4 = s3 << 4;
	r4 = r3 << 4;

	lfoSpeedBase[0] = (ready() << 2) * 1.3;
	lfoSpeedBase[1] = (ready() << 2) * 1.3;
	lfoSpeedBase[2] = (ready() << 2) * 1.3;

	lfoDepthBase[0] = ready() << 2;
	lfoDepthBase[1] = ready() << 2;
	lfoDepthBase[2] = ready() << 2;

	temp = ready();
	lfoAss[0][0] = bitRead(temp, 0);
	lfoAss[0][1] = bitRead(temp, 1);
	lfoAss[0][2] = bitRead(temp, 2);
	lfoAss[0][3] = bitRead(temp, 3);
	lfoAss[0][4] = bitRead(temp, 4);
	lfoAss[0][5] = bitRead(temp, 5);
	lfoAss[0][6] = bitRead(temp, 6);
	lfoAss[0][7] = bitRead(temp, 7);

	temp = ready();
	lfoAss[0][8] = bitRead(temp, 0);
	lfoAss[0][9] = bitRead(temp, 1);
	lfoAss[0][10] = bitRead(temp, 2);
	lfoAss[0][11] = bitRead(temp, 3);
	lfoAss[0][12] = bitRead(temp, 4);
	lfoAss[0][13] = bitRead(temp, 5);
	lfoAss[0][14] = bitRead(temp, 6);
	lfoAss[0][15] = bitRead(temp, 7);

	temp = ready();
	lfoAss[0][16] = bitRead(temp, 0);
	lfoAss[0][17] = bitRead(temp, 1);
	lfoAss[0][18] = bitRead(temp, 2);
	lfoAss[0][19] = bitRead(temp, 3);
	lfoAss[1][0] = bitRead(temp, 4);
	lfoAss[1][1] = bitRead(temp, 5);
	lfoAss[1][2] = bitRead(temp, 6);
	lfoAss[1][3] = bitRead(temp, 7);

	temp = ready();
	lfoAss[1][4] = bitRead(temp, 0);
	lfoAss[1][5] = bitRead(temp, 1);
	lfoAss[1][6] = bitRead(temp, 2);
	lfoAss[1][7] = bitRead(temp, 3);
	lfoAss[1][8] = bitRead(temp, 4);
	lfoAss[1][9] = bitRead(temp, 5);
	lfoAss[1][10] = bitRead(temp, 6);
	lfoAss[1][11] = bitRead(temp, 7);

	temp = ready();
	lfoAss[1][12] = bitRead(temp, 0);
	lfoAss[1][13] = bitRead(temp, 1);
	lfoAss[1][14] = bitRead(temp, 2);
	lfoAss[1][15] = bitRead(temp, 3);
	lfoAss[1][16] = bitRead(temp, 4);
	lfoAss[1][17] = bitRead(temp, 5);
	lfoAss[1][18] = bitRead(temp, 6);
	lfoAss[1][19] = bitRead(temp, 7);

	temp = ready();
	lfoAss[2][0] = bitRead(temp, 0);
	lfoAss[2][1] = bitRead(temp, 1);
	lfoAss[2][2] = bitRead(temp, 2);
	lfoAss[2][3] = bitRead(temp, 3);
	lfoAss[2][4] = bitRead(temp, 4);
	lfoAss[2][5] = bitRead(temp, 5);
	lfoAss[2][6] = bitRead(temp, 6);
	lfoAss[2][7] = bitRead(temp, 7);

	temp = ready();
	lfoAss[2][8] = bitRead(temp, 0);
	lfoAss[2][9] = bitRead(temp, 1);
	lfoAss[2][10] = bitRead(temp, 2);
	lfoAss[2][11] = bitRead(temp, 3);
	lfoAss[2][12] = bitRead(temp, 4);
	lfoAss[2][13] = bitRead(temp, 5);
	lfoAss[2][14] = bitRead(temp, 6);
	lfoAss[2][15] = bitRead(temp, 7);

	temp = ready();
	lfoAss[2][16] = bitRead(temp, 0);
	lfoAss[2][17] = bitRead(temp, 1);
	lfoAss[2][18] = bitRead(temp, 2);
	lfoAss[2][19] = bitRead(temp, 3);
	bitWrite(lfoShape[0], 0, bitRead(temp, 4));
	bitWrite(lfoShape[0], 1, bitRead(temp, 5));
	bitWrite(lfoShape[0], 2, bitRead(temp, 6));
	bitWrite(lfoShape[1], 0, bitRead(temp, 7));

	if (jumble) {
		for (int i = 0; i < 20; i++) {

			lfoAss[0][i] = 0;
			lfoAss[1][i] = 0;
			lfoAss[2][i] = 0;
		}

		for (int i = 0; i < 20; i++) {
			if (random(20) > 15) {
				lfoAss[0][random(19)] = 1;
			}
			if (random(20) > 15) {
				lfoAss[1][random(19)] = 1;
			}
			if (random(20) > 15) {
				lfoAss[2][random(19)] = 1;
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

	filterMode = uint2FilterMode((temp >> 3) & 0x7);
	bitWrite(arpRangeBase, 0, bitRead(temp, 6));
	bitWrite(arpRangeBase, 1, bitRead(temp, 7));


	temp = ready();
	bitWrite(resBase, 0, bitRead(temp, 0));
	bitWrite(resBase, 1, bitRead(temp, 1));
	bitWrite(resBase, 2, bitRead(temp, 2));
	bitWrite(resBase, 3, bitRead(temp, 3));
	bitWrite(arpMode, 0, bitRead(temp, 4));
	bitWrite(arpMode, 1, bitRead(temp, 5));
	bitWrite(arpMode, 2, bitRead(temp, 6));
	bitWrite(arpMode, 3, bitRead(temp, 7));

	arpRound = 0;
	arpCount = 0;
	preset_data.cutoff = ready() << 2;
	arpSpeedBase = ready();
	if (arpSpeedBase == 0)
		arpSpeedBase = 1;

	lastMovedPot(lastPot);

	updateFilter();

	bitWrite(my_preset.voice[0].reg_control, 0, 0);
	bitWrite(my_preset.voice[1].reg_control, 0, 0);
	bitWrite(my_preset.voice[2].reg_control, 0, 0);

	if (jumble)
		arpMode = 0;

	my_preset.set_leds();

	Timer1.initialize(100);      //
	Timer1.attachInterrupt(isr); // attach the service routine here

	// UPDATE EDITOR

	sendCC(2, pw1Base >> 1);
	sendCC(10, pw2Base >> 1);
	sendCC(18, pw3Base >> 1);

	sendCC(3, map(tuneBase1, 0, 25, 0, 1023));
	sendCC(11, map(tuneBase2, 0, 25, 0, 1023));
	sendCC(19, map(tuneBase3, 0, 25, 0, 1023));

	sendCC(4, int(fineBase1 * 1023));
	sendCC(12, int(fineBase2 * 1023));
	sendCC(20, int(fineBase3 * 1023));

	sendCC(5, glide1 << 4);
	sendCC(13, glide2 << 4);
	sendCC(21, glide3 << 4);

	sendCC(6, a1 << 6);
	sendCC(14, a2 << 6);
	sendCC(22, a3 << 6);
	sendCC(7, d1 << 6);
	sendCC(15, d2 << 6);
	sendCC(23, d3 << 6);
	sendCC(8, s1 << 6);
	sendCC(16, s2 << 6);
	sendCC(24, s3 << 6);
	sendCC(9, r1 << 6);
	sendCC(17, r2 << 6);
	sendCC(25, r3 << 6);

	sendCC(26, lfoSpeedBase[0] / 1.3);
	sendCC(28, lfoSpeedBase[1] / 1.3);
	sendCC(30, lfoSpeedBase[2] / 1.3);

	sendCC(27, lfoDepthBase[0]);
	sendCC(29, lfoDepthBase[1]);
	sendCC(31, lfoDepthBase[2]);

	sendCC(34, arpStepBase << 2);
	sendCC(36, arpRangeBase << 8);
	sendCC(35, arpSpeedBase << 2);

	sendCC(59, preset_data.cutoff);
	sendCC(33, resBase << 6);
}

void saveChannels() {

	EEPROM.update(3998, masterChannel);
	EEPROM.update(3997, masterChannelOut);
}
