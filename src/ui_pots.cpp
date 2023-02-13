#include "globals.h"
#include "ui_pots.h"
#include "midi.h"
#include "ui_vars.h"

static int arpDivisions[] = {1, 3, 6, 8, 12, 24, 32, 48};

static byte scale4bit(int input) { return (input >> 6); }

static byte octScale(int value) {
	value = map(value, 0, 1023, 0, 25);
	if (value > 24)
		value = 24;
	return (value);
}

void movedPot(byte number, int value, bool isMidi) {
	if (!ui_state.saveMode) {
		if (!isMidi) {
			ui_state.lastPot = 20; // unselect
		}
		switch (number) {
			case 4:
				if (value < 1) {
					value = 1;
				}
				preset_data.voice[0].pulsewidth_base = value << 1;
				if (!isMidi) {
					sendCC(2, value);
					ui_state.lastPot = 0;
				}
				break; // PW1
			case 24:
				if (value < 1) {
					value = 1;
				}
				preset_data.voice[1].pulsewidth_base = value << 1;
				if (!isMidi) {
					sendCC(10, value);
					ui_state.lastPot = 3;
				}
				break; // PW2
			case 30:
				if (value < 1) {
					value = 1;
				}
				preset_data.voice[2].pulsewidth_base = value << 1;
				if (!isMidi) {
					sendCC(18, value);
					ui_state.lastPot = 6;
				}
				break; // PW3

			case 6:
				preset_data.voice[0].tune_base = octScale(value);
				if (!isMidi) {
					sendCC(3, value);
					ui_state.lastPot = 1;
				}
				break; // TUNE1
			case 26:
				preset_data.voice[1].tune_base = octScale(value);
				if (!isMidi) {
					sendCC(11, value);
					ui_state.lastPot = 4;
				}
				break; // TUNE2
			case 21:
				preset_data.voice[2].tune_base = octScale(value);
				if (!isMidi) {
					sendCC(19, value);
					ui_state.lastPot = 7;
				}
				break; // TUNE3

			case 14:
				preset_data.voice[0].fine_base = value / 1023.f;
				if (!isMidi) {
					sendCC(4, value);
					ui_state.lastPot = 2;
				}
				break; // FINE1
			case 17:
				preset_data.voice[1].fine_base = value / 1023.f;
				if (!isMidi) {
					sendCC(12, value);
					ui_state.lastPot = 5;
				}
				break; // FINE2
			case 31:
				if (!isMidi) {
					sendCC(20, value);
					preset_data.voice[2].fine_base = value / 1023.f;
					ui_state.lastPot = 8;
				}
				break; // FINE3

			case 1:
				preset_data.voice[0].glide = value >> 4;
				if (!isMidi) {
					sendCC(5, value);
				}
				break; // GLIDE 1
			case 27:
				preset_data.voice[1].glide = value >> 4;
				if (!isMidi) {
					sendCC(13, value);
				}
				break; // GLIDE 2
			case 19:
				preset_data.voice[2].glide = value >> 4;
				if (!isMidi) {
					sendCC(21, value);
				}
				break; // GLIDE 3

			case 5: // ATTACK 1
				if (!isMidi) {
					sendCC(6, value);
				}
				value = scale4bit(value);
				preset_data.voice[0].attack = value;

				break;

			case 22: // ATTACK 2
				if (!isMidi) {
					sendCC(14, value);
				}
				value = scale4bit(value);
				preset_data.voice[1].attack = value;
				break;

			case 29: // ATTACK 3
				if (!isMidi) {
					sendCC(22, value);
				}
				a4 = value;
				value = scale4bit(value);
				preset_data.voice[2].attack = value;
				break;

			case 15: // DECAY 1
				if (!isMidi) {
					sendCC(7, value);
				}
				value = scale4bit(value);
				preset_data.voice[0].decay = value;
				break;

			case 25: // DECAY 2
				if (!isMidi) {
					sendCC(15, value);
				}
				value = scale4bit(value);
				preset_data.voice[1].decay = value;
				break;

			case 18: // DECAY 3
				if (!isMidi) {
					sendCC(23, value);
				}
				d4 = value;
				value = scale4bit(value);
				preset_data.voice[2].decay = value;
				break;

			case 13: // SUSTAIN 1
				if (!isMidi) {
					sendCC(8, value);
				}
				value = scale4bit(value);
				preset_data.voice[0].sustain = value;
				break;

			case 23: // SUSTAIN 2
				if (!isMidi) {
					sendCC(16, value);
				}
				value = scale4bit(value);
				preset_data.voice[1].sustain = value;
				break;

			case 28: // SUSTAIN 3
				if (!isMidi) {
					sendCC(24, value);
				}
				s4 = value >> 2;
				value = scale4bit(value);
				preset_data.voice[2].sustain = value;
				break;

			case 16: // RELEASE 1
				if (!isMidi) {
					sendCC(9, value);
				}
				value = scale4bit(value);
				preset_data.voice[0].release = value;
				break;

			case 20: // RELEASE 2
				if (!isMidi) {
					sendCC(17, value);
				}
				value = scale4bit(value);
				preset_data.voice[1].release = value;
				break;

			case 3: // RELEASE 3
				if (!isMidi) {
					sendCC(25, value);
				}
				r4 = value;

				value = scale4bit(value);
				preset_data.voice[2].release = value;
				break;

			case 11:
				if (!isMidi) {
					sendCC(26, value);
					ui_state.lastPot = 9;
				}
				lfoClockSpeedPending[0] = 1 + (value >> 7);
				preset_data.lfo[0].speed = value * 1.3;
				ui_state.selectedLfo = 0;
				break; // LFO RATE 1

			case 12:
				if (!isMidi) {
					sendCC(27, value);
					ui_state.lastPot = 10;
				}
				preset_data.lfo[0].depth = value;
				ui_state.selectedLfo = 0;
				break; // LFO DEPTH 1

			case 10:
				if (!isMidi) {
					sendCC(28, value);
					ui_state.lastPot = 11;
				}
				lfoClockSpeedPending[1] = 1 + (value >> 7);
				preset_data.lfo[1].speed = value * 1.3;
				ui_state.selectedLfo = 1;
				break; // LFO RATE 2

			case 9:
				if (!isMidi) {
					sendCC(29, value);
					ui_state.lastPot = 12;
				}
				preset_data.lfo[1].depth = value;
				ui_state.selectedLfo = 1;
				break; // LFO DEPTH 2

			case 36:
				if (!isMidi) {
					sendCC(30, value);
					ui_state.lastPot = 13;
				}
				lfoClockSpeedPending[2] = 1 + (value >> 7);
				preset_data.lfo[2].speed = value * 1.3;
				ui_state.selectedLfo = 2;
				break; // LFO RATE 3

			case 2:
				if (!isMidi) {
					sendCC(31, value);
					ui_state.lastPot = 14;
				}
				preset_data.lfo[2].depth = value;
				ui_state.selectedLfo = 2;
				break; // LFO DEPTH 3

			case 8:
				if (!isMidi) {
					sendCC(59, value);
					ui_state.lastPot = 15;
				}
				preset_data.cutoff = value;
				break; // CUTOFF

			case 0:
				if (!isMidi) {
					sendCC(33, value);
					ui_state.lastPot = 16;
				}
				preset_data.resonance_base = scale4bit(value);
				break; // RESONANCE

			case 7:
				if (!isMidi) {
					sendCC(34, value);
					ui_state.lastPot = 17;
				}
				if (voice_state.n_held_keys() > 1) { // TODO why >, not >=?
					arpStepBase = value >> 2;
				}
				break; // ARP SCRUB

			case 41:
				if (!isMidi) {
					sendCC(35, value);
					ui_state.lastPot = 18;
				}

				preset_data.arp_speed_base = (1023 - value) >> 2;
				preset_data.arp_rate = arpDivisions[preset_data.arp_speed_base >> 5];

				break; // ARP RATE

			case 32:
				if (!isMidi) {
					sendCC(36, value);
				}
				ui_state.lastPot = 19;
				preset_data.arp_range_base = value >> 8;
				break; // ARP RANGE
		}
	}
}
