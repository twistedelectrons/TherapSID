#include "globals.h"
#include "leds.h"
#include "preset.h"
#include "sid.h"
#include "paraphonic.h"
#include "midi.h"
#include "lfo.h"
#include "arp.h"

static bool saveEngaged;
static byte midiSetup = 0;
static bool arpModeHeld;
static bool assignmentChanged;

enum Button {
	RECT1 = 2,
	TRI1 = 12,
	SAW1 = 4,
	NOISE1 = 8,
	RECT2 = 0,
	TRI2 = 10,
	SAW2 = 6,
	NOISE2 = 14,
	RECT3 = 1,
	TRI3 = 9,
	SAW3 = 5,
	NOISE3 = 13,
	
	SYNC1 = 11,
	RING1 = 3,
	SYNC2 = 16,
	RING2 = 20,
	SYNC3 = 28,
	RING3 = 24,

	LFO_CHAIN1 = 15,
	LFO_CHAIN2 = 27,
	LFO_CHAIN3 = 21,

	LFO_RECT = 30,
	LFO_SAW = 18,
	LFO_TRI = 26,
	LFO_NOISE = 22,
	LFO_ENV3 = 31,

	ARP_MODE = 32,
	RETRIG = 29,
	LOOP = 17,

	PRESET_UP = 19,
	PRESET_DOWN = 25,
	PRESET_RESET = 23,

	FILTER_MODE = 7
};

static void panic(){} // FIXME

void shapeButtPressed(uint8_t voice, PresetVoice::Shape shape) {
	if (voice >= 3) panic();
	Preset my_preset; // FIXME

	if (!filterModeHeld) {
		my_preset.voice[voice].toggle_shape(shape);
		// FIXME this needs deduplication of non-updates.
		sendMidiButt(37 + 4*voice + 0, my_preset.voice[voice].control & PresetVoice::PULSE);
		sendMidiButt(37 + 4*voice + 1, my_preset.voice[voice].control & PresetVoice::TRI);
		sendMidiButt(37 + 4*voice + 2, my_preset.voice[voice].control & PresetVoice::SAW);
		sendMidiButt(37 + 4*voice + 3, my_preset.voice[voice].control & PresetVoice::NOISE);

		if (voice == 0) {
			shape1Pressed = true;
		}
	}
	else {
		my_preset.voice[voice].filter_enabled ^= 1;
		showFilterAssigns(); // FIXME remove
		assignmentChanged = true;
	}
}

