#include "globals.h"
#include "pots.h"
#include "leds.h"
#include "lfo.h"
#include "midi.h"

static int arpDivisions[] = {1, 3, 6, 8, 12, 24, 32, 48};

static int scaleFine(int input) {

	input = map(input, 0, 1023, -51, 51);
	return (input);
}

static byte scale4bit(int input) { return (input >> 6); }

static byte scale100(int input) {
	input = map(input, 0, 1023, 0, 101);
	if (input > 99)
		input = 99;
	return (input);
}

static byte octScale(int value) {

	value = map(value, 0, 1023, 0, 25);
	if (value > 24)
		value = 24;
	return (value);
}

void movedPot(byte number, int value, bool isMidi) {

	if (!saveMode) {

		if (!isMidi) {
			lastMovedPot(20); // unselect
		}
		switch (number) {

			case 4:
				pw1Base = value << 1;
				if (pa) {
					pw2Base = pw3Base = pw1Base;
				}
				if (value < 1) {
					value = 1;
				}
				if (!isMidi) {
					sendCC(2, value);
					ledNumber(scale100(value));
					lastMovedPot(0);
				}
				break; // PW1
			case 24:
				pw2Base = value << 1;
				if (value < 1) {
					value = 1;
				}
				if (!isMidi) {
					sendCC(10, value);
					ledNumber(scale100(value));
					lastMovedPot(3);
				}
				break; // PW2
			case 30:
				pw3Base = value << 1;
				if (value < 1) {
					value = 1;
				}
				if (!isMidi) {
					sendCC(18, value);
					ledNumber(scale100(value));
					lastMovedPot(6);
				}
				break; // PW3

			case 6:
				tuneBase1 = octScale(value);
				if (pa) {
					tuneBase2 = tuneBase3 = tuneBase1;
				}
				if (!isMidi) {
					sendCC(3, value);
					ledNumber(tuneBase1 - 12);
					lastMovedPot(1);
				}
				break; // TUNE1
			case 26:
				tuneBase2 = octScale(value);
				if (!isMidi) {
					sendCC(11, value);
					ledNumber(tuneBase2 - 12);
					lastMovedPot(4);
				}
				break; // TUNE2
			case 21:
				tuneBase3 = octScale(value);
				if (!isMidi) {
					sendCC(19, value);
					ledNumber(tuneBase3 - 12);
					lastMovedPot(7);
				}
				break; // TUNE3

			case 14:
				fineBase1 = value;
				fineBase1 /= 1023;
				if (pa) {
					fineBase2 = fineBase3 = fineBase1;
				}
				if (!isMidi) {
					sendCC(4, value);
					ledNumber(scaleFine(value));
					lastMovedPot(2);
				}
				break; // FINE1
			case 17:
				fineBase2 = value;
				fineBase2 /= 1023;
				if (!isMidi) {
					sendCC(12, value);
					ledNumber(scaleFine(value));
					lastMovedPot(5);
				}
				break; // FINE2
			case 31:
				if (!isMidi) {
					sendCC(20, value);
					fineBase3 = value;
					fineBase3 /= 1023;
					ledNumber(scaleFine(value));
					lastMovedPot(8);
				}
				break; // FINE3

			case 1:
				glide1 = value >> 4;
				if (pa) {
					glide2 = glide3 = glide1;
				}
				if (!isMidi) {
					sendCC(5, value);
					ledNumber(glide1);
				}
				break; // GLIDE 1
			case 27:
				glide2 = value >> 4;
				if (!isMidi) {
					sendCC(13, value);
					ledNumber(glide2);
				}
				break; // GLIDE 2
			case 19:
				glide3 = value >> 4;
				if (!isMidi) {
					sendCC(21, value);
					ledNumber(glide3);
				}
				break; // GLIDE 3

			case 5: // ATTACK 1

				if (pa) {
					a2 = a3 = a1;
					d2 = d3 = d1;
					r2 = r3 = r1;
					s2 = s3 = s1;
					sid[12] = 255 & a2 << 4;
					sid[12] = sid[12] | d2;
					sid[19] = 255 & a3 << 4;
					sid[19] = sid[19] | d3;
				}

				if (!isMidi) {
					sendCC(6, value);
				}
				value = scale4bit(value);
				ledNumber(value);
				a1 = value;
				sid[5] = 255 & a1 << 4;
				sid[5] = sid[5] | d1;

				break;

			case 22: // ATTACK 2
				if (!isMidi) {
					sendCC(14, value);
				}
				value = scale4bit(value);
				ledNumber(value);
				a2 = value;
				sid[12] = 255 & a2 << 4;
				sid[12] = sid[12] | d2;
				break;

			case 29: // ATTACK 3
				if (!isMidi) {
					sendCC(22, value);
				}
				a4 = value;
				value = scale4bit(value);
				ledNumber(value);
				a3 = value;
				sid[19] = 255 & a3 << 4;
				sid[19] = sid[19] | d3;
				break;

			case 15: // DECAY 1
				if (!isMidi) {
					sendCC(7, value);
				}
				value = scale4bit(value);
				ledNumber(value);
				d1 = value;
				sid[5] = 255 & a1 << 4;
				sid[5] = sid[5] | d1;
				if (pa) {
					a2 = a3 = a1;
					d2 = d3 = d1;
					r2 = r3 = r1;
					s2 = s3 = s1;
					sid[12] = 255 & a2 << 4;
					sid[12] = sid[12] | d2;
					sid[19] = 255 & a3 << 4;
					sid[19] = sid[19] | d3;
				}
				break;

			case 25: // DECAY 2
				if (!isMidi) {
					sendCC(15, value);
				}
				value = scale4bit(value);
				ledNumber(value);
				d2 = value;
				sid[12] = 255 & a2 << 4;
				sid[12] = sid[12] | d2;
				break;

			case 18: // DECAY 3
				if (!isMidi) {
					sendCC(23, value);
				}
				d4 = value;
				value = scale4bit(value);
				ledNumber(value);
				d3 = value;
				sid[19] = 255 & a3 << 4;
				sid[19] = sid[19] | d3;
				break;

			case 13: // SUSTAIN 1
				if (!isMidi) {
					sendCC(8, value);
				}
				value = scale4bit(value);
				ledNumber(value);
				s1 = value;
				if (pa) {
					a2 = a3 = a1;
					d2 = d3 = d1;
					r2 = r3 = r1;
					s2 = s3 = s1;
					sid[13] = 255 & s2 << 4;
					sid[13] = sid[13] | r2;
					sid[20] = 255 & s3 << 4;
					sid[20] = sid[20] | r3;
				}
				sid[6] = 255 & s1 << 4;
				sid[6] = sid[6] | r1;
				break;

			case 23: // SUSTAIN 2
				if (!isMidi) {
					sendCC(16, value);
				}
				value = scale4bit(value);
				ledNumber(value);
				s2 = value;
				sid[13] = 255 & s2 << 4;
				sid[13] = sid[13] | r2;
				break;

			case 28: // SUSTAIN 3
				if (!isMidi) {
					sendCC(24, value);
				}
				s4 = value >> 2;
				value = scale4bit(value);
				ledNumber(value);
				s3 = value;
				sid[20] = 255 & s3 << 4;
				sid[20] = sid[20] | r3;
				break;

			case 16: // RELEASE 1
				if (!isMidi) {
					sendCC(9, value);
				}
				value = scale4bit(value);
				ledNumber(value);
				r1 = value;
				if (pa) {
					a2 = a3 = a1;
					d2 = d3 = d1;
					r2 = r3 = r1;
					s2 = s3 = s1;
					sid[13] = 255 & s2 << 4;
					sid[13] = sid[13] | r2;
					sid[20] = 255 & s3 << 4;
					sid[20] = sid[20] | r3;
				}
				sid[6] = 255 & s1 << 4;
				sid[6] = sid[6] | r1;
				break;

			case 20: // RELEASE 2
				if (!isMidi) {
					sendCC(17, value);
				}
				value = scale4bit(value);
				ledNumber(value);
				r2 = value;
				sid[13] = 255 & s2 << 4;
				sid[13] = sid[13] | r2;
				break;

			case 3: // RELEASE 3
				if (!isMidi) {
					sendCC(25, value);
				}
				r4 = value;

				value = scale4bit(value);
				ledNumber(value);
				r3 = value;
				sid[20] = 255 & s3 << 4;
				sid[20] = sid[20] | r3;
				break;

			case 11:
				if (!isMidi) {
					sendCC(26, value);
					if (sync) {
						ledNumber((value >> 7) + 1);
					} else {
						ledNumber(scale100(value));
					}

					lastMovedPot(9);
				}
				lfoClockSpeedPending[0] = 1 + (value >> 7);
				lfoSpeedBase[0] = value * 1.3;
				setLfo(0);
				break; // LFO RATE 1

			case 12:
				if (!isMidi) {
					sendCC(27, value);
					ledNumber(scale100(value));
					lastMovedPot(10);
				}
				lfoDepthBase[0] = value;
				setLfo(0);
				break; // LFO DEPTH 1

			case 10:
				if (!isMidi) {
					sendCC(28, value);

					if (sync) {
						ledNumber((value >> 7) + 1);
					} else {
						ledNumber(scale100(value));
					}

					lastMovedPot(11);
				}
				lfoClockSpeedPending[1] = 1 + (value >> 7);
				lfoSpeedBase[1] = value * 1.3;
				setLfo(1);
				break; // LFO RATE 2

			case 9:
				if (!isMidi) {
					sendCC(29, value);
					ledNumber(scale100(value));
					lastMovedPot(12);
				}
				lfoDepthBase[1] = value;
				setLfo(1);
				break; // LFO DEPTH 2

			case 36:
				if (!isMidi) {
					sendCC(30, value);

					if (sync) {
						ledNumber((value >> 7) + 1);
					} else {
						ledNumber(scale100(value));
					}

					lastMovedPot(13);
				}
				lfoClockSpeedPending[2] = 1 + (value >> 7);
				lfoSpeedBase[2] = value * 1.3;
				setLfo(2);
				break; // LFO RATE 3

			case 2:
				if (!isMidi) {
					sendCC(31, value);
					lastMovedPot(14);
					ledNumber(scale100(value));
				}
				lfoDepthBase[2] = value;
				setLfo(2);
				break; // LFO DEPTH 3

			case 8:
				if (!isMidi) {
					sendCC(59, value);
					ledNumber(scale100(value));
					lastMovedPot(15);
				}
				cutBase = value;
				break; // CUTOFF

			case 0:
				if (!isMidi) {
					sendCC(33, value);
					ledNumber(resBase);
					lastMovedPot(16);
				}
				resBase = scale4bit(value);
				break; // RESONANCE

			case 7:
				if (!isMidi) {
					sendCC(34, value);
					lastMovedPot(17);
				}
				if (held > 1) {
					arpStepBase = value >> 2;
				}
				break; // ARP SCRUB

			case 41:
				if (!isMidi) {
					sendCC(35, value);
					lastMovedPot(18);
					ledNumber(scale100(value - 10));
				}

				arpSpeedBase = (1023 - value) >> 2;
				if (arpSpeedBase << 4 > 4000) {
					arping = false;
				} else {
					arping = true;
				}

				arpRate = arpDivisions[arpSpeedBase >> 5];

				break; // ARP RATE

			case 32:
				if (!isMidi) {
					sendCC(36, value);
					ledNumber(arpRange + 1);
				}
				lastMovedPot(19);
				arpRangeBase = value >> 8;
				break; // ARP RANGE
		}
	}
}
