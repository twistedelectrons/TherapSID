#include "globals.h"
#include "lfo.h"
#include "arp.h"
#include "midi.h"

static int arpRangeLfo1, arpRangeLfo3, arpRangeLfo2;
static int arpSpeedLfo1, arpSpeedLfo2, arpSpeedLfo3;
static int arpStep;
static int arpStepLast, arpStepLfo1, arpStepLfo2, arpStepLfo3;
static int lfoCut1, lfoCut2, lfoCut3;
static int lfoSpeedLfo1, lfoSpeedLfo2, lfoSpeedLfo3, lfoSpeedLfo4, lfoSpeedLfo5, lfoSpeedLfo6, lfoDepthLfo1,
    lfoDepthLfo2, lfoDepthLfo3, lfoDepthLfo4, lfoDepthLfo5, lfoDepthLfo6;
static int pw1Lfo1, pw1Lfo2, pw1Lfo3, pw2Lfo1, pw2Lfo2, pw2Lfo3, pw3Lfo1, pw3Lfo2, pw3Lfo3;
static byte resLfo1, resLfo2, resLfo3;
static int lfoDepth[3] = {0, 0, 0};
static byte lfoLast[3];
static byte lfoStepQuantized[3];
static const int pwMin = 64;
static const int pwMax = 2050;

