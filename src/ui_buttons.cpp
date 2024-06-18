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

// combine waveforms (sorry no mask used, but it should fit into waveform enum)
WaveformState combineWaveformAsid(byte chip, byte voice, WaveformState waveform) {

	if (waveform == WaveformState::RECT) {
		switch (asidState.overrideWaveform[chip][voice]) {
			case WaveformState::TRI:
				waveform = WaveformState::RT;
				break;
			case WaveformState::SAW:
				waveform = WaveformState::RS;
				break;
			case WaveformState::RT:
				waveform = WaveformState::TRI;
				break;
			case WaveformState::TS:
				waveform = WaveformState::RTS;
				break;
			case WaveformState::RS:
				waveform = WaveformState::SAW;
				break;
			case WaveformState::RTS:
				waveform = WaveformState::TS;
				break;
			default:
				break;
		}

	} else if (waveform == WaveformState::TRI) {
		switch (asidState.overrideWaveform[chip][voice]) {
			case WaveformState::RECT:
				waveform = WaveformState::RT;
				break;
			case WaveformState::SAW:
				waveform = WaveformState::TS;
				break;
			case WaveformState::RT:
				waveform = WaveformState::RECT;
				break;
			case WaveformState::TS:
				waveform = WaveformState::SAW;
				break;
			case WaveformState::RS:
				waveform = WaveformState::RTS;
				break;
			case WaveformState::RTS:
				waveform = WaveformState::RS;
				break;
			default:
				break;
		}

	} else if (waveform == WaveformState::SAW) {
		switch (asidState.overrideWaveform[chip][voice]) {
			case WaveformState::RECT:
				waveform = WaveformState::RS;
				break;
			case WaveformState::TRI:
				waveform = WaveformState::TS;
				break;
			case WaveformState::RT:
				waveform = WaveformState::RTS;
				break;
			case WaveformState::TS:
				waveform = WaveformState::TRI;
				break;
			case WaveformState::RS:
				waveform = WaveformState::RECT;
				break;
			case WaveformState::RTS:
				waveform = WaveformState::RT;
				break;
			default:
				break;
		}
	}

	return waveform;
}

void updateWaveformAsid(byte chip, byte voice, bool all, WaveformState waveform, bool isCombineMode) {
	if (isCombineMode) {
		waveform = combineWaveformAsid(chip, voice, waveform);
	}

	if (asidState.overrideWaveform[chip][voice] == waveform) {
		asidState.overrideWaveform[chip][voice] = WaveformState::NOISE_ONLY;
	} else {
		asidState.overrideWaveform[chip][voice] = waveform;
	}
	asidState.muteChannel[chip][voice] = false;

	if (all) {
		// Copy from 0 to other chips
		for (byte i = 1; i < SIDCHIPS; i++) {
			asidState.overrideWaveform[i][voice] = asidState.overrideWaveform[0][voice];
			asidState.muteChannel[i][voice] = false;
		}
	}
}

void updateTriStateButtonAsid(byte chip, byte voice, bool all, OverrideState buttonState[][SIDVOICES_PER_CHIP]) {
	if (buttonState[chip][voice] == OverrideState::OFF) {
		buttonState[chip][voice] = OverrideState::ON;
	} else {
		buttonState[chip][voice] = static_cast<OverrideState>(static_cast<int>(buttonState[chip][voice]) + 1);
	}
	if (all) {
		// Copy from 0 to other chips
		for (byte i = 1; i < SIDCHIPS; i++) {
			buttonState[i][voice] = buttonState[0][voice];
		}
	}
}

