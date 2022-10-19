#include <MIDI.h>

#include "globals.h"
#include "leds.h"
#include "lfo.h"
#include "arp.h"

void setLfo(byte number) {

	if (number != selectedLfoLast) {
		selectedLfo = selectedLfoLast = number;
		showLfo(); //
	}
}

void showLfo() {

	ledSet(22, 0);
	ledSet(23, 0);
	ledSet(24, 0);
	ledSet(25, 0);
	ledSet(26, 0);

	if (lfoShape[selectedLfo]) {
		ledSet(21 + lfoShape[selectedLfo], 1);
	}
	ledSet(30, retrig[selectedLfo]);

	ledSet(31, looping[selectedLfo]);
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

	if (lfoAss[1][10]) {
		lfoDepthLfo1 = map(lfo[1], 0, 255, 0, lfoDepth[1]);
	} else {
		lfoDepthLfo1 = 0;
	}
	if (lfoAss[2][10]) {
		lfoDepthLfo2 = map(lfo[2], 0, 255, 0, lfoDepth[2]);
	} else {
		lfoDepthLfo2 = 0;
	}

	if (lfoAss[0][12]) {
		lfoDepthLfo3 = map(lfo[0], 0, 255, 0, lfoDepth[0]);
	} else {
		lfoDepthLfo3 = 0;
	}
	if (lfoAss[2][12]) {
		lfoDepthLfo4 = map(lfo[2], 0, 255, 0, lfoDepth[2]);
	} else {
		lfoDepthLfo4 = 0;
	}

	if (lfoAss[0][14]) {
		lfoDepthLfo5 = map(lfo[0], 0, 255, 0, lfoDepth[0]);
	} else {
		lfoDepthLfo5 = 0;
	}
	if (lfoAss[1][14]) {
		lfoDepthLfo6 = map(lfo[1], 0, 255, 0, lfoDepth[1]);
	} else {
		lfoDepthLfo6 = 0;
	}

	lfoDepth[0] = lfoDepthBase[0] + lfoDepthLfo1 + lfoDepthLfo2;
	if (lfoDepth[0] > 1023) {
		lfoDepth[0] = 1023;
	}

	lfoDepth[1] = lfoDepthBase[1] + lfoDepthLfo3 + lfoDepthLfo4;
	if (lfoDepth[1] > 1023) {
		lfoDepth[1] = 1023;
	}

	lfoDepth[2] = lfoDepthBase[2] + lfoDepthLfo5 + lfoDepthLfo6;
	if (lfoDepth[2] > 1023) {
		lfoDepth[2] = 1023;
	}

	if (lfoSpeed[0] < 1)
		lfoSpeed[0] = 0;

	if (lfoAss[0][1]) {
		lfoTune1 = map(lfo[0], 0, 255, -lfoDepth[0] >> 5, lfoDepth[0] >> 5);
	} else {
		lfoTune1 = 0;
	}
	if (lfoAss[1][1]) {
		lfoTune2 = map(lfo[1], 0, 255, -lfoDepth[1] >> 5, lfoDepth[1] >> 5);
	} else {
		lfoTune2 = 0;
	}
	if (lfoAss[2][1]) {
		lfoTune3 = map(lfo[2], 0, 255, -lfoDepth[2] >> 5, lfoDepth[2] >> 5);
	} else {
		lfoTune3 = 0;
	}

	if (lfoAss[0][4]) {
		lfoTune4 = map(lfo[0], 0, 255, -lfoDepth[0] >> 5, lfoDepth[0] >> 5);
	} else {
		lfoTune4 = 0;
	}
	if (lfoAss[1][4]) {
		lfoTune5 = map(lfo[1], 0, 255, -lfoDepth[1] >> 5, lfoDepth[1] >> 5);
	} else {
		lfoTune5 = 0;
	}
	if (lfoAss[2][4]) {
		lfoTune6 = map(lfo[2], 0, 255, -lfoDepth[2] >> 5, lfoDepth[2] >> 5);
	} else {
		lfoTune6 = 0;
	}

	if (lfoAss[0][7]) {
		lfoTune7 = map(lfo[0], 0, 255, -lfoDepth[0] >> 5, lfoDepth[0] >> 5);
		lfoTune7 *= 2;
	} else {
		lfoTune7 = 0;
	}
	if (lfoAss[1][7]) {
		lfoTune8 = map(lfo[1], 0, 255, -lfoDepth[1] >> 5, lfoDepth[1] >> 5);
		lfoTune8 *= 2;
	} else {
		lfoTune8 = 0;
	}
	if (lfoAss[2][7]) {
		lfoTune9 = map(lfo[2], 0, 255, -lfoDepth[2] >> 5, lfoDepth[2] >> 5);
		lfoTune9 *= 2;
	} else {
		lfoTune9 = 0;
	}

	if (lfoAss[0][15]) {
		lfoCut1 = map(lfo[0], 0, 255, 0, lfoDepth[0]);
	} else {
		lfoCut1 = 0;
	}
	if (lfoAss[1][15]) {
		lfoCut2 = map(lfo[1], 0, 255, 0, lfoDepth[1]);
	} else {
		lfoCut2 = 0;
	}
	if (lfoAss[2][15]) {
		lfoCut3 = map(lfo[2], 0, 255, 0, lfoDepth[2]);
	} else {
		lfoCut3 = 0;
	}

	if (lfoAss[0][0]) {
		pw1Lfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0] << 1);
	} else {
		pw1Lfo1 = 0;
	}
	if (lfoAss[1][0]) {
		pw1Lfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1] << 1);
	} else {
		pw1Lfo2 = 0;
	}
	if (lfoAss[2][0]) {
		pw1Lfo3 = map(lfo[2], 0, 255, 0, lfoDepth[2] << 1);
	} else {
		pw1Lfo3 = 0;
	}

	if (lfoAss[0][3]) {
		pw2Lfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0] << 1);
	} else {
		pw2Lfo1 = 0;
	}
	if (lfoAss[1][3]) {
		pw2Lfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1] << 1);
	} else {
		pw2Lfo2 = 0;
	}
	if (lfoAss[2][3]) {
		pw2Lfo3 = map(lfo[2], 0, 255, 0, lfoDepth[2] << 1);
	} else {
		pw2Lfo3 = 0;
	}

	if (lfoAss[0][6]) {
		pw3Lfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0] << 1);
	} else {
		pw3Lfo1 = 0;
	}
	if (lfoAss[1][6]) {
		pw3Lfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1] << 1);
	} else {
		pw3Lfo2 = 0;
	}
	if (lfoAss[2][6]) {
		pw3Lfo3 = map(lfo[2], 0, 255, 0, lfoDepth[2] << 1);
	} else {
		pw3Lfo3 = 0;
	}

	if (lfoAss[0][2]) {
		lfoFine1 = map(lfo[0], 0, 255, 0, lfoDepth[0]);
		lfoFine1 /= 1023;
	} else {
		lfoFine1 = 0;
	}
	if (lfoAss[1][2]) {
		lfoFine2 = map(lfo[1], 0, 255, 0, lfoDepth[1]);
		lfoFine2 /= 1023;
	} else {
		lfoFine2 = 0;
	}
	if (lfoAss[2][2]) {
		lfoFine3 = map(lfo[2], 0, 255, 0, lfoDepth[2]);
		lfoFine3 /= 1023;
	} else {
		lfoFine3 = 0;
	}

	if (lfoAss[0][5]) {
		lfoFine4 = map(lfo[0], 0, 255, 0, lfoDepth[0]);
		lfoFine4 /= 1023;
	} else {
		lfoFine4 = 0;
	}
	if (lfoAss[1][5]) {
		lfoFine5 = map(lfo[1], 0, 255, 0, lfoDepth[1]);
		lfoFine5 /= 1023;
	} else {
		lfoFine5 = 0;
	}
	if (lfoAss[2][5]) {
		lfoFine6 = map(lfo[2], 0, 255, 0, lfoDepth[2]);
		lfoFine6 /= 1023;
	} else {
		lfoFine6 = 0;
	}

	if (lfoAss[0][8]) {
		lfoFine7 = map(lfo[0], 0, 255, 0, lfoDepth[0]);
		lfoFine7 /= 1023;
	} else {
		lfoFine7 = 0;
	}
	if (lfoAss[1][8]) {
		lfoFine8 = map(lfo[1], 0, 255, 0, lfoDepth[1]);
		lfoFine8 /= 1023;
	} else {
		lfoFine8 = 0;
	}
	if (lfoAss[2][8]) {
		lfoFine9 = map(lfo[2], 0, 255, 0, lfoDepth[2]);
		lfoFine9 /= 1023;
	} else {
		lfoFine9 = 0;
	}

	if (lfoAss[0][16]) {
		resLfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0]) >> 6;
	} else {
		resLfo1 = 0;
	}
	if (lfoAss[1][16]) {
		resLfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1]) >> 6;
	} else {
		resLfo2 = 0;
	}
	if (lfoAss[2][16]) {
		resLfo3 = map(lfo[2], 0, 255, 0, lfoDepth[2]) >> 6;
	} else {
		resLfo3 = 0;
	}

	if (lfoAss[1][9]) {
		lfoSpeedLfo1 = map(lfo[1], 0, 255, 0, lfoDepth[1]);
	} else {
		lfoSpeedLfo1 = 0;
	}
	if (lfoAss[2][9]) {
		lfoSpeedLfo2 = map(lfo[2], 0, 255, 0, lfoDepth[2]);
	} else {
		lfoSpeedLfo2 = 0;
	}

	if (lfoAss[0][11]) {
		lfoSpeedLfo3 = map(lfo[0], 0, 255, 0, lfoDepth[0]);
	} else {
		lfoSpeedLfo3 = 0;
	}
	if (lfoAss[2][11]) {
		lfoSpeedLfo4 = map(lfo[2], 0, 255, 0, lfoDepth[2]);
	} else {
		lfoSpeedLfo4 = 0;
	}

	if (lfoAss[0][13]) {
		lfoSpeedLfo5 = map(lfo[0], 0, 255, 0, lfoDepth[0]);
	} else {
		lfoSpeedLfo5 = 0;
	}
	if (lfoAss[1][13]) {
		lfoSpeedLfo6 = map(lfo[1], 0, 255, 0, lfoDepth[1]);
	} else {
		lfoSpeedLfo6 = 0;
	}

	if (lfoAss[0][18]) {
		arpSpeedLfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0]) << 2;
	} else {
		arpSpeedLfo1 = 0;
	}
	if (lfoAss[1][18]) {
		arpSpeedLfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1]) << 2;
	} else {
		arpSpeedLfo2 = 0;
	}
	if (lfoAss[2][18]) {
		arpSpeedLfo3 = map(lfo[2], 0, 255, 0, lfoDepth[2]) << 2;
	} else {
		arpSpeedLfo3 = 0;
	}

	if (lfoAss[0][19]) {
		arpRangeLfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0]) >> 7;
	} else {
		arpRangeLfo1 = 0;
	}
	if (lfoAss[1][19]) {
		arpRangeLfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1]) >> 7;
	} else {
		arpRangeLfo2 = 0;
	}
	if (lfoAss[2][19]) {
		arpRangeLfo3 = map(lfo[2], 0, 255, 0, lfoDepth[2]) >> 7;
	} else {
		arpRangeLfo3 = 0;
	}

	if (lfoAss[0][17]) {
		arpStepLfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0] >> 2);
	} else {
		arpStepLfo1 = 0;
	}
	if (lfoAss[1][17]) {
		arpStepLfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1] >> 2);
	} else {
		arpStepLfo2 = 0;
	}
	if (lfoAss[2][17]) {
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
	arpSpeed = arpSpeed;
	if (arpSpeed < 0)
		arpSpeed = 0;

	lfoSpeed[0] = lfoSpeedBase[0] + lfoSpeedLfo1 + lfoSpeedLfo2;

	lfoSpeed[1] = lfoSpeedBase[1] + lfoSpeedLfo3 + lfoSpeedLfo4;

	lfoSpeed[2] = lfoSpeedBase[2] + lfoSpeedLfo5 + lfoSpeedLfo6;

	res = resBase + resLfo1 + resLfo2 + resLfo3;
	if (res > 15)
		res = 15;

	bitWrite(sid[23], 4, bitRead(res, 0));
	bitWrite(sid[23], 5, bitRead(res, 1));
	bitWrite(sid[23], 6, bitRead(res, 2));
	bitWrite(sid[23], 7, bitRead(res, 3));

	finalCut = cutBase + lfoCut1 + lfoCut2 + lfoCut3;
	if (finalCut < 0) {
		finalCut = 0;
	} else if (finalCut > 1023) {
		finalCut = 1023;
	}

	sid[21] = finalCut;
	sid[22] = finalCut >> 3;

	pw1 = pw1Base + pw1Lfo1 + pw1Lfo2 + pw1Lfo3;
	if (pw1 < 0) {
		pw1 = 0;
	} else if (pw1 > 2046) {
		pw1 = 2046;
	}

	sid[2] = lowByte(pw1);
	sid[3] = highByte(pw1);

	pw2 = pw2Base + pw2Lfo1 + pw2Lfo2 + pw2Lfo3;
	if (pw2 < 0) {
		pw2 = 0;
	} else if (pw2 > 2046) {
		pw2 = 2046;
	}

	sid[9] = lowByte(pw2);
	sid[10] = highByte(pw2);

	pw3 = pw3Base + pw3Lfo1 + pw3Lfo2 + pw3Lfo3;
	if (pw3 < 0) {
		pw3 = 0;
	} else if (pw3 > 2046) {
		pw3 = 2046;
	}

	sid[16] = lowByte(pw3);
	sid[17] = highByte(pw3);

	byte temp;
	for (int i = 0; i < 3; i++) {
		temp = map(lfo[i], 0, 255, 0, lfoDepth[i]) >> 3;
		if (temp != lfoLast[i]) {
			lfoLast[i] = temp;
			if (sendLfo)
				MIDI.sendControlChange(56 + i, temp, masterChannelOut);
		}
	}
}

void lastMovedPot(byte number) {
	lastPot = number;
	if (lastPot != 20) {

		ledSet(13, lfoAss[0][number]);
		ledSet(14, lfoAss[1][number]);
		ledSet(15, lfoAss[2][number]);
	}
}

void chain() {
	if (lastPot != 20) {
		lfoAss[selectedLfo][lastPot] = !lfoAss[selectedLfo][lastPot];

		lastMovedPot(lastPot);
	}
}

void clearLfo() {

	for (int i = 0; i < 20; i++) {
		lfoAss[selectedLfo][i] = 0;
	}
	digit(0, 10);
	digit(1, 11);
}

void checkIllegalModulation() {}