/// Sets a lot of global variables and returns sid's parameters after lfo'ing them.
ParamsAfterLfo lfoTick() {
	ParamsAfterLfo result;

	for (int i = 0; i < 3; i++) {
		if (!cvActive[i]) {
			switch (preset_data.lfo[i].shape) {
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

						if (lfoStepQuantized[i] != lfoStep[i] >> 6) {
							lfoStepQuantized[i] = lfoStep[i] >> 6; // generate noise 4 times per LFO cycle
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

	if (preset_data.lfo[1].mapping[10]) {
		lfoDepthLfo1 = map(lfo[1], 0, 255, -lfoDepth[1], lfoDepth[1]);
	} else {
		lfoDepthLfo1 = 0;
	}
	if (preset_data.lfo[2].mapping[10]) {
		lfoDepthLfo2 = map(lfo[2], 0, 255, -lfoDepth[2], lfoDepth[2]);
	} else {
		lfoDepthLfo2 = 0;
	}

	if (preset_data.lfo[0].mapping[12]) {
		lfoDepthLfo3 = map(lfo[0], 0, 255, -lfoDepth[0], lfoDepth[0]);
	} else {
		lfoDepthLfo3 = 0;
	}
	if (preset_data.lfo[2].mapping[12]) {
		lfoDepthLfo4 = map(lfo[2], 0, 255, -lfoDepth[2], lfoDepth[2]);
	} else {
		lfoDepthLfo4 = 0;
	}

	if (preset_data.lfo[0].mapping[14]) {
		lfoDepthLfo5 = map(lfo[0], 0, 255, -lfoDepth[0], lfoDepth[0]);
	} else {
		lfoDepthLfo5 = 0;
	}
	if (preset_data.lfo[1].mapping[14]) {
		lfoDepthLfo6 = map(lfo[1], 0, 255, -lfoDepth[1], lfoDepth[1]);
	} else {
		lfoDepthLfo6 = 0;
	}

	lfoDepth[0] = preset_data.lfo[0].depth + lfoDepthLfo1 + lfoDepthLfo2;
	if (modToLfo)
		lfoDepth[0] += (modWheelLast << 3);
	if (lfoDepth[0] > 1023) {
		lfoDepth[0] = 1023;
	}

	lfoDepth[1] = preset_data.lfo[1].depth + lfoDepthLfo3 + lfoDepthLfo4;
	if (aftertouchToLfo)
		lfoDepth[1] += (aftertouch << 3);

	if (lfoDepth[1] > 1023) {
		lfoDepth[1] = 1023;
	}

	lfoDepth[2] = preset_data.lfo[2].depth + lfoDepthLfo5 + lfoDepthLfo6;
	if (velocityToLfo) {
		lfoDepth[2] += velocityLast << 3;
	}
	if (lfoDepth[2] > 1023) {
		lfoDepth[2] = 1023;
	}

	if (preset_data.lfo[0].mapping[1]) {
		lfoTune1 = map(lfo[0], 0, 255, -lfoDepth[0] >> tuneLfoRange, lfoDepth[0] >> tuneLfoRange);
	} else {
		lfoTune1 = 0;
	}
	if (preset_data.lfo[1].mapping[1]) {
		lfoTune2 = map(lfo[1], 0, 255, -lfoDepth[1] >> tuneLfoRange, lfoDepth[1] >> tuneLfoRange);
	} else {
		lfoTune2 = 0;
	}
	if (preset_data.lfo[2].mapping[1]) {
		lfoTune3 = map(lfo[2], 0, 255, -lfoDepth[2] >> tuneLfoRange, lfoDepth[2] >> tuneLfoRange);
	} else {
		lfoTune3 = 0;
	}

	if (preset_data.lfo[0].mapping[4]) {
		lfoTune4 = map(lfo[0], 0, 255, -lfoDepth[0] >> tuneLfoRange, lfoDepth[0] >> tuneLfoRange);
	} else {
		lfoTune4 = 0;
	}
	if (preset_data.lfo[1].mapping[4]) {
		lfoTune5 = map(lfo[1], 0, 255, -lfoDepth[1] >> tuneLfoRange, lfoDepth[1] >> tuneLfoRange);
	} else {
		lfoTune5 = 0;
	}
	if (preset_data.lfo[2].mapping[4]) {
		lfoTune6 = map(lfo[2], 0, 255, -lfoDepth[2] >> tuneLfoRange, lfoDepth[2] >> tuneLfoRange);
	} else {
		lfoTune6 = 0;
	}

	if (preset_data.lfo[0].mapping[7]) {
		lfoTune7 = map(lfo[0], 0, 255, -lfoDepth[0] >> tuneLfoRange, lfoDepth[0] >> tuneLfoRange);
		lfoTune7 *= 2;
	} else {
		lfoTune7 = 0;
	}
	if (preset_data.lfo[1].mapping[7]) {
		lfoTune8 = map(lfo[1], 0, 255, -lfoDepth[1] >> tuneLfoRange, lfoDepth[1] >> tuneLfoRange);
		lfoTune8 *= 2;
	} else {
		lfoTune8 = 0;
	}
	if (preset_data.lfo[2].mapping[7]) {
		lfoTune9 = map(lfo[2], 0, 255, -lfoDepth[2] >> tuneLfoRange, lfoDepth[2] >> tuneLfoRange);
		lfoTune9 *= 2;
	} else {
		lfoTune9 = 0;
	}

	if (preset_data.lfo[0].mapping[15]) {
		lfoCut1 = map(lfo[0], 0, 255, -lfoDepth[0], lfoDepth[0]);
	} else {
		lfoCut1 = 0;
	}
	if (preset_data.lfo[1].mapping[15]) {
		lfoCut2 = map(lfo[1], 0, 255, -lfoDepth[1], lfoDepth[1]);
	} else {
		lfoCut2 = 0;
	}
	if (preset_data.lfo[2].mapping[15]) {
		lfoCut3 = map(lfo[2], 0, 255, -lfoDepth[2], lfoDepth[2]);
	} else {
		lfoCut3 = 0;
	}

	if (preset_data.lfo[0].mapping[0]) {
		pw1Lfo1 = map(lfo[0], 0, 255, -lfoDepth[0], lfoDepth[0] << 1);
	} else {
		pw1Lfo1 = 0;
	}
	if (preset_data.lfo[1].mapping[0]) {
		pw1Lfo2 = map(lfo[1], 0, 255, -lfoDepth[1], lfoDepth[1] << 1);
	} else {
		pw1Lfo2 = 0;
	}
	if (preset_data.lfo[2].mapping[0]) {
		pw1Lfo3 = map(lfo[2], 0, 255, -lfoDepth[2], lfoDepth[2] << 1);
	} else {
		pw1Lfo3 = 0;
	}

	if (preset_data.lfo[0].mapping[3]) {
		pw2Lfo1 = map(lfo[0], 0, 255, -lfoDepth[0], lfoDepth[0] << 1);
	} else {
		pw2Lfo1 = 0;
	}
	if (preset_data.lfo[1].mapping[3]) {
		pw2Lfo2 = map(lfo[1], 0, 255, -lfoDepth[1], lfoDepth[1] << 1);
	} else {
		pw2Lfo2 = 0;
	}
	if (preset_data.lfo[2].mapping[3]) {
		pw2Lfo3 = map(lfo[2], 0, 255, -lfoDepth[2], lfoDepth[2] << 1);
	} else {
		pw2Lfo3 = 0;
	}

	if (preset_data.lfo[0].mapping[6]) {
		pw3Lfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0] << 1);
	} else {
		pw3Lfo1 = 0;
	}
	if (preset_data.lfo[1].mapping[6]) {
		pw3Lfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1] << 1);
	} else {
		pw3Lfo2 = 0;
	}
	if (preset_data.lfo[2].mapping[6]) {
		pw3Lfo3 = map(lfo[2], 0, 255, 0, lfoDepth[2] << 1);
	} else {
		pw3Lfo3 = 0;
	}

	if (preset_data.lfo[0].mapping[2]) {
		lfoFine1 = map(lfo[0], 0, 255, -lfoDepth[0], lfoDepth[0]);
		lfoFine1 /= 1023;
	} else {
		lfoFine1 = 0;
	}
	if (preset_data.lfo[1].mapping[2]) {
		lfoFine2 = map(lfo[1], 0, 255, -lfoDepth[1], lfoDepth[1]);
		lfoFine2 /= 1023;
	} else {
		lfoFine2 = 0;
	}
	if (preset_data.lfo[2].mapping[2]) {
		lfoFine3 = map(lfo[2], 0, 255, -lfoDepth[1], lfoDepth[1]);
		lfoFine3 /= 1023;
	} else {
		lfoFine3 = 0;
	}

	if (preset_data.lfo[0].mapping[5]) {
		lfoFine4 = map(lfo[0], 0, 255, -lfoDepth[0], lfoDepth[0]);
		lfoFine4 /= 1023;
	} else {
		lfoFine4 = 0;
	}
	if (preset_data.lfo[1].mapping[5]) {
		lfoFine5 = map(lfo[1], 0, 255, -lfoDepth[1], lfoDepth[1]);
		lfoFine5 /= 1023;
	} else {
		lfoFine5 = 0;
	}
	if (preset_data.lfo[2].mapping[5]) {
		lfoFine6 = map(lfo[2], 0, 255, -lfoDepth[2], lfoDepth[2]);
		lfoFine6 /= 1023;
	} else {
		lfoFine6 = 0;
	}

	if (preset_data.lfo[0].mapping[8]) {
		lfoFine7 = map(lfo[0], 0, 255, -lfoDepth[0], lfoDepth[0]);
		lfoFine7 /= 1023;
	} else {
		lfoFine7 = 0;
	}
	if (preset_data.lfo[1].mapping[8]) {
		lfoFine8 = map(lfo[1], 0, 255, -lfoDepth[1], lfoDepth[1]);
		lfoFine8 /= 1023;
	} else {
		lfoFine8 = 0;
	}
	if (preset_data.lfo[2].mapping[8]) {
		lfoFine9 = map(lfo[2], 0, 255, -lfoDepth[2], lfoDepth[2]);
		lfoFine9 /= 1023;
	} else {
		lfoFine9 = 0;
	}

	if (preset_data.lfo[0].mapping[16]) {
		resLfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0]) >> 6;
	} else {
		resLfo1 = 0;
	}
	if (preset_data.lfo[1].mapping[16]) {
		resLfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1]) >> 6;
	} else {
		resLfo2 = 0;
	}
	if (preset_data.lfo[2].mapping[16]) {
		resLfo3 = map(lfo[2], 0, 255, 0, lfoDepth[2]) >> 6;
	} else {
		resLfo3 = 0;
	}

	if (preset_data.lfo[1].mapping[9]) {
		lfoSpeedLfo1 = map(lfo[1], 0, 255, -lfoDepth[1], lfoDepth[1]);
	} else {
		lfoSpeedLfo1 = 0;
	}
	if (preset_data.lfo[2].mapping[9]) {
		lfoSpeedLfo2 = map(lfo[2], 0, 255, -lfoDepth[2], lfoDepth[2]);
	} else {
		lfoSpeedLfo2 = 0;
	}

	if (preset_data.lfo[0].mapping[11]) {
		lfoSpeedLfo3 = map(lfo[0], 0, 255, -lfoDepth[0], lfoDepth[0]);
	} else {
		lfoSpeedLfo3 = 0;
	}
	if (preset_data.lfo[2].mapping[11]) {
		lfoSpeedLfo4 = map(lfo[2], 0, 255, -lfoDepth[2], lfoDepth[2]);
	} else {
		lfoSpeedLfo4 = 0;
	}

	if (preset_data.lfo[0].mapping[13]) {
		lfoSpeedLfo5 = map(lfo[0], 0, 255, -lfoDepth[0], lfoDepth[0]);
	} else {
		lfoSpeedLfo5 = 0;
	}
	if (preset_data.lfo[1].mapping[13]) {
		lfoSpeedLfo6 = map(lfo[1], 0, 255, -lfoDepth[1], lfoDepth[1]);
	} else {
		lfoSpeedLfo6 = 0;
	}

	if (preset_data.lfo[0].mapping[18]) {
		arpSpeedLfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0]) << 2;
	} else {
		arpSpeedLfo1 = 0;
	}
	if (preset_data.lfo[1].mapping[18]) {
		arpSpeedLfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1]) << 2;
	} else {
		arpSpeedLfo2 = 0;
	}
	if (preset_data.lfo[2].mapping[18]) {
		arpSpeedLfo3 = map(lfo[2], 0, 255, 0, lfoDepth[2]) << 2;
	} else {
		arpSpeedLfo3 = 0;
	}

	if (preset_data.lfo[0].mapping[19]) {
		arpRangeLfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0]) >> 7;
	} else {
		arpRangeLfo1 = 0;
	}
	if (preset_data.lfo[1].mapping[19]) {
		arpRangeLfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1]) >> 7;
	} else {
		arpRangeLfo2 = 0;
	}
	if (preset_data.lfo[2].mapping[19]) {
		arpRangeLfo3 = map(lfo[2], 0, 255, 0, lfoDepth[2]) >> 7;
	} else {
		arpRangeLfo3 = 0;
	}

	if (preset_data.lfo[0].mapping[17]) {
		arpStepLfo1 = map(lfo[0], 0, 255, 0, lfoDepth[0] >> 2);
	} else {
		arpStepLfo1 = 0;
	}
	if (preset_data.lfo[1].mapping[17]) {
		arpStepLfo2 = map(lfo[1], 0, 255, 0, lfoDepth[1] >> 2);
	} else {
		arpStepLfo2 = 0;
	}
	if (preset_data.lfo[2].mapping[17]) {
		arpStepLfo3 = map(lfo[2], 0, 255, 0, lfoDepth[2] >> 2);
	} else {
		arpStepLfo3 = 0;
	}

	if (voice_state.n_held_keys() > 0 && preset_data.arp_mode) {
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

	arpRange = preset_data.arp_range_base + arpRangeLfo1 + arpRangeLfo2 + arpRangeLfo3;
	if (arpRange > 3)
		arpRange = 3;

	arpSpeed = (preset_data.arp_speed_base << 4) - arpSpeedLfo1 - arpSpeedLfo2 - arpSpeedLfo3;
	if (arpSpeed < 0)
		arpSpeed = 0;

	lfoSpeed[0] = preset_data.lfo[0].speed + lfoSpeedLfo1 + lfoSpeedLfo2;

	lfoSpeed[1] = preset_data.lfo[1].speed + lfoSpeedLfo3 + lfoSpeedLfo4;

	lfoSpeed[2] = preset_data.lfo[2].speed + lfoSpeedLfo5 + lfoSpeedLfo6;

	result.resonance = preset_data.resonance_base + resLfo1 + resLfo2 + resLfo3;
	if (result.resonance > 15)
		result.resonance = 15;

	result.cutoff = preset_data.cutoff + lfoCut1 + lfoCut2 + lfoCut3;
	if (result.cutoff < 0) {
		result.cutoff = 0;
	} else if (result.cutoff > 1023) {
		result.cutoff = 1023;
	}

	int pw1 = preset_data.voice[0].pulsewidth_base + pw1Lfo1 + pw1Lfo2 + pw1Lfo3;
	if (pw1 < 0) {
		pw1 = 0;
	} else if (pw1 > 2046) {
		pw1 = 2046;
	}

	if (pwLimit) {
		pw1 = constrain(pw1, pwMin, pwMax);
	}

	result.pulsewidth[0] = pw1;

	int pw2 = preset_data.voice[1].pulsewidth_base + pw2Lfo1 + pw2Lfo2 + pw2Lfo3;
	if (pw2 < 0) {
		pw2 = 0;
	} else if (pw2 > 2046) {
		pw2 = 2046;
	}

	if (pwLimit) {
		pw2 = constrain(pw2, pwMin, pwMax);
	}

	result.pulsewidth[1] = pw2;

	int pw3 = preset_data.voice[2].pulsewidth_base + pw3Lfo1 + pw3Lfo2 + pw3Lfo3;
	if (pw3 < 0) {
		pw3 = 0;
	} else if (pw3 > 2046) {
		pw3 = 2046;
	}

	if (pwLimit) {
		pw3 = constrain(pw3, pwMin, pwMax);
	}

	result.pulsewidth[2] = pw3;

	byte temp;
	for (int i = 0; i < 3; i++) {
		temp = map(lfo[i], 0, 255, 0, max(0, lfoDepth[i])) >> 3;
		if (temp != lfoLast[i]) {
			lfoLast[i] = temp;
			if (sendLfo)
				sendControlChange(56 + i, temp, masterChannelOut);
		}
	}

	return result;
}