bool updateChipSoloStatus() {
	byte soloChip, shouldSolo;
	// Find the selected chip
	if (asidState.selectedSids.b.sid1 && !asidState.selectedSids.b.sid2) {
		soloChip = 0;
	} else if (asidState.selectedSids.b.sid2 && !asidState.selectedSids.b.sid1) {
		soloChip = 1;
	} else {
		soloChip = 2;
	}

	// Find out if a chip should be soloed or umute all
	if (asidState.soloedChannel == 16 + soloChip) {
		// This chip is already soloed => unmute all
		shouldSolo = false;
		asidState.soloedChannel = -1;
	} else {
		// Solo the selected chip
		shouldSolo = true;
		asidState.soloedChannel = 16 + soloChip;
	}

	// Update SID chip channels & filtermode mute status
	for (byte c = 0; c < SIDCHIPS; c++) {
		for (byte v = 0; v < SIDVOICES_PER_CHIP; v++) {
			asidState.muteChannel[c][v] = shouldSolo && (soloChip != c);
		}

		asidState.muteFilterMode[c] = shouldSolo && (soloChip != c);
	}

	// Update FM channels mute status
	if (asidState.isSidFmMode) {
		for (byte v = 0; v < OPL_NUM_CHANNELS_MAX; v++) {
			asidState.muteFMChannel[v] = shouldSolo && (soloChip != 1);
		}
	}

	return asidState.soloedChannel != -1;
}

