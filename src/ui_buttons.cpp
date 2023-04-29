#include "globals.h"
#include "preset.h"
#include "midi.h"
#include "arp.h"
#include "util.hpp"
#include "ui_vars.h"
#include "ui_leds.h"
#include "ui_controller.h"

const byte presetChords[16][6] = {
    {0, 4, 7, 10, 14, 17}, // C Major 7
    {0, 3, 7, 10, 14, 17}, // C Minor 7
    {0, 4, 7, 11, 14, 17}, // C Dominant 7
    {0, 4, 7, 11, 14, 18}, // C Major 7(#11)
    {0, 4, 7, 10, 13, 17}, // C6
    {0, 4, 7, 9, 14, 17},  // C Minor Major 7
    {0, 2, 4, 7, 9, 11},   // Cm6
    {0, 3, 6, 10, 14, 17}, // C Minor 7(b5)
    {0, 5, 7, 10, 14, 17}, // C9
    {0, 4, 7, 10, 14, 19}, // C Major 9
    {0, 4, 7, 10, 14, 15}, // Cadd9
    {0, 4, 7, 11, 14, 21}, // C13
    {0, 2, 4, 7, 9, 10},   // Cmadd9
    {0, 4, 7, 11, 15, 18}, // C7(#9)
    {0, 4, 7, 10, 14, 21}, // C6/9
    {0, 3, 7, 9, 14, 17}   // Cm(Maj7)/C Minor Major 7
};

byte presetChordNumber;

static bool saveEngaged;
static bool noArpAction;
static bool
    presetDownDisabled; // temporarily disable button release action if we just entered savemode (individual debounce)
static bool
    presetUpDisabled; // temporarily disable button release action if we just entered savemode (individual debounce)
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

static void shapeButtPressed(uint8_t voice, PresetVoice::Shape shape) {
	if (voice >= 3)
		panic(2, 1);

	if (!ui_state.filterModeHeld) {
		preset_data.voice[voice].toggle_shape(shape);
		sendMidiButt(37 + 4 * voice + 0, preset_data.voice[voice].reg_control & PresetVoice::PULSE);
		sendMidiButt(37 + 4 * voice + 1, preset_data.voice[voice].reg_control & PresetVoice::TRI);
		sendMidiButt(37 + 4 * voice + 2, preset_data.voice[voice].reg_control & PresetVoice::SAW);
		sendMidiButt(37 + 4 * voice + 3, preset_data.voice[voice].reg_control & PresetVoice::NOISE);

		if (voice == 0) {
			shape1Pressed = true;
		}
	} else {
		preset_data.voice[voice].filter_enabled ^= 1;
		filterAssignmentChanged = true;
	}
}

