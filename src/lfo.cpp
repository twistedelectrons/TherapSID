#include "globals.h"
#include "leds.h"
#include "lfo.h"
#include "arp.h"
#include "midi.h"

static int arpRangeLfo1, arpRangeLfo3, arpRangeLfo2;
static int arpSpeedLfo1, arpSpeedLfo2, arpSpeedLfo3;
static int arpStep;
static int arpStepLast, arpStepLfo1, arpStepLfo2, arpStepLfo3;
static int finalCut;
static int lfoCut1, lfoCut2, lfoCut3;
static int lfoSpeedLfo1, lfoSpeedLfo2, lfoSpeedLfo3, lfoSpeedLfo4, lfoSpeedLfo5, lfoSpeedLfo6, lfoDepthLfo1,
    lfoDepthLfo2, lfoDepthLfo3, lfoDepthLfo4, lfoDepthLfo5, lfoDepthLfo6;
static int pw1, pw2, pw3;
static int pw1Lfo1, pw1Lfo2, pw1Lfo3, pw2Lfo1, pw2Lfo2, pw2Lfo3, pw3Lfo1, pw3Lfo2, pw3Lfo3;
static byte resLfo1, resLfo2, resLfo3, res;
static byte selectedLfoLast;
static int lfoDepth[3];
static byte lfoLast[3];

static const bool limitPw = true;
static const int pwMin = 10;
static const int pwMax = 2050;

void setLfo(byte number) {
	if (number != selectedLfoLast) {
		selectedLfo = selectedLfoLast = number;
		// FIXME update leds
	}
}