void buttChanged(byte number, bool value) {
	Preset my_preset; // FIXME
	if (!value) {
		// ledNumber(number);
		//  PRESSED
		switch (number) {
			case RECT1: shapeButtPressed(0, PresetVoice::PULSE); break;
			case TRI1: shapeButtPressed(0, PresetVoice::TRI); break;
			case SAW1: shapeButtPressed(0, PresetVoice::SAW); break;
			case NOISE1: shapeButtPressed(0, PresetVoice::NOISE); break;
			case RECT2: shapeButtPressed(1, PresetVoice::PULSE); break;
			case TRI2: shapeButtPressed(1, PresetVoice::TRI); break;
			case SAW2: shapeButtPressed(1, PresetVoice::SAW); break;
			case NOISE2: shapeButtPressed(1, PresetVoice::NOISE); break;
			case RECT3: shapeButtPressed(2, PresetVoice::PULSE); break;
			case TRI3: shapeButtPressed(2, PresetVoice::TRI); break;
			case SAW3: shapeButtPressed(2, PresetVoice::SAW); break;
			case NOISE3: shapeButtPressed(2, PresetVoice::NOISE); break;

			case SYNC1:
				my_preset.voice[0].control ^= 2;
				sendMidiButt(49, my_preset.voice[0].control & 2);
				break;
			case RING1:
				my_preset.voice[0].control ^= 4;
				sendMidiButt(50, my_preset.voice[0].control & 4);
				break;

			case SYNC2:
				my_preset.voice[1].control ^= 2;
				sendMidiButt(51, my_preset.voice[1].control & 2);
				break;
			case RING2:
				my_preset.voice[1].control ^= 4;
				sendMidiButt(52, my_preset.voice[1].control & 4);
				break;

			case SYNC3:
				my_preset.voice[2].control ^= 2;
				sendMidiButt(53, my_preset.voice[2].control & 2);
				break;
			case RING3:
				my_preset.voice[2].control ^= 4;
				sendMidiButt(54, my_preset.voice[2].control & 4);
				break;

			case LFO_CHAIN1:
				lfoButtPressed = true;
				selectedLfo = 0;
				if ((lastPot != 9) && (lastPot != 10)) {
					chain();
				}
				break;
			case LFO_CHAIN2:
				lfoButtPressed = true;
				selectedLfo = 1;
				if ((lastPot != 11) && (lastPot != 12)) {
					chain();
				}
				break;
			case LFO_CHAIN3:
				lfoButtPressed = true;
				selectedLfo = 2;
				if ((lastPot != 13) && (lastPot != 14)) {
					chain();
				}
				break;

			case LFO_RECT:
				if (lfoShape[selectedLfo] != 1) {
					lfoShape[selectedLfo] = 1;
				} else {
					lfoShape[selectedLfo] = 0;
				}
				break;
			case LFO_SAW:
				if (lfoShape[selectedLfo] != 2) {
					lfoShape[selectedLfo] = 2;
				} else {
					lfoShape[selectedLfo] = 0;
				}
				break;
			case LFO_TRI:
				if (lfoShape[selectedLfo] != 3) {
					lfoShape[selectedLfo] = 3;
				} else {
					lfoShape[selectedLfo] = 0;
				}
				break;
			case LFO_NOISE:
				if (lfoShape[selectedLfo] != 4) {
					lfoShape[selectedLfo] = 4;
				} else {
					lfoShape[selectedLfo] = 0;
				}
				break;
			case LFO_ENV3:
				if (lfoShape[selectedLfo] != 5) {
					lfoShape[selectedLfo] = 5;
				} else {
					lfoShape[selectedLfo] = 0;
				}

				break;

			case ARP_MODE:
				arpModeHeld = true;
				if (midiSetup) {
					midiSetup = 3;
					digit(99, 0);
					digit(99, 1);
				}
				break;

			case RETRIG:
				retrig[selectedLfo] = !retrig[selectedLfo];
				break;
			case LOOP:
				looping[selectedLfo] = !looping[selectedLfo];
				break;

			case PRESET_UP:
				if (midiSetup > 0) {
					if ((midiSetup == 1) && (masterChannel < 16)) {
						masterChannel++;
						saveChannels();
						ledNumber(masterChannel);
					}
					if ((midiSetup == 2) && (masterChannelOut < 16)) {
						masterChannelOut++;
						saveChannels();
						ledNumber(masterChannelOut);
					}
				} else {
					if (arpModeHeld) {
						midiSetup = 1;
						ledNumber(masterChannel);
					} else {
						saveModeTimer = 0;
						presetUp = true;
						if (presetDown) {
							if (!saveMode) {
								saveMode = true;
								saveBounce = 1600;
							} else {
								saveBounce = 1600;
								save();
							}
						}
					}
				}
				break;

			case PRESET_DOWN:

				if (midiSetup > 0) {
					if ((midiSetup == 1) && (masterChannel > 1)) {
						masterChannel--;
						saveChannels();
						ledNumber(masterChannel);
					}
					if ((midiSetup == 2) && (masterChannelOut > 1)) {
						masterChannelOut--;
						saveChannels();
						ledNumber(masterChannelOut);
					}
				} else {
					if (arpModeHeld) {
						midiSetup = 2;
						ledNumber(masterChannelOut);
					} else {
						saveModeTimer = 0;
						presetDown = true;
						if (presetUp) {
							if (!saveMode) {
								saveMode = true;
								saveBounce = 1600;
							} else {
								saveBounce = 1600;
								save();
							}
						}
					}
				}
				break;

			case PRESET_RESET:
				resetDown = true;
				load(1);
				resetDownTimer = 0;
				break;

			case FILTER_MODE:
				assignmentChanged = false;
				filterModeHeld = true;
				showFilterAssigns();

				break;
		}

		my_preset.set_leds();
	} else {
		switch (number) {
			case RECT1:
			case TRI1:
			case SAW1:
			case NOISE1:
				shape1Pressed = false;
				shape1PressedTimer = 0;
				break;

			case LFO_CHAIN1:
			case LFO_CHAIN2:
			case LFO_CHAIN3:
				lfoButtPressed = false;
				lfoButtTimer = 0;
				break;
			
			case ARP_MODE:
				arpModeHeld = false;
				arpModeCounter = 0;
				if (!midiSetup) {
					if (!pa) {
						arpMode++;
						if (arpMode > 4) {
							arpMode = 0;
							sendNoteOff(lastNote, 127, masterChannelOut);
						}
						showArp();
					}
				} else if (midiSetup == 3) {
					midiSetup = 0;
					digit(0, 99);
					digit(1, 99);
				}
				break;

			case PRESET_UP:
				if (!midiSetup) {
					presetUp = false;
					presetScrollSpeed = 10000;
					if ((!saveBounce) && (!loadTimer) && (!scrolled)) {
						preset++;
						if (preset > 99) {
							preset = 1;
						}
						ledNumber(preset);
					} else {
						saveEngaged = false;
					}
					saveModeTimer = 0;
				}
				scrolled = false;
				break;

			case PRESET_DOWN:
				if (!midiSetup) {
					presetDown = false;
					presetScrollSpeed = 10000;
					if ((!saveBounce) && (!loadTimer) && (!scrolled)) {
						preset--;
						if (preset < 1) {
							preset = 99;
						}
						ledNumber(preset);
					} else {
						saveEngaged = false;
					}
					saveModeTimer = 0;
				}
				scrolled = false;
				break;

			case PRESET_RESET:
				resetDown = false;
				break;

			case FILTER_MODE:
				if (!fatChanged) {
					unShowFilterAssigns();

					if (!assignmentChanged) {
						// TODO ugh
						filterMode = static_cast<FilterMode>(
							(static_cast<int>(filterMode) + 1) % 5
						);
						updateFilter();
						sendCC(55, map((int)filterMode, 0, 4, 0, 1023));
					}
				} else {
					fatChanged = false;
				}
				filterModeHeld = false;
				break;
		}
	}
}