void buttChanged(byte number, bool value) {
	if (!value) {
		//  PRESSED
		switch (number) {
			case RECT1:
				shapeButtPressed(0, PresetVoice::PULSE);
				break;
			case TRI1:
				shapeButtPressed(0, PresetVoice::TRI);
				break;
			case SAW1:
				shapeButtPressed(0, PresetVoice::SAW);
				break;
			case NOISE1:
				shapeButtPressed(0, PresetVoice::NOISE);
				break;
			case RECT2:
				shapeButtPressed(1, PresetVoice::PULSE);
				break;
			case TRI2:
				shapeButtPressed(1, PresetVoice::TRI);
				break;
			case SAW2:
				shapeButtPressed(1, PresetVoice::SAW);
				break;
			case NOISE2:
				shapeButtPressed(1, PresetVoice::NOISE);
				break;
			case RECT3:
				shapeButtPressed(2, PresetVoice::PULSE);
				break;
			case TRI3:
				shapeButtPressed(2, PresetVoice::TRI);
				break;
			case SAW3:
				shapeButtPressed(2, PresetVoice::SAW);
				break;
			case NOISE3:
				shapeButtPressed(2, PresetVoice::NOISE);
				break;

			case SYNC1:
				preset_data.voice[0].reg_control ^= 2;
				sendMidiButt(49, preset_data.voice[0].reg_control & 2);
				break;
			case RING1:
				preset_data.voice[0].reg_control ^= 4;
				sendMidiButt(50, preset_data.voice[0].reg_control & 4);
				break;

			case SYNC2:
				preset_data.voice[1].reg_control ^= 2;
				sendMidiButt(51, preset_data.voice[1].reg_control & 2);
				break;
			case RING2:
				preset_data.voice[1].reg_control ^= 4;
				sendMidiButt(52, preset_data.voice[1].reg_control & 4);
				break;

			case SYNC3:
				preset_data.voice[2].reg_control ^= 2;
				sendMidiButt(53, preset_data.voice[2].reg_control & 2);
				break;
			case RING3:
				preset_data.voice[2].reg_control ^= 4;
				sendMidiButt(54, preset_data.voice[2].reg_control & 4);
				break;

			case LFO_CHAIN1:
				lfoButtPressed = true;
				ui_state.selectedLfo = 0;
				if (ui_state.lastPot != 9 && ui_state.lastPot != 10 && ui_state.lastPot != 20) {
					preset_data.lfo[ui_state.selectedLfo].mapping[ui_state.lastPot] =
					    !preset_data.lfo[ui_state.selectedLfo].mapping[ui_state.lastPot];
				}
				break;
			case LFO_CHAIN2:
				lfoButtPressed = true;
				ui_state.selectedLfo = 1;
				if (ui_state.lastPot != 11 && ui_state.lastPot != 12 && ui_state.lastPot != 20) {
					preset_data.lfo[ui_state.selectedLfo].mapping[ui_state.lastPot] =
					    !preset_data.lfo[ui_state.selectedLfo].mapping[ui_state.lastPot];
				}
				break;
			case LFO_CHAIN3:
				lfoButtPressed = true;
				ui_state.selectedLfo = 2;
				if (ui_state.lastPot != 13 && ui_state.lastPot != 14 && ui_state.lastPot != 20) {
					preset_data.lfo[ui_state.selectedLfo].mapping[ui_state.lastPot] =
					    !preset_data.lfo[ui_state.selectedLfo].mapping[ui_state.lastPot];
				}
				break;

			case LFO_RECT:
				if (preset_data.lfo[ui_state.selectedLfo].shape != 1) {
					preset_data.lfo[ui_state.selectedLfo].shape = 1;
				} else {
					preset_data.lfo[ui_state.selectedLfo].shape = 0;
				}
				break;
			case LFO_SAW:
				if (preset_data.lfo[ui_state.selectedLfo].shape != 2) {
					preset_data.lfo[ui_state.selectedLfo].shape = 2;
				} else {
					preset_data.lfo[ui_state.selectedLfo].shape = 0;
				}
				break;
			case LFO_TRI:
				if (preset_data.lfo[ui_state.selectedLfo].shape != 3) {
					preset_data.lfo[ui_state.selectedLfo].shape = 3;
				} else {
					preset_data.lfo[ui_state.selectedLfo].shape = 0;
				}
				break;
			case LFO_NOISE:
				if (preset_data.lfo[ui_state.selectedLfo].shape != 4) {
					preset_data.lfo[ui_state.selectedLfo].shape = 4;
				} else {
					preset_data.lfo[ui_state.selectedLfo].shape = 0;
				}
				break;
			case LFO_ENV3:
				if (preset_data.lfo[ui_state.selectedLfo].shape != 5) {
					preset_data.lfo[ui_state.selectedLfo].shape = 5;
				} else {
					preset_data.lfo[ui_state.selectedLfo].shape = 0;
				}

				break;

			case ARP_MODE:
				noArpAction = true;
				arpModeHeld = true;
				if (ui_state.midiSetup) {
					ui_state.midiSetup = 3;
				}
				break;

			case RETRIG:
				if (arpModeHeld) {
					noArpAction = false;
					autoChordChanged = true;
					if (autoChord) {
						for (int i = 128; i > 0; i--) {
							chordKeys[i] = 0;
						}
						presetChordNumber++;
						if (presetChordNumber > 15)
							presetChordNumber = 0;
						clearAutoChord = true;
					} else {
						autoChord = true;
						bool noNotes = true; // use preset chords if no keys are held
						for (int i = 128; i > 0; i--) {
							chordKeys[i] = heldKeys[i];
							if (heldKeys[i]) {
								chordRoot = i;
								noNotes = false;
							}
						}
						if (noNotes) {
							chordRoot = 0;
							for (int i = 0; i < 6; i++) {
								if (presetChordNumber % 2 == 0) { // alternate between 3 and 6 note chords
									if (i < 3) {
										chordKeys[presetChords[presetChordNumber][i]] = 1;
									}
								} else {
									chordKeys[presetChords[presetChordNumber][i]] = 1;
								}
							}
						}
					}
				} else {
					preset_data.lfo[ui_state.selectedLfo].retrig = !preset_data.lfo[ui_state.selectedLfo].retrig;
				}
				break;
			case LOOP:
				preset_data.lfo[ui_state.selectedLfo].looping = !preset_data.lfo[ui_state.selectedLfo].looping;
				break;

			case PRESET_UP:
				if (ui_state.midiSetup > 0) {
					if ((ui_state.midiSetup == 1) && (masterChannel < 16)) {
						masterChannel++;
						saveChannels();
					}
					if ((ui_state.midiSetup == 2) && (masterChannelOut < 16)) {
						masterChannelOut++;
						saveChannels();
					}
				} else {
					presetScrollTimer = 0;
					presetScrollSpeed = 26000;
					if (arpModeHeld) {
						ui_state.midiSetup = 1;
					} else {
						presetUp = true;
						if (presetDown) {
							if (!ui_state.saveMode) {
								ui_state.saveMode = true;
								presetDownDisabled = presetUpDisabled = true;
							} else {
								save();
								ui_state.saveMode = false;
								presetDownDisabled = presetUpDisabled = true;
							}
						}
					}
				}
				break;

			case PRESET_DOWN:

				if (ui_state.midiSetup > 0) {
					if ((ui_state.midiSetup == 1) && (masterChannel > 1)) {
						masterChannel--;
						saveChannels();
					}
					if ((ui_state.midiSetup == 2) && (masterChannelOut > 1)) {
						masterChannelOut--;
						saveChannels();
					}
				} else {
					if (arpModeHeld) {
						ui_state.midiSetup = 2;
					} else {
						presetScrollSpeed = 26000;
						presetScrollTimer = 0;
						presetDown = true;
						if (presetUp) {
							if (!ui_state.saveMode) {
								ui_state.saveMode = true;
								presetDownDisabled = presetUpDisabled = true;
							} else {
								save();
								ui_state.saveMode = false;
								presetDownDisabled = presetUpDisabled = true;
							}
						}
					}
				}
				break;

			case PRESET_RESET:
				resetDown = true;
				if (ui_state.saveMode) {
					ui_state.saveMode = false;
				} else {
					load(1);
					resetDownTimer = 0;
				}
				break;

			case FILTER_MODE:
				fatChanged = false;
				filterAssignmentChanged = false;
				ui_state.filterModeHeld = true;
				volumeChanged = false;
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
				if (ui_state.filterModeHeld) {
					fatChanged = true;
					if (preset_data.paraphonic)
						preset_data.fat_mode = static_cast<FatMode>(((int)preset_data.fat_mode + 1) % 6);
					else
						preset_data.fat_mode = static_cast<FatMode>(((int)preset_data.fat_mode + 1) % 5);

				} else {
					if (noArpAction) {
						if (!ui_state.midiSetup) {
							if (!preset_data.paraphonic) {
								preset_data.arp_mode++;
								if (preset_data.arp_mode > 4) {
									preset_data.arp_mode = 0;
									sendNoteOff(lastNote, 127, masterChannelOut);
								}
								reset_arp();
							}
						} else if (ui_state.midiSetup == 3) {
							ui_state.midiSetup = 0;
						}
					}
				}

				arpModeHeld = false;
				break;

			case PRESET_UP:
				if (!ui_state.midiSetup) {

					presetUp = false;
					if (!presetUpDisabled) {
						if ((!presetDown) && (!saveBounce) && (!loadTimer) && (!scrolled)) {
							preset++;
							if (preset > PRESET_NUMBER_MAX) {
								preset = PRESET_NUMBER_MIN;
							}
						} else {
							saveEngaged = false;
						}
					} else {
						presetUpDisabled = false;
					}
				}
				scrolled = false;
				break;

			case PRESET_DOWN:
				if (!ui_state.midiSetup) {

					presetDown = false;
					if (!presetDownDisabled) {
						if ((!presetUp) && (!saveBounce) && (!loadTimer) && (!scrolled)) {
							preset--;
							if (preset < PRESET_NUMBER_MIN) {
								preset = PRESET_NUMBER_MAX;
							}
						} else {
							saveEngaged = false;
						}
					} else {
						presetDownDisabled = false;
					}
					scrolled = false;
				}
				break;

			case PRESET_RESET:
				resetDown = false;
				break;

			case FILTER_MODE:
				if ((!filterAssignmentChanged) && (!fatChanged)) {
					preset_data.filter_mode =
					    static_cast<FilterMode>((static_cast<int>(preset_data.filter_mode) + 1) % 5);
					sendCC(55, map((int)preset_data.filter_mode, 0, 4, 0, 1023));
				}
				ui_state.filterModeHeld = false;

				break;
		}
	}
}