void lfoTick() {

	for (int i = 0; i < 3; i++) {
		if (!cvActive[i]) {
			switch (lfoShape[i]) {
				case 0:
					lfo[i] = 255;
					break; // manual
				case 1:
					if (lfoStep[i] > 127) {
						lfo[i] = 0;
					} else {
						lfo[i] = 255;
					}
					break; // square
				case 2:
					if (lfoStep[i] < 128) {
						lfo[i] = lfoStep[i] << 1;
					} else {
						lfo[i] = 255 - ((lfoStep[i] - 128) << 1);
					}
					break; // triangle
				case 3:
					lfo[i] = 255 - lfoStep[i];
					break; // saw
				case 4:
					if (sync) {
						if (lfoNewRand[i]) {
							lfo[i] = random(255);
							lfoNewRand[i] = false;
						}
					} else {
						if ((lfoStep[i] == 1) || (lfoStep[i] == 50) || (lfoStep[i] == 100) || (lfoStep[i] == 150) ||
						    (lfoStep[i] == 200)) {
							lfo[i] = random(255);
						}
					}

					break; // noise
				case 5:
					lfo[i] = env;
					break; // env3
			}
		}
	}

	if (preset_data.lfo_map[1][10]) {
		lfoDepthLfo1 = map(lfo[1], 0, 255, 0, lfoDepth[1]);
	} else {
		lfoDepthLfo1 = 0;
	}
	if (preset_data.lfo_map[2][10]) {
		lfoDepthLfo2 = map(lfo[2], 0, 255, 0, lfoDepth[2]);
	} else {
		lfoDepthLfo2 = 0;
	}

	if (preset_data.lfo_map[0][12]) {
		lfoDepthLfo3 = map(lfo[0], 0, 255, 0, lfoDepth[0]);
	} else {
		lfoDepthLfo3 = 0;
	}
	if (preset_data.lfo_map[2][12]) {
		lfoDepthLfo4 = map(lfo[2], 0, 255, 0, lfoDepth[2]);
	} else {
		lfoDepthLfo4 = 0;
	}

	if (preset_data.lfo_map[0][14]) {
		lfoDepthLfo5 = map(lfo[0], 0, 255, 0, lfoDepth[0]);
	} else {
		lfoDepthLfo5 = 0;
	}
	if (preset_data.lfo_map[1][14]) {
		lfoDepthLfo6 = map(lfo[1], 0, 255, 0, lfoDepth[1]);
	} else {
		lfoDepthLfo6 = 0;
	}

	lfoDepth[0] = preset_data.lfo[0].depth + lfoDepthLfo1 + lfoDepthLfo2;
	if (lfoDepth[0] > 1023) {
		lfoDepth[0] = 1023;
	}

	lfoDepth[1] = preset_data.lfo[1].depth + lfoDepthLfo3 + lfoDepthLfo4;
	if (lfoDepth[1] > 1023) {
		lfoDepth[1] = 1023;
	}

	lfoDepth[2] = preset_data.lfo[2].depth + lfoDepthLfo5 + lfoDepthLfo6;
	if (lfoDepth[2] > 1023) {
		lfoDepth[2] = 1023;
	}

	if (lfoSpeed[0] < 1)
		lfoSpeed[0] = 0;

	if (preset_data.lfo_map[0][1]) {
		lfoTune1 = map(lfo[0], 0, 255, -lfoDepth[0] >> 5, lfoDepth[0] >> 5);
	} else {
		lfoTune1 = 0;
	}
	if (preset_data.lfo_map[1][1]) {
		lfoTune2 = map(lfo[1], 0, 255, -lfoDepth[1] >> 5, lfoDepth[1] >> 5);
	} else {
		lfoTune2 = 0;
	}
	if (preset_data.lfo_map[2][1]) {
		lfoTune3 = map(lfo[2], 0, 255, -lfoDepth[2] >> 5, lfoDepth[2] >> 5);
	} else {
		lfoTune3 = 0;
	}

	if (preset_data.lfo_map[0][4]) {
		lfoTune4 = map(lfo[0], 0, 255, -lfoDepth[0] >> 5, lfoDepth[0] >> 5);
	} else {
		lfoTune4 = 0;
	}
	if (preset_data.lfo_map[1][4]) {
		lfoTune5 = map(lfo[1], 0, 255, -lfoDepth[1] >> 5, lfoDepth[1] >> 5);
	} else {
		lfoTune5 = 0;
	}
	if (preset_data.lfo_map[2][4]) {
		lfoTune6 = map(lfo[2], 0, 255, -lfoDepth[2] >> 5, lfoDepth[2] >> 5);
	} else {
		lfoTune6 = 0;
	}

	if (preset_data.lfo_map[0][7]) {
		lfoTune7 = map(lfo[0], 0, 255, -lfoDepth[0] >> 5, lfoDepth[0] >> 5);
		lfoTune7 *= 2;
	} else {
		lfoTune7 = 0;
	}
	if (preset_data.lfo_map[1][7]) {
		lfoTune8 = map(lfo[1], 0, 255, -lfoDepth[1] >> 5, lfoDepth[1] >> 5);
		lfoTune8 *= 2;
	} else {
		lfoTune8 = 0;
	}
	if (preset_data.lfo_map[2][7]) {
		lfoTune9 = map(lfo[2], 0, 255, -lfoDepth[2] >> 5, lfoDepth[2] >> 5);
		lfoTune9 *= 2;
	} else {
		lfoTune9 = 0;
	}

	if (preset_data.lfo_map[0][15]) {
		lfoCut1 = map(lfo[0], 0, 255, 0, lfoDepth[0]);
	} else {
		lfoCut1 = 0;
	}
	if (preset_data.lfo_map[1][15]) {
		lfoCut2 = map(lfo[1], 0, 255, 0, lfoDepth[1]);
	} else {
		lfoCut2 = 0;
	}
	if (preset_data.lfo_map[2][15]) {
		lfoCut3 = map(lfo[2], 0, 255, 0, lfoDepth[2]);
	} else {
		lfoCut3 = 0;
	}

	if (preset_data.lfo_map[0][0]) {
		pw1Lfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0] << 1);
	} else {
		pw1Lfo1 = 0;
	}
	if (preset_data.lfo_map[1][0]) {
		pw1Lfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1] << 1);
	} else {
		pw1Lfo2 = 0;
	}
	if (preset_data.lfo_map[2][0]) {
		pw1Lfo3 = map(lfo[2], 0, 255, 0, lfoDepth[2] << 1);
	} else {
		pw1Lfo3 = 0;
	}

	if (preset_data.lfo_map[0][3]) {
		pw2Lfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0] << 1);
	} else {
		pw2Lfo1 = 0;
	}
	if (preset_data.lfo_map[1][3]) {
		pw2Lfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1] << 1);
	} else {
		pw2Lfo2 = 0;
	}
	if (preset_data.lfo_map[2][3]) {
		pw2Lfo3 = map(lfo[2], 0, 255, 0, lfoDepth[2] << 1);
	} else {
		pw2Lfo3 = 0;
	}

	if (preset_data.lfo_map[0][6]) {
		pw3Lfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0] << 1);
	} else {
		pw3Lfo1 = 0;
	}
	if (preset_data.lfo_map[1][6]) {
		pw3Lfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1] << 1);
	} else {
		pw3Lfo2 = 0;
	}
	if (preset_data.lfo_map[2][6]) {
		pw3Lfo3 = map(lfo[2], 0, 255, 0, lfoDepth[2] << 1);
	} else {
		pw3Lfo3 = 0;
	}

	if (preset_data.lfo_map[0][2]) {
		lfoFine1 = map(lfo[0], 0, 255, 0, lfoDepth[0]);
		lfoFine1 /= 1023;
	} else {
		lfoFine1 = 0;
	}
	if (preset_data.lfo_map[1][2]) {
		lfoFine2 = map(lfo[1], 0, 255, 0, lfoDepth[1]);
		lfoFine2 /= 1023;
	} else {
		lfoFine2 = 0;
	}
	if (preset_data.lfo_map[2][2]) {
		lfoFine3 = map(lfo[2], 0, 255, 0, lfoDepth[2]);
		lfoFine3 /= 1023;
	} else {
		lfoFine3 = 0;
	}

	if (preset_data.lfo_map[0][5]) {
		lfoFine4 = map(lfo[0], 0, 255, 0, lfoDepth[0]);
		lfoFine4 /= 1023;
	} else {
		lfoFine4 = 0;
	}
	if (preset_data.lfo_map[1][5]) {
		lfoFine5 = map(lfo[1], 0, 255, 0, lfoDepth[1]);
		lfoFine5 /= 1023;
	} else {
		lfoFine5 = 0;
	}
	if (preset_data.lfo_map[2][5]) {
		lfoFine6 = map(lfo[2], 0, 255, 0, lfoDepth[2]);
		lfoFine6 /= 1023;
	} else {
		lfoFine6 = 0;
	}

	if (preset_data.lfo_map[0][8]) {
		lfoFine7 = map(lfo[0], 0, 255, 0, lfoDepth[0]);
		lfoFine7 /= 1023;
	} else {
		lfoFine7 = 0;
	}
	if (preset_data.lfo_map[1][8]) {
		lfoFine8 = map(lfo[1], 0, 255, 0, lfoDepth[1]);
		lfoFine8 /= 1023;
	} else {
		lfoFine8 = 0;
	}
	if (preset_data.lfo_map[2][8]) {
		lfoFine9 = map(lfo[2], 0, 255, 0, lfoDepth[2]);
		lfoFine9 /= 1023;
	} else {
		lfoFine9 = 0;
	}

	if (preset_data.lfo_map[0][16]) {
		resLfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0]) >> 6;
	} else {
		resLfo1 = 0;
	}
	if (preset_data.lfo_map[1][16]) {
		resLfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1]) >> 6;
	} else {
		resLfo2 = 0;
	}
	if (preset_data.lfo_map[2][16]) {
		resLfo3 = map(lfo[2], 0, 255, 0, lfoDepth[2]) >> 6;
	} else {
		resLfo3 = 0;
	}

	if (preset_data.lfo_map[1][9]) {
		lfoSpeedLfo1 = map(lfo[1], 0, 255, 0, lfoDepth[1]);
	} else {
		lfoSpeedLfo1 = 0;
	}
	if (preset_data.lfo_map[2][9]) {
		lfoSpeedLfo2 = map(lfo[2], 0, 255, 0, lfoDepth[2]);
	} else {
		lfoSpeedLfo2 = 0;
	}

	if (preset_data.lfo_map[0][11]) {
		lfoSpeedLfo3 = map(lfo[0], 0, 255, 0, lfoDepth[0]);
	} else {
		lfoSpeedLfo3 = 0;
	}
	if (preset_data.lfo_map[2][11]) {
		lfoSpeedLfo4 = map(lfo[2], 0, 255, 0, lfoDepth[2]);
	} else {
		lfoSpeedLfo4 = 0;
	}

	if (preset_data.lfo_map[0][13]) {
		lfoSpeedLfo5 = map(lfo[0], 0, 255, 0, lfoDepth[0]);
	} else {
		lfoSpeedLfo5 = 0;
	}
	if (preset_data.lfo_map[1][13]) {
		lfoSpeedLfo6 = map(lfo[1], 0, 255, 0, lfoDepth[1]);
	} else {
		lfoSpeedLfo6 = 0;
	}

	if (preset_data.lfo_map[0][18]) {
		arpSpeedLfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0]) << 2;
	} else {
		arpSpeedLfo1 = 0;
	}
	if (preset_data.lfo_map[1][18]) {
		arpSpeedLfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1]) << 2;
	} else {
		arpSpeedLfo2 = 0;
	}
	if (preset_data.lfo_map[2][18]) {
		arpSpeedLfo3 = map(lfo[2], 0, 255, 0, lfoDepth[2]) << 2;
	} else {
		arpSpeedLfo3 = 0;
	}

	if (preset_data.lfo_map[0][19]) {
		arpRangeLfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0]) >> 7;
	} else {
		arpRangeLfo1 = 0;
	}
	if (preset_data.lfo_map[1][19]) {
		arpRangeLfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1]) >> 7;
	} else {
		arpRangeLfo2 = 0;
	}
	if (preset_data.lfo_map[2][19]) {
		arpRangeLfo3 = map(lfo[2], 0, 255, 0, lfoDepth[2]) >> 7;
	} else {
		arpRangeLfo3 = 0;
	}

	if (preset_data.lfo_map[0][17]) {
		arpStepLfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0] >> 2);
	} else {
		arpStepLfo1 = 0;
	}
	if (preset_data.lfo_map[1][17]) {
		arpStepLfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1] >> 2);
	} else {
		arpStepLfo2 = 0;
	}
	if (preset_data.lfo_map[2][17]) {
		arpStepLfo3 = map(lfo[2], 0, 255, 0, lfoDepth[2] >> 2);
	} else {
		arpStepLfo3 = 0;
	}

	if ((held) && (!arpMode)) {

		arpStep = arpStepBase + arpStepLfo1 + arpStepLfo2 + arpStepLfo3;
		if (arpStep > 255) {
			arpStep = 255;
		} else if (arpStep < 1) {
			arpStep = 0;
		}

		if (arpStep != arpStepLast) {
			arpStepLast = arpStep;
			arpSteptrigger(arpStepLast);
		}
	}

	arpRange = arpRangeBase + arpRangeLfo1 + arpRangeLfo2 + arpRangeLfo3;
	if (arpRange > 3)
		arpRange = 3;

	arpSpeed = (arpSpeedBase << 4) - arpSpeedLfo1 - arpSpeedLfo2 - arpSpeedLfo3;
	if (arpSpeed < 0)
		arpSpeed = 0;

	lfoSpeed[0] = preset_data.lfo[0].speed + lfoSpeedLfo1 + lfoSpeedLfo2;

	lfoSpeed[1] = preset_data.lfo[1].speed + lfoSpeedLfo3 + lfoSpeedLfo4;

	lfoSpeed[2] = preset_data.lfo[2].speed + lfoSpeedLfo5 + lfoSpeedLfo6;

	res = preset_data.resonance_base + resLfo1 + resLfo2 + resLfo3;
	if (res > 15)
		res = 15;

	// bitWrite(sid[23], 4, bitRead(res, 0)); // FIXME restore resonance LFO
	// bitWrite(sid[23], 5, bitRead(res, 1));
	// bitWrite(sid[23], 6, bitRead(res, 2));
	// bitWrite(sid[23], 7, bitRead(res, 3));

	finalCut = preset_data.cutoff + lfoCut1 + lfoCut2 + lfoCut3;
	if (finalCut < 0) {
		finalCut = 0;
	} else if (finalCut > 1023) {
		finalCut = 1023;
	}

	// sid[21] = finalCut; // FIXME restore cutoff LFO
	// sid[22] = finalCut >> 3;

	pw1 = preset_data.voice[0].pulsewidth_base + pw1Lfo1 + pw1Lfo2 + pw1Lfo3;
	if (pw1 < 0) {
		pw1 = 0;
	} else if (pw1 > 2046) {
		pw1 = 2046;
	}

	if (limitPw) {
		pw1 = constrain(pw1, pwMin, pwMax);
	}

	// sid[2] = lowByte(pw1); // FIXME restore pulse width LFO
	// sid[3] = highByte(pw1);

	pw2 = preset_data.voice[1].pulsewidth_base + pw2Lfo1 + pw2Lfo2 + pw2Lfo3;
	if (pw2 < 0) {
		pw2 = 0;
	} else if (pw2 > 2046) {
		pw2 = 2046;
	}

	if (limitPw) {
		pw2 = constrain(pw2, pwMin, pwMax);
	}

	// sid[9] = lowByte(pw2); // FIXME
	// sid[10] = highByte(pw2);

	pw3 = preset_data.voice[2].pulsewidth_base + pw3Lfo1 + pw3Lfo2 + pw3Lfo3;
	if (pw3 < 0) {
		pw3 = 0;
	} else if (pw3 > 2046) {
		pw3 = 2046;
	}

	if (limitPw) {
		pw3 = constrain(pw3, pwMin, pwMax);
	}

	// sid[16] = lowByte(pw3); // FIXME
	// sid[17] = highByte(pw3);

	byte temp;
	for (int i = 0; i < 3; i++) {
		temp = map(lfo[i], 0, 255, 0, lfoDepth[i]) >> 3;
		if (temp != lfoLast[i]) {
			lfoLast[i] = temp;
			if (sendLfo)
				sendControlChange(56 + i, temp);
		}
	}
}

void lastMovedPot(byte number) {
	lastPot = number;
	preset_data.set_leds(lastPot, selectedLfo, filterModeHeld);
}

void chain() {
	if (lastPot != 20) {
		preset_data.lfo_map[selectedLfo][lastPot] = !preset_data.lfo_map[selectedLfo][lastPot];

		lastMovedPot(lastPot);
	}
}

void clearLfo() {

	for (int i = 0; i < 20; i++) {
		preset_data.lfo_map[selectedLfo][i] = 0;
	}
	digit(0, 10);
	digit(1, 11);
}
