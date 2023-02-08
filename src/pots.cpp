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
				if (value < 1) {
					value = 1;
				}
				preset_data.voice[0].pulsewidth_base = value << 1;
				if (!isMidi) {
					sendCC(2, value);
					ledNumber(scale100(value));
					lastMovedPot(0);
				}
				break; // PW1
			case 24:
				if (value < 1) {
					value = 1;
				}
				preset_data.voice[1].pulsewidth_base = value << 1;
				if (!isMidi) {
					sendCC(10, value);
					ledNumber(scale100(value));
					lastMovedPot(3);
				}
				break; // PW2
			case 30:
				if (value < 1) {
					value = 1;
				}
				preset_data.voice[2].pulsewidth_base = value << 1;
				if (!isMidi) {
					sendCC(18, value);
					ledNumber(scale100(value));
					lastMovedPot(6);
				}
				break; // PW3

			case 6:
				preset_data.voice[0].tune_base = octScale(value);
				if (!isMidi) {
					sendCC(3, value);
					ledNumber(preset_data.voice[0].tune_base - 12);
					lastMovedPot(1);
				}
				break; // TUNE1
			case 26:
				preset_data.voice[1].tune_base = octScale(value);
				if (!isMidi) {
					sendCC(11, value);
					ledNumber(preset_data.voice[1].tune_base - 12);
					lastMovedPot(4);
				}
				break; // TUNE2
			case 21:
				preset_data.voice[2].tune_base = octScale(value);
				if (!isMidi) {
					sendCC(19, value);
					ledNumber(preset_data.voice[2].tune_base - 12);
					lastMovedPot(7);
				}
				break; // TUNE3

			case 14:
				preset_data.voice[0].fine_base = value / 1023.f;
				if (!isMidi) {
					sendCC(4, value);
					ledNumber(scaleFine(value));
					lastMovedPot(2);
				}
				break; // FINE1
			case 17:
				preset_data.voice[1].fine_base = value / 1023.f;
				if (!isMidi) {
					sendCC(12, value);
					ledNumber(scaleFine(value));
					lastMovedPot(5);
				}
				break; // FINE2
			case 31:
				if (!isMidi) {
					sendCC(20, value);
					preset_data.voice[2].fine_base = value / 1023.f;
					ledNumber(scaleFine(value));
					lastMovedPot(8);
				}
				break; // FINE3

			case 1:
				preset_data.voice[0].glide = value >> 4;
				if (!isMidi) {
					sendCC(5, value);
					ledNumber(preset_data.voice[0].glide);
				}
				break; // GLIDE 1
			case 27:
				preset_data.voice[1].glide = value >> 4;
				if (!isMidi) {
					sendCC(13, value);
					ledNumber(preset_data.voice[1].glide);
				}
				break; // GLIDE 2
			case 19:
				preset_data.voice[2].glide = value >> 4;
				if (!isMidi) {
					sendCC(21, value);
					ledNumber(preset_data.voice[2].glide);
				}
				break; // GLIDE 3

			case 5: // ATTACK 1
				if (!isMidi) {
					sendCC(6, value);
				}
				value = scale4bit(value);
				ledNumber(value);
				preset_data.voice[0].attack = value;

				break;

			case 22: // ATTACK 2
				if (!isMidi) {
					sendCC(14, value);
				}
				value = scale4bit(value);
				ledNumber(value);
				preset_data.voice[1].attack = value;
				break;

			case 29: // ATTACK 3
				if (!isMidi) {
					sendCC(22, value);
				}
				a4 = value;
				value = scale4bit(value);
				ledNumber(value);
				preset_data.voice[2].attack = value;
				break;

			case 15: // DECAY 1
				if (!isMidi) {
					sendCC(7, value);
				}
				value = scale4bit(value);
				ledNumber(value);
				preset_data.voice[0].decay = value;
				break;

			case 25: // DECAY 2
				if (!isMidi) {
					sendCC(15, value);
				}
				value = scale4bit(value);
				ledNumber(value);
				preset_data.voice[1].decay = value;
				break;

			case 18: // DECAY 3
				if (!isMidi) {
					sendCC(23, value);
				}
				d4 = value;
				value = scale4bit(value);
				ledNumber(value);
				preset_data.voice[2].decay = value;
				break;

			case 13: // SUSTAIN 1
				if (!isMidi) {
					sendCC(8, value);
				}
				value = scale4bit(value);
				ledNumber(value);
				preset_data.voice[0].sustain = value;
				break;

			case 23: // SUSTAIN 2
				if (!isMidi) {
					sendCC(16, value);
				}
				value = scale4bit(value);
				ledNumber(value);
				preset_data.voice[1].sustain = value;
				break;

			case 28: // SUSTAIN 3
				if (!isMidi) {
					sendCC(24, value);
				}
				s4 = value >> 2;
				value = scale4bit(value);
				ledNumber(value);
				preset_data.voice[2].sustain = value;
				break;

			case 16: // RELEASE 1
				if (!isMidi) {
					sendCC(9, value);
				}
				value = scale4bit(value);
				ledNumber(value);
				preset_data.voice[0].release = value;
				break;

			case 20: // RELEASE 2
				if (!isMidi) {
					sendCC(17, value);
				}
				value = scale4bit(value);
				ledNumber(value);
				preset_data.voice[1].release = value;
				break;

			case 3: // RELEASE 3
				if (!isMidi) {
					sendCC(25, value);
				}
				r4 = value;

				value = scale4bit(value);
				ledNumber(value);
				preset_data.voice[2].release = value;
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
				preset_data.lfo[0].speed = value * 1.3;
				setLfo(0);
				break; // LFO RATE 1

			case 12:
				if (!isMidi) {
					sendCC(27, value);
					ledNumber(scale100(value));
					lastMovedPot(10);
				}
				preset_data.lfo[0].depth = value;
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
				preset_data.lfo[1].speed = value * 1.3;
				setLfo(1);
				break; // LFO RATE 2

			case 9:
				if (!isMidi) {
					sendCC(29, value);
					ledNumber(scale100(value));
					lastMovedPot(12);
				}
				preset_data.lfo[1].depth = value;
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
				preset_data.lfo[2].speed = value * 1.3;
				setLfo(2);
				break; // LFO RATE 3

			case 2:
				if (!isMidi) {
					sendCC(31, value);
					lastMovedPot(14);
					ledNumber(scale100(value));
				}
				preset_data.lfo[2].depth = value;
				setLfo(2);
				break; // LFO DEPTH 3

			case 8:
				if (!isMidi) {
					sendCC(59, value);
					ledNumber(scale100(value));
					lastMovedPot(15);
				}
				preset_data.cutoff = value;
				break; // CUTOFF

			case 0:
				if (!isMidi) {
					sendCC(33, value);
					ledNumber(preset_data.resonance_base);
					lastMovedPot(16);
				}
				preset_data.resonance_base = scale4bit(value);
				break; // RESONANCE

			case 7:
				if (!isMidi) {
					sendCC(34, value);
					lastMovedPot(17);
				}
				if (voice_state.n_held_keys() > 1) { // TODO why >, not >=?
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

				preset_data.arp_rate = arpDivisions[arpSpeedBase >> 5];

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
