#include "globals.h"
#include "preset.h"
#include "midi.h"
#include "arp.h"
#include "util.hpp"
#include "ui_controller.h"
#include "asid.h"

#include "ui_leds.h"

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
enum class Button {
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
	LFO_SAW = 26,
	LFO_TRI = 18,
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

// Retrieves a useful index for similar buttons, could be voice or similar
byte indexFromButton(Button button) {
	byte index;

	switch (button) {
		case Button::RECT2:
		case Button::TRI2:
		case Button::SAW2:
		case Button::NOISE2:
		case Button::SYNC2:
		case Button::RING2:
		case Button::LFO_CHAIN2:
		case Button::LFO_TRI:
			index = 1;
			break;

		case Button::RECT3:
		case Button::TRI3:
		case Button::SAW3:
		case Button::NOISE3:
		case Button::SYNC3:
		case Button::RING3:
		case Button::LFO_CHAIN3:
		case Button::LFO_SAW:
			index = 2;
			break;

		case Button::LFO_NOISE:
			index = 3;
			break;

		case Button::LFO_ENV3:
			index = 4;
			break;

		default:
			index = 0;
			break;
	}

	return index;
}

void updateWaveformAsid(byte chip, byte voice, bool all, WaveformState waveform) {
	if (asidState.overrideWaveform[chip][voice] == waveform) {
		asidState.overrideWaveform[chip][voice] = WaveformState::NOISE_ONLY;
	} else {
		asidState.overrideWaveform[chip][voice] = waveform;
	}
	asidState.muteChannel[chip][voice] = false;

	if (all) {
		// Copy from 0 to 1
		asidState.overrideWaveform[1][voice] = asidState.overrideWaveform[0][voice];
		asidState.muteChannel[1][voice] = false;
	}
}

void updateTriStateButtonAsid(byte chip, byte voice, bool all, OverrideState buttonState[][SIDVOICES]) {
	if (buttonState[chip][voice] == OverrideState::OFF) {
		buttonState[chip][voice] = OverrideState::ON;
	} else {
		buttonState[chip][voice] = static_cast<OverrideState>(static_cast<int>(buttonState[chip][voice]) + 1);
	}
	if (all) {
		// Copy from 0 to 1
		buttonState[1][voice] = buttonState[0][voice];
	}
}

void buttChangedAsid(Button button, bool value) {
	if (!value) {
		// Pressed
		byte index = indexFromButton(button);
		byte chip;
		bool indicateChange[] = {true, true};
		bool all = false;

		// If one pressed - execute it only on that chip
		// If both affected - execute 0 then copy to 1 (executing actions if needed)
		if ((asidState.selectedSids.all == 0) || (asidState.selectedSids.b.sid1 && asidState.selectedSids.b.sid2)) {
			// Both chips should be affected
			all = true;
			chip = 0;
		} else {
			// One chip selected
			if (asidState.selectedSids.b.sid1) {
				chip = 0;
				indicateChange[1] = false;
			} else {
				chip = 1;
				indicateChange[0] = false;
			}
		}

		switch (button) {
			case Button::RECT1:
			case Button::RECT2:
			case Button::RECT3:
				updateWaveformAsid(chip, index, all, WaveformState::RECT);
				break;

			case Button::TRI1:
			case Button::TRI2:
			case Button::TRI3:
				updateWaveformAsid(chip, index, all, WaveformState::TRI);
				break;

			case Button::SAW1:
			case Button::SAW2:
			case Button::SAW3:
				updateWaveformAsid(chip, index, all, WaveformState::SAW);
				break;

			case Button::NOISE1:
			case Button::NOISE2:
			case Button::NOISE3:
				asidState.muteChannel[chip][index] = !asidState.muteChannel[chip][index];
				if (all) {
					asidState.muteChannel[1][index] = asidState.muteChannel[0][index];
				}
				break;

			case Button::SYNC1:
			case Button::SYNC2:
			case Button::SYNC3:
				updateTriStateButtonAsid(chip, index, all, asidState.overrideSync);
				break;

			case Button::RING1:
			case Button::RING2:
			case Button::RING3:
				updateTriStateButtonAsid(chip, index, all, asidState.overrideRingMod);
				break;

			case Button::LFO_CHAIN1:
			case Button::LFO_CHAIN2:
			case Button::LFO_CHAIN3:
				updateTriStateButtonAsid(chip, index, all, asidState.overrideFilterRoute);
				asidUpdateFilterRoute(chip, false);
				if (all) {
					asidUpdateFilterRoute(1, all);
				}
				break;

			case Button::PRESET_RESET:
				resetDown = true;
				resetDownTimer = 0;

				if (all) {
					asidRestore(-1);
				} else {
					asidRestore(chip);
				}
				return; // Done, no indication update
				break;

			case Button::FILTER_MODE:
				asidAdvanceFilterMode(chip, false);
				if (all) {
					// Same thing for chip 2
					asidAdvanceFilterMode(1, all);
				}
				break;

			case Button::LFO_RECT: {
				bool tmpMode;
				asidState.isCleanMode = !asidState.isCleanMode;
				tmpMode = asidState.isCleanMode;
				asidRestore(-1); // restore both

				// Restore will affect clean mode itself, so need to keep the state
				asidState.isCleanMode = tmpMode;
				return; // Done, no indication update
			} break;

			case Button::RETRIG:
				asidState.selectedSids.b.sid1 = true;
				return; // Done, no indication update
				break;

			case Button::LOOP:
				asidState.selectedSids.b.sid2 = true;
				return; // Done, no indication update
				break;

			case Button::ARP_MODE:
				asidToggleCutoffAdjustMode(true);
				return; // Done, no indication update
				break;

#ifdef ASID_PROFILER
			case Button::LFO_TRI:
				asidState.selectedSids.b.dbg1 = true;
				return;
				break;

			case Button::LFO_SAW:
				asidState.selectedSids.b.dbg2 = true;
				return;
				break;

			case Button::LFO_NOISE:
				asidState.selectedSids.b.dbg3 = true;
				return;
				break;

			case Button::LFO_ENV3:
				asidState.selectedSids.b.dbg4 = true;
				return;
				break;
#endif

			default:
				indicateChange[chip] = false;
				break;
		}

		// Update dot indication for changed SIDs, if needed
		for (byte i = 0; i < 2; i++) {
			if (indicateChange[i] && !asidState.isRemixed[i]) {
				asidIndicateChanged(i);
			}
		}

	} else {
		// Released
		switch (button) {
			case Button::PRESET_RESET:
				resetDown = false;
				break;

			case Button::RETRIG:
				asidState.selectedSids.b.sid1 = false;
				break;

			case Button::LOOP:
				asidState.selectedSids.b.sid2 = false;
				break;

			case Button::ARP_MODE:
				asidToggleCutoffAdjustMode(false);
				break;

#ifdef ASID_PROFILER
			case Button::LFO_TRI:
				asidState.selectedSids.b.dbg1 = false;
				break;

			case Button::LFO_SAW:
				asidState.selectedSids.b.dbg2 = false;
				break;

			case Button::LFO_NOISE:
				asidState.selectedSids.b.dbg3 = false;
				break;

			case Button::LFO_ENV3:
				asidState.selectedSids.b.dbg4 = false;
				break;
#endif
			default:
				break;
		}
	}
}

void buttChanged(byte number, bool value) {
	Button button = static_cast<Button>(number);
	if (asidState.enabled) {
		return buttChangedAsid(button, value);
	}

	if (!value) {
		//  PRESSED
		byte index = indexFromButton(button);

		switch (button) {
			case Button::RECT1:
			case Button::RECT2:
			case Button::RECT3:
				shapeButtPressed(index, PresetVoice::PULSE);
				break;

			case Button::TRI1:
			case Button::TRI2:
			case Button::TRI3:
				shapeButtPressed(index, PresetVoice::TRI);
				break;

			case Button::SAW1:
			case Button::SAW2:
			case Button::SAW3:
				shapeButtPressed(index, PresetVoice::SAW);
				break;

			case Button::NOISE1:
			case Button::NOISE2:
			case Button::NOISE3:
				shapeButtPressed(index, PresetVoice::NOISE);
				break;

			case Button::SYNC1:
			case Button::SYNC2:
			case Button::SYNC3:
				preset_data.voice[index].reg_control ^= 2;
				sendMidiButt(49 + index * 2, preset_data.voice[index].reg_control & 2);
				break;

			case Button::RING1:
			case Button::RING2:
			case Button::RING3:
				preset_data.voice[index].reg_control ^= 4;
				sendMidiButt(50 + index * 2, preset_data.voice[index].reg_control & 4);
				break;

			case Button::LFO_CHAIN1:
			case Button::LFO_CHAIN2:
			case Button::LFO_CHAIN3:
				lfoButtPressed = true;
				ui_state.selectedLfo = index;
				if (ui_state.lastPot != (9 + index * 2) && ui_state.lastPot != (10 + index * 2) &&
				    ui_state.lastPot != 20) {
					preset_data.lfo[ui_state.selectedLfo].mapping[ui_state.lastPot] =
					    !preset_data.lfo[ui_state.selectedLfo].mapping[ui_state.lastPot];
				}
				break;

			case Button::LFO_RECT:
			case Button::LFO_SAW:
			case Button::LFO_TRI:
			case Button::LFO_NOISE:
			case Button::LFO_ENV3:
				if (preset_data.lfo[ui_state.selectedLfo].shape != (index + 1)) {
					preset_data.lfo[ui_state.selectedLfo].shape = (index + 1);
				} else {
					preset_data.lfo[ui_state.selectedLfo].shape = 0;
				}
				break;

				break;

			case Button::ARP_MODE:
				if (!demoMode) {
					noArpAction = true;
					arpModeHeld = true;
					if (ui_state.midiSetup) {
						ui_state.midiSetup = 3;
					}
				}
				break;

			case Button::RETRIG:
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

			case Button::LOOP:
				preset_data.lfo[ui_state.selectedLfo].looping = !preset_data.lfo[ui_state.selectedLfo].looping;
				break;

			case Button::PRESET_UP:
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

			case Button::PRESET_DOWN:

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

			case Button::PRESET_RESET:
				if (demoMode)
					stopdemoMode = true;
				resetDown = true;
				if (ui_state.saveMode) {
					ui_state.saveMode = false;
				} else {
					load(1);
					resetDownTimer = 0;
				}
				break;

			case Button::FILTER_MODE:
				fatChanged = false;
				filterAssignmentChanged = false;
				ui_state.filterModeHeld = true;
				volumeChanged = false;
				break;
		}
	} else {
		switch (button) {
			case Button::RECT1:
			case Button::TRI1:
			case Button::SAW1:
			case Button::NOISE1:
				shape1Pressed = false;
				shape1PressedTimer = 0;
				break;

			case Button::LFO_CHAIN1:
			case Button::LFO_CHAIN2:
			case Button::LFO_CHAIN3:
				lfoButtPressed = false;
				lfoButtTimer = 0;
				break;

			case Button::ARP_MODE:
				if (!demoMode) {
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
									if (preset_data.arp_mode > 7) {
										preset_data.arp_mode = 0;
										sendNoteOff(lastNote, 127, masterChannelOut);
									}
									arpReset();
								}
							} else if (ui_state.midiSetup == 3) {
								ui_state.midiSetup = 0;
							}
						}
					}

					arpModeHeld = false;
				}
				break;

			case Button::PRESET_UP:
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

			case Button::PRESET_DOWN:
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

			case Button::PRESET_RESET:
				resetDown = false;
				break;

			case Button::FILTER_MODE:
				if ((!filterAssignmentChanged) && (!fatChanged)) {
					preset_data.filter_mode =
					    static_cast<FilterMode>((static_cast<int>(preset_data.filter_mode) + 1) % 5);
					sendCC(55, map((int)preset_data.filter_mode, 0, 4, 0, 1023));
				}
				ui_state.filterModeHeld = false;

				break;

			default:
				break;
		}
	}
}