void buttChangedAsid(Button button, bool value) {
	if (!value) {
		// Pressed
		byte index = indexFromButton(button);
		byte chip;

		bool all = false;
		bool checkMoreButtons = false;

		// Find out what chips are selected
		// If one pressed - execute actions only on that chip
		// If all active  - execute chip 0 then copy to other chips (executing actions if needed)
		if (asidState.selectedSids.all == 0) {
			// All chips should be affected
			all = true;
			chip = 0;
		} else {
			// One chip selected
			if (asidState.selectedSids.b.sid1 && !asidState.selectedSids.b.sid2) {
				chip = 0;
			} else if (asidState.selectedSids.b.sid2 && !asidState.selectedSids.b.sid1) {
				chip = 1;
			} else {
				chip = 2;
			}
		}

		// Check global buttons
		switch (button) {
			case Button::PRESET_RESET:
				// prevent short circuit combo (LFO_NOISE+FILTER_MODE)
				if (asidState.isShiftMode) {
					break;
				}

				// Clear remix parameters
				resetDown = true;
				resetDownTimer = 0;

				if (all) {
					asidRestore(-1);
				} else {
					asidRestore(chip);
				}
				break;

			case Button::LFO_RECT:
				// Clean mode - no remixing possible
				asidState.isCleanMode = !asidState.isCleanMode;

				// full restore by shift
				if (asidState.isShiftMode) {

					bool tmpMode = asidState.isCleanMode;
					asidRestore(-1); // restore both

					// Restore will affect clean mode itself, so need to keep the state
					asidState.isCleanMode = tmpMode;

				} else {

					// Restore isOverride States
					asidUpdateOverrides();
				}
				break;

			case Button::RETRIG:
				// SID 1 select-button (SID 3 by combo)
				asidSelectDefaultChip(asidState.selectButtonCounter ? 2 : 0);

				// Solo entire chip
				if (asidState.isSoloButtonHeld && !updateChipSoloStatus() && !asidState.selectButtonCounter) {
					asidClearDefaultChip();
				}

				// increment select counter
				if (SIDCHIPS > 2) {
					asidState.selectButtonCounter++;
				}
				break;

			case Button::LOOP:
				// SID 2 select-button (SID 3 by combo)
				asidSelectDefaultChip(asidState.selectButtonCounter ? 2 : 1);

				// Solo entire chip
				if (asidState.isSoloButtonHeld && !updateChipSoloStatus() && !asidState.selectButtonCounter) {
					asidClearDefaultChip();
				}

				// increment select counter
				if (SIDCHIPS > 2) {
					asidState.selectButtonCounter++;
				}
				break;

			case Button::ARP_MODE:
				// Toggle SID filter cutoff scaling method
				asidToggleCutoffAdjustMode(true);
				break;

			case Button::PRESET_UP:
			case Button::PRESET_DOWN:
				// Select default chip for remixing
				asidAdvanceDefaultChip(button == Button::PRESET_UP);
				break;

#ifdef ASID_PROFILER
			case Button::LFO_TRI:
				asidState.selectedSids.b.dbg1 = true;
				break;

			case Button::LFO_SAW:
				asidState.selectedSids.b.dbg2 = true;
				break;

			case Button::LFO_NOISE:
				asidState.selectedSids.b.dbg3 = true;
				break;

			case Button::LFO_ENV3:
				asidState.selectedSids.b.dbg4 = true;
				break;
#else

			case Button::LFO_TRI:
				break;

			case Button::LFO_SAW:
				break;

			case Button::LFO_NOISE:
				// Engage Shift function
				asidState.isShiftMode = true;
				break;

			case Button::LFO_ENV3:
				// Engage Solo function
				asidState.isSoloButtonHeld = true;
#endif
				break;

			default:
				// Other press => continue to check buttons
				checkMoreButtons = true;
				break;
		}

		if (!checkMoreButtons) {
			// Done, no indication update
			return;
		}

		// Check chip-specific buttons
		if (asidState.isSidFmMode && chip == 1) {
			// If the FM mode is on and the FM chip is selected, handle FM-specific buttons

			int8_t muteFMChannelIdx = -1; // default => no FM mute
			switch (button) {
				case Button::RECT1:
				case Button::RECT2:
				case Button::RECT3:
					muteFMChannelIdx = index * 4;
					break;

				case Button::TRI1:
				case Button::TRI2:
				case Button::TRI3:
					muteFMChannelIdx = 1 + index * 4;
					break;

				case Button::SAW1:
				case Button::SAW2:
				case Button::SAW3:
					muteFMChannelIdx = 2 + index * 4;
					break;

				case Button::NOISE1:
				case Button::NOISE2:
					muteFMChannelIdx = 3 + index * 4;
					break;

				default:
					// nothing to do..
					break;
			}

			// If any FM mute should be done, update status
			if (muteFMChannelIdx != -1) {
				if (asidState.isSoloButtonHeld) {
					// Solo mute case - mute all other channels than this one
					// If already soloed, unmute all

					bool shouldSolo;
					if (asidState.soloedChannel == 32 + muteFMChannelIdx) {
						// The pressed channel is already soloed => remove solo and unmute all
						shouldSolo = false;
						asidState.soloedChannel = -1;
					} else {
						// Store solo state and mute all other channels
						shouldSolo = true;
						asidState.soloedChannel = 32 + muteFMChannelIdx;
					}

					// Mute/unmute all SID channels (& filtermode?)
					for (byte c = 0; c < SIDCHIPS; c++) {
						for (byte v = 0; v < SIDVOICES_PER_CHIP; v++) {
							asidState.muteChannel[c][v] = shouldSolo;
						}

						// RIO: should a complete chip be muted? then:
						// asidState.muteFilterMode[c] = shouldSolo;
					}

					// For FM channels, mute all but the soloed one (or unmute all)
					for (byte v = 0; v < OPL_NUM_CHANNELS_MAX; v++) {
						asidState.muteFMChannel[v] = shouldSolo && (v != muteFMChannelIdx);
					}
				} else {
					// Regular mute case - just invert muting status of the selected channel
					asidState.muteFMChannel[muteFMChannelIdx] = !asidState.muteFMChannel[muteFMChannelIdx];

					// Reset soloed status
					asidState.soloedChannel = -1;
				}
			}

		} else {
			// Handling of buttons for a SID chip
			switch (button) {
				case Button::RECT1:
				case Button::RECT2:
				case Button::RECT3:
					updateWaveformAsid(chip, index, all, WaveformState::RECT, asidState.isShiftMode);
					break;

				case Button::TRI1:
				case Button::TRI2:
				case Button::TRI3:
					updateWaveformAsid(chip, index, all, WaveformState::TRI, asidState.isShiftMode);
					break;

				case Button::SAW1:
				case Button::SAW2:
				case Button::SAW3:
					updateWaveformAsid(chip, index, all, WaveformState::SAW, asidState.isShiftMode);
					break;

				case Button::NOISE1:
				case Button::NOISE2:
				case Button::NOISE3:
					if (asidState.isSoloButtonHeld) {
						bool shouldSolo;
						if (asidState.soloedChannel == (chip * 3 + index)) {
							// The pressed channel is already soloed => remove solo and unmute all
							shouldSolo = false;
							asidState.soloedChannel = -1;
						} else {
							// Store solo state and mute all other channels
							shouldSolo = true;
							asidState.soloedChannel = (chip * 3 + index);
						}

						// Mute all but the soloed SID channel (or unmute all)
						for (byte c = 0; c < SIDCHIPS; c++) {
							for (byte v = 0; v < SIDVOICES_PER_CHIP; v++) {
								asidState.muteChannel[c][v] = shouldSolo && !((c == chip || all) && (v == index));
							}
						}

						// Mute/unmute all FM channels
						for (byte v = 0; v < OPL_NUM_CHANNELS_MAX; v++) {
							asidState.muteFMChannel[v] = shouldSolo;
						}

					} else {

						if (asidState.isShiftMode) {
							asidRestoreVoice(all ? -1 : chip, index, InitState::VOICE);
						} else {
							asidState.muteChannel[chip][index] = !asidState.muteChannel[chip][index];
							if (all) {
								// Same thing for other chips
								for (byte i = 1; i < SIDCHIPS; i++) {
									asidState.muteChannel[i][index] = asidState.muteChannel[0][index];
								}
							}

							// Reset soloed status
							asidState.soloedChannel = -1;
						}
					}

					break;

				case Button::SYNC1:
				case Button::SYNC2:
				case Button::SYNC3:
					if (asidState.isShiftMode) {
						asidRestoreVoice(all ? -1 : chip, index, InitState::PITCH);
					} else {
						updateTriStateButtonAsid(chip, index, all, asidState.overrideSync);
					}
					break;

				case Button::RING1:
				case Button::RING2:
				case Button::RING3:
					if (asidState.isShiftMode) {
						asidRestoreVoice(all ? -1 : chip, index, InitState::ADSR);
					} else {
						updateTriStateButtonAsid(chip, index, all, asidState.overrideRingMod);
					}
					break;

				case Button::LFO_CHAIN1:
				case Button::LFO_CHAIN2:
				case Button::LFO_CHAIN3:
					if (asidState.isShiftMode) {
						asidRestoreVoice(all ? -1 : chip, index, InitState::FILTER_ROUTE);
					} else {
						updateTriStateButtonAsid(chip, index, all, asidState.overrideFilterRoute);
						asidUpdateFilterRoute(chip, false);
						if (all) {
							// Same thing for other chips
							for (byte i = 1; i < SIDCHIPS; i++) {
								asidUpdateFilterRoute(i, all);
							}
						}
					}
					break;

				case Button::FILTER_MODE:
					if (asidState.isShiftMode) {
						asidRestoreFilterMode(all ? -1 : chip);
					} else {
						asidAdvanceFilterMode(chip, false);
						if (all) {
							// Same thing for other chips
							for (byte i = 1; i < SIDCHIPS; i++) {
								asidAdvanceFilterMode(i, all);
							}
						}
					}
					break;

				default:
					// nothing to do..
					break;
			}
		}

		// Update dot indication for changed SIDs, if needed
		for (byte i = 0; i < 2; i++) {
			asidIndicateChanged(i);
		}

	} else {
		// Released events, global
		switch (button) {
			case Button::PRESET_RESET:
				// prevent short circuit combo (LFO_NOISE+FILTER_MODE)
				if (asidState.isShiftMode) {
					break;
				}

				resetDown = false;
				break;

			case Button::RETRIG:
			case Button::LOOP:
				if (!asidState.isSoloButtonHeld) {

					// validate selection
					if (!asidState.selectButtonCounter) {
						asidClearDefaultChip();

						// Reset soloed status
						asidState.soloedChannel = -1;

					} else {

						// Restore held button
						asidSelectDefaultChip(button == Button::RETRIG ? 1 : 0);
					}
				}

				// decrement select counter
				if (asidState.selectButtonCounter) {
					asidState.selectButtonCounter--;
				}

				break;

			case Button::ARP_MODE:
				asidToggleCutoffAdjustMode(false);
				break;

#ifndef ASID_PROFILER

			case Button::LFO_NOISE:
				asidState.isShiftMode = false;
				break;

			case Button::LFO_ENV3:
				// Disengage Solo function
				asidState.isSoloButtonHeld = false;
				break;
#else
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
				noArpAction = true;
				arpModeHeld = true;
				if (ui_state.midiSetup) {
					ui_state.midiSetup = 3;
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
					setNextFilterMode();
					sendCC(55, map(getFilterModeIdx(), 0, 7, 0, 1023));
				}
				ui_state.filterModeHeld = false;

				break;

			default:
				break;
		}
	}
}
