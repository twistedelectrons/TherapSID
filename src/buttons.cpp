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

void buttChanged(byte number, bool value) {
	if (!value) {
		// ledNumber(number);
		//  PRESSED
		switch (number) {
			case RECT1:
				if (!filterModeHeld) {
					sidShape(0, 1, !bitRead(sid[4], 6));
					shape1Pressed = true;
					paraShape();
					sendMidiButt(37, bitRead(sid[4], 6));
				} else {
					filterEnabled[0] = !filterEnabled[0];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(0);
				break;
			case TRI1:
				if (!filterModeHeld) {
					sidShape(0, 2, !bitRead(sid[4], 4));
					shape1Pressed = true;
					paraShape();
					sendMidiButt(38, bitRead(sid[4], 4));
				} else {
					filterEnabled[0] = !filterEnabled[0];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(0);
				break;
			case SAW1:
				if (!filterModeHeld) {
					sidShape(0, 3, !bitRead(sid[4], 5));
					shape1Pressed = true;
					paraShape();
					sendMidiButt(39, bitRead(sid[4], 5));
				} else {
					filterEnabled[0] = !filterEnabled[0];
					showFilterAssigns();
				assignmentChanged = true;
				}
				setFilterBit(0);
				break;
			case NOISE1:
				if (!filterModeHeld) {
					sidShape(0, 4, !bitRead(sid[4], 7));
					shape1Pressed = true;
					paraShape();
					sendMidiButt(40, bitRead(sid[4], 7));
				} else {
					filterEnabled[0] = !filterEnabled[0];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(0);
				break;

			case RECT2:
				if (!filterModeHeld) {
					sidShape(1, 1, !bitRead(sid[11], 6));
					sendMidiButt(41, bitRead(sid[11], 6));
				} else {
					filterEnabled[1] = !filterEnabled[1];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(1);
				break;
			case TRI2:
				if (!filterModeHeld) {
					sidShape(1, 2, !bitRead(sid[11], 4));
					sendMidiButt(42, bitRead(sid[11], 4));
				} else {
					filterEnabled[1] = !filterEnabled[1];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(1);
				break;
			case SAW2:
				if (!filterModeHeld) {
				sidShape(1, 3, !bitRead(sid[11], 5));
					sendMidiButt(43, bitRead(sid[11], 5));
				} else {
					filterEnabled[1] = !filterEnabled[1];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(1);
				break;
			case NOISE2:
				if (!filterModeHeld) {
					sidShape(1, 4, !bitRead(sid[11], 7));
					sendMidiButt(44, bitRead(sid[11], 7));
				} else {
					filterEnabled[1] = !filterEnabled[1];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(1);
				break;

			case RECT3:
				if (!filterModeHeld) {
					sidShape(2, 1, !bitRead(sid[18], 6));
					sendMidiButt(45, bitRead(sid[18], 6));
				} else {
					filterEnabled[2] = !filterEnabled[2];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(2);
				break;
			case TRI3:
				if (!filterModeHeld) {
					sidShape(2, 2, !bitRead(sid[18], 4));
					sendMidiButt(46, bitRead(sid[18], 4));
				} else {
					filterEnabled[2] = !filterEnabled[2];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(2);
				break;
			case SAW3:
				if (!filterModeHeld) {
					sidShape(2, 3, !bitRead(sid[18], 5));
					sendMidiButt(47, bitRead(sid[18], 5));
				} else {
					filterEnabled[2] = !filterEnabled[2];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(2);
				break;
			case NOISE3:
				if (!filterModeHeld) {
					sidShape(2, 4, !bitRead(sid[18], 7));
				sendMidiButt(48, bitRead(sid[18], 7));
				} else {
					filterEnabled[2] = !filterEnabled[2];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(2);
				break;

			case SYNC1:
				bitWrite(sid[4], 1, !bitRead(sid[4], 1));
				ledSet(16, bitRead(sid[4], 1));
				sendMidiButt(49, bitRead(sid[4], 1));
				break;
			case RING1:
				bitWrite(sid[4], 2, !bitRead(sid[4], 2));
				ledSet(17, bitRead(sid[4], 2));
				sendMidiButt(50, bitRead(sid[4], 2));
				break;

			case SYNC2:
				bitWrite(sid[11], 1, !bitRead(sid[11], 1));
				ledSet(18, bitRead(sid[11], 1));
				sendMidiButt(51, bitRead(sid[11], 1));
				break;
			case RING2:
				bitWrite(sid[11], 2, !bitRead(sid[11], 2));
				ledSet(19, bitRead(sid[11], 2));
				sendMidiButt(52, bitRead(sid[11], 2));
				break;

			case SYNC3:
				bitWrite(sid[18], 1, !bitRead(sid[18], 1));
				ledSet(20, bitRead(sid[18], 1));
				sendMidiButt(53, bitRead(sid[18], 1));
				break;
			case RING3:
				bitWrite(sid[18], 2, !bitRead(sid[18], 2));
				ledSet(21, bitRead(sid[18], 2));
				sendMidiButt(54, bitRead(sid[18], 2));
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
				showLfo();
				break;
			case LFO_SAW:
				if (lfoShape[selectedLfo] != 2) {
					lfoShape[selectedLfo] = 2;
				} else {
					lfoShape[selectedLfo] = 0;
				}
				showLfo();
				break;
			case LFO_TRI:
				if (lfoShape[selectedLfo] != 3) {
					lfoShape[selectedLfo] = 3;
				} else {
					lfoShape[selectedLfo] = 0;
				}
				showLfo();
				break;
			case LFO_NOISE:
				if (lfoShape[selectedLfo] != 4) {
					lfoShape[selectedLfo] = 4;
				} else {
					lfoShape[selectedLfo] = 0;
				}
				showLfo();
				break;
			case LFO_ENV3:
				if (lfoShape[selectedLfo] != 5) {
					lfoShape[selectedLfo] = 5;
				} else {
					lfoShape[selectedLfo] = 0;
				}
				showLfo();

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
				showLfo();
				break;
			case LOOP:
				looping[selectedLfo] = !looping[selectedLfo];
				showLfo();
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
