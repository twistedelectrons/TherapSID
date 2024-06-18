#include "asid.h"
#include "ui_leds.h"
#include "sid.h"
#include <util/atomic.h>
#include "util.hpp"
#include "armsid.h"
#include "opl.h"
#include "ui_pots.h"

asidState_t asidState;

#define SID_VC_PITCH_LO 0x00
#define SID_VC_PITCH_HI 0x01
#define SID_VC_PULSE_WIDTH_LO 0x02
#define SID_VC_PULSE_WIDTH_HI 0x03
#define SID_VC_CONTROL 0x04
#define SID_VC_ENVELOPE_AD 0x05
#define SID_VC_ENVELOPE_SR 0x06

#define SID_FILTER_CUTOFF_LO 0x15
#define SID_FILTER_CUTOFF_HI 0x16
#define SID_FILTER_RESONANCE_ROUTING 0x17
#define SID_FILTER_MODE_VOLUME 0x18

#define POT_NOON 512

#define DISPLAY_STANDARD 0
#define DISPLAY_ASID_REMIX 1
#define DISPLAY_ASID_CLEAN 2
#define DISPLAY_ASID_SHIFT 3 // shift indicator

#define FINETUNE_0_CENTS 31 + 978 // corresponds to -26 cents, to simulate PAL C64 on TS 1MHz

#define SLOW_TIMER_INIT_2_SEC (39 * 2) // 39 Hz

#ifdef ASID_PROFILER
static byte maxSidTime;
static uint16_t ticksBetweenMsg;
static uint16_t minTicksBetweenMsg;
static uint16_t maxTicksBetweenMsg;
static uint16_t cpuTimer;
static uint16_t* ptrVariables[] = {&ticksBetweenMsg, &minTicksBetweenMsg, &maxTicksBetweenMsg};
static byte variableIndex = 0;
#endif

void visualizeSidLEDs();

/*

 Main functionalites of the TherapSID ASID implementation
 ========================================================

 When receiving the first ASID protocol message, it will directly put the
 instrument in ASID mode (indicated by "AS"). Each message will instruct
 which registers in the SID chip to update.

 The incoming SID data is visualized the following way:
 * The relevant Voice LEDs lights up to show what waveforms and sync/ring mod
   states are used

 * The LFO LEDs are used to indicate filter status:
  - LINK 1, 2, 3 indicates that the voice is routed to the filter
  - Waveforms indicate the cutoff frequency as a 5-stage amount
  - RETRIG/LOOP indicate the resonance level as a 2-stage amount

 * The Filter type LEDs shows the currently active filter type


The ASID mode supports several types of SID-files:

 * When using standard MIDI interfaces and one or two SID-chips:
  - Single-SID files at 50/60Hz (the most common format). The SID playback is
    duplicated on the second SID.
 * When using Turbo MIDI (Elektron TM-1 interface and similar) also the
   following:
  - Dual-SID files ("Stereo SIDs”)
  - Multi-speed SID files (2x, 3x, 4x etc)
  - Combinations of the above
  - When also using ARM2SID:
   -- SID+FM files (custom files made for the Sound Expander or FM-YAM, which
      uses the Yamaha OPL1 or OPL2 chips)
  - When built with SIDCHIPS = 3 and using ARM2SID with special pin connected:
   -- Triple-SID files (Note: See sid.h for instructions on hw patching)


In addition there is the possibility to remix the SID files live:
 * NOISE waveform button mutes/unmutes the corresponding track

 * RECT/TRI/SAW/SYNC/RING waveform buttons force that feature on/off (ignoring
   incoming changes from the SID file). RECT/TRI/SAW combinations are possible
   with the shift button (LFO NOISE) held down. Noise will always be let through.

 * WIDTH forces the pulse width to that value (ignoring incoming)

 * TUNE transposes the pitch +/- one octave. Noise will be left untransposed.

 * FINE fine-tunes the pitch +/-55 cents

 * LFO LINK buttons force filter routing of the corresponding track on/off
   (ignoring incoming changes)

 * CUTOFF, RESO, ATTACK, DECAY, SUSTAIN, RELEASE adds/subtracts offset to the
   incoming changes. Noon means no change.
   Note: there is also a way to let the cutoff instead be scaled by
   using another cutoff adjusting mode (ARP MODE, see below)

 * FILTER TYPE button forces the type (ignoring incoming changes)

 * RETRIG and LOOP means SID1 and SID2 respectively. When held down, only that
   chip will be affected by the knob moves and button presses. This means that
   completely different parameters can be used on each chip, including mutes.

 * ARP MODE will toggle the CUTOFF adjusting mode. "FO" (Filter Offset) means
   offset is added/subtracted, while "FS" (Filter Scaling) means offset is
   scaled (0 to 200%). Different songs might benefit from one or the other
   mode. This mode is global and is not affected by SID1/SID2 buttons.

 * The dots on the red LED display indicates that something has been remixed.
   Left dot means SID1 and right dot means SID2.

 * Pressing LFO SQUARE will put the player in a "clean" mode (indicated by
   "AC"), accepts any remixed parameters, but plays sid tune unchanged

 * Brief press on RESET restores remix parameters to original. This also works
   with the SID1/2 separation.

 * Long press on RESET jumps out of ASID mode

 * Holding ENV3 when pressing on a channel mute button will instead solo it. If
   holding ENV3 and pressing RETRIG or LOOP (i.e the SID1/SID2 selectors), that
   whole SID chip will be soloed and selected.

 * PRESET UP/DOWN will change the default chip for remixing - from affecting
   both to only one of them (useful for one-handed remixing of one chip, not
   needing to hold RETRIG/LOOP). This will be indicated by A1, A2 - as opposed
   to AS which means both.

 * Holding the shift button (LFO NOISE) allows you to restore individual
   parameter areas like:
  - SH + NOISE restores VOICE parameters (WAVEFORM, PW, TUNE, FINE, RING/SYNC)
  - SH + SYNC restores PITCH (TUNE, FINE)
  - SH + RING restores ADSR
  - SH + LFO LINK restores FILTER ROUTE
  - SH + FILTER MODE restores FILTER MODE
  - SH + POT restores individual values by turning the POT

In the SID+FM mode, the following buttons apply for remixing:
 * Voice 1 SQR to NOISE, Voice 2 SQR to NOISE and Voice 3 SQR to SAW works as
   indication LEDs and mute on/off buttons for the 9 (or 11) FM channels

 * WIDTH1 to WIDTH3 add/subtracts to the Operator 1 level of the 9 FM channels

 * ATTACK1 to ATTACK3 add/subtracts to the Operator 2 level of the 9 FM channels

 * RATE1 to DEPTH3, SCRUB to RANGE adds/subtracts to Feedback of the 9 FM channels

*/

/*
 * Initialize data structures for a certain chip. -1 means all chips.
 */
void asidInit(int chip) {

	byte first = chip > -1 ? chip : 0;
	byte last = chip > -1 ? chip : SIDCHIPS - 1;

	for (byte chip = first; chip <= last; chip++) {

		for (byte i = 0; i < SIDVOICES_PER_CHIP; i++) {
			asidInitVoice(chip, i, InitState::ALL);
		}

		asidInitFilterMode(chip);

		asidState.adjustCutoff[chip] = POT_VALUE_TO_ASID_CUTOFF(POT_NOON);
		asidState.adjustReso[chip] = POT_VALUE_TO_ASID_LORES(POT_NOON);

#ifdef ASID_VOLUME_ADJUST
		asidState.adjustVolume[chip] = POT_VALUE_TO_ASID_LORES(POT_NOON);
#endif

		asidState.isRemixed[chip] = false;
		dotSet(chip, false);
	}

	asidState.selectedSids.all = 0;
	asidState.isCleanMode = false;
	asidState.isShiftMode = false;
	asidState.slowTimer = SLOW_TIMER_INIT_2_SEC;
	asidState.displayState = DISPLAY_STANDARD;
	asidState.defaultSelectedChip = -1;
	asidState.isSoloButtonHeld = false;
	asidState.selectButtonCounter = 0;
	asidState.soloedChannel = -1;
	asidState.lastDuplicatedChip = asidState.isSidFmMode ? 0 : 1;

	// FM Channels
	for (byte i = 0; i < OPL_NUM_CHANNELS_MAX; i++) {
		asidState.muteFMChannel[i] = false;
	}

	for (byte i = 0; i < OPL_NUM_CHANNELS_MELODY_MODE; i++) {
		asidState.adjustFMOpLevel[i] = POT_NOON;
		asidState.adjustFMOpLevel[i + OPL_NUM_CHANNELS_MELODY_MODE] = POT_NOON;
		asidState.adjustFMFeedback[i] = POT_NOON;
	}
}

/*
 * Initialize SIDs single voice
 */
void asidInitVoice(int chip, byte voice, InitState init) {

	byte first = chip > -1 ? chip : 0;
	byte last = chip > -1 ? chip : SIDCHIPS - 1;

	for (byte chip = first; chip <= last; chip++) {
		// clang-format off
		bool initWaveForm		= init == InitState::ALL || init == InitState::VOICE || init == InitState::WAVEFORM;
		bool initPW				= init == InitState::ALL || init == InitState::VOICE || init == InitState::PW;
		bool initPitch			= init == InitState::ALL || init == InitState::VOICE || init == InitState::PITCH;
		bool initSync			= init == InitState::ALL || init == InitState::VOICE || init == InitState::SYNC;
		bool initRing			= init == InitState::ALL || init == InitState::VOICE || init == InitState::RING;
		bool initADSR			= init == InitState::ALL || init == InitState::ADSR;
		bool initFilterRoute	= init == InitState::ALL || init == InitState::FILTER_ROUTE;
		// clang-format on

		if (initWaveForm) {
			asidState.muteChannel[chip][voice] = false;
			asidState.overrideWaveform[chip][voice] = WaveformState::SIDFILE;
		}

		if (initSync) {
			asidState.overrideSync[chip][voice] = OverrideState::SIDFILE;
		}

		if (initRing) {
			asidState.overrideRingMod[chip][voice] = OverrideState::SIDFILE;
		}

		if (initPW) {
			asidState.overridePW[chip][voice] = POT_VALUE_TO_ASID_PW(POT_NOON);
			asidState.isOverridePW[chip][voice] = false;
		}

		if (initPitch) {
			asidState.adjustOctave[chip][voice] = 0;
			asidState.adjustFine[chip][voice] = FINETUNE_0_CENTS;
		}

		if (initADSR) {
			asidState.adjustAttack[chip][voice] = POT_VALUE_TO_ASID_LORES(POT_NOON);
			asidState.adjustSustain[chip][voice] = POT_VALUE_TO_ASID_LORES(POT_NOON);
			asidState.adjustDecay[chip][voice] = POT_VALUE_TO_ASID_LORES(POT_NOON);
			asidState.adjustRelease[chip][voice] = POT_VALUE_TO_ASID_LORES(POT_NOON);
		}

		if (initFilterRoute) {
			asidState.overrideFilterRoute[chip][voice] = OverrideState::SIDFILE;
		}
	}
}

/*
 * Initialize SID register structures to stable state, used first time entering ASID mode
 */
void initializeSids() {
	byte chip;
	// SID chip registers
	for (size_t reg = 0; reg < sizeof(asidState.lastSIDvalues[0]) / (sizeof(*asidState.lastSIDvalues[0])); reg++) {
		for (chip = 0; chip < SIDCHIPS; chip++) {
			asidState.lastSIDvalues[chip][reg] = 0;
		}
	}
	for (chip = 0; chip < SIDCHIPS; chip++) {
		asidState.lastSIDvalues[chip][SID_FILTER_MODE_VOLUME] = 0x0F;
	}

	// ARMSID FM chip registers
	for (size_t reg = 0;
	     reg < sizeof(asidState.lastFMvaluesFeedbackConn) / (sizeof(*asidState.lastFMvaluesFeedbackConn)); reg++) {
		asidState.lastFMvaluesFeedbackConn[reg] = 0;
	}

	for (size_t reg = 0; reg < sizeof(asidState.lastFMvaluesKslTotalLev) / (sizeof(*asidState.lastFMvaluesKslTotalLev));
	     reg++) {
		asidState.lastFMvaluesKslTotalLev[reg] = 0;
	}
}

void showWaveformLEDs(byte voice, byte data) {
	// Basic waveforms
	ledSet(4 * voice + 1, (data & 0b01000000) > 0);
	ledSet(4 * voice + 2, (data & 0b00010000) > 0);
	ledSet(4 * voice + 3, (data & 0b00100000) > 0);
	ledSet(4 * voice + 4, (data & 0b10000000) > 0);

	// Sync/Ring
	ledSet(16 + voice * 2, (data & 0b00000010) > 0);
	ledSet(17 + voice * 2, (data & 0b00000100) > 0);
}

void showFilterRouteLEDs(byte data) {
	ledSet(13, (data & 0b00000001) > 0);
	ledSet(14, (data & 0b00000010) > 0);
	ledSet(15, (data & 0b00000100) > 0);
}

void showFilterModeLEDs(byte data) {
	ledSet(27, (data & 0b00010000) > 0);
	ledSet(28, (data & 0b00100000) > 0);
	ledSet(29, (data & 0b01000000) > 0);
}

void showFilterCutoffLEDs(byte data) {
	ledSet(22, data > 40);
	ledSet(23, data > 80);
	ledSet(24, data > 120);
	ledSet(25, data > 160);
	ledSet(26, data > 200);
}

void showFilterResoLEDs(byte data) {
	ledSet(30, (data >> 4) > 8);
	ledSet(31, (data >> 4) > 14);
}

void asidSetNextFilterMode(byte chip) {
	switch (asidState.filterMode[chip]) {
		case FilterMode::LOWPASS:
			asidState.filterMode[chip] = FilterMode::LB;
			break;
		case FilterMode::LB:
			asidState.filterMode[chip] = FilterMode::BANDPASS;
			break;
		case FilterMode::BANDPASS:
			asidState.filterMode[chip] = FilterMode::BH;
			break;
		case FilterMode::BH:
			asidState.filterMode[chip] = FilterMode::HIGHPASS;
			break;
		case FilterMode::HIGHPASS:
			asidState.filterMode[chip] = FilterMode::NOTCH;
			break;
		case FilterMode::NOTCH:
			asidState.filterMode[chip] = FilterMode::LBH;
			break;
		case FilterMode::LBH:
			asidState.filterMode[chip] = FilterMode::OFF;
			break;
		default:
			asidState.filterMode[chip] = FilterMode::LOWPASS;
			break;
	}
}

void updateLastSIDValues(int chip, byte voice, InitState init) {
	for (size_t reg = 0; reg < sizeof(asidState.lastSIDvalues[0]) / (sizeof(*asidState.lastSIDvalues[0])); reg++) {

		// cast size_t to byte
		byte r = static_cast<byte>(reg);
		byte voiceBase = voice * 7;

		// If specific register not covered by requested init, skip it
		switch (init) {

			case InitState::ALL:
				// default
				break;

			case InitState::WAVEFORM:     // REG COUPLED: +SYNC, RING, GATE...
			case InitState::SYNC:         // REG COUPLED: +WAVEFORMS
			case InitState::RING:         // REG COUPLED: +WAVEFORMS
			case InitState::FILTER_ROUTE: // REG COUPLED: +RESO
				// not supported
				return;

			case InitState::VOICE: // [WAVEFORMS, PW, SYNC/RING, PITCH]
				if (r != voiceBase + SID_VC_PITCH_LO && r != voiceBase + SID_VC_PITCH_HI &&
				    r != voiceBase + SID_VC_PULSE_WIDTH_LO && r != voiceBase + SID_VC_PULSE_WIDTH_HI &&
				    r != voiceBase + SID_VC_CONTROL) {
					continue;
				}
				break;

			case InitState::PITCH:
				if (r != voiceBase + SID_VC_PITCH_LO && r != voiceBase + SID_VC_PITCH_HI) {
					continue;
				}
				break;

			case InitState::PW:
				if (r != voiceBase + SID_VC_PULSE_WIDTH_LO && r != voiceBase + SID_VC_PULSE_WIDTH_HI) {
					continue;
				}
				break;

			case InitState::ADSR:
				if (r != voiceBase + SID_VC_ENVELOPE_AD && r != voiceBase + SID_VC_ENVELOPE_SR) {
					continue;
				}
				break;

			case InitState::FILTER_MODE:
				if (r != SID_FILTER_MODE_VOLUME) {
					continue;
				}
				break;
		}

		if (chip <= 0) {
			sid_chips[0].send_update_immediate(reg, asidState.lastSIDvalues[0][reg]);
		}
		if (((chip < 0) || (chip == 1)) && !asidState.isSidFmMode) {
			sid_chips[1].send_update_immediate(reg, asidState.lastSIDvalues[1][reg]);
		}
#if SIDCHIPS > 2
		if (((chip < 0) || (chip == 2)) && !asidState.isSidFmMode) {
			sid_chips[2].send_update_immediate(reg, asidState.lastSIDvalues[2][reg]);
		}
#endif
	}
}

/*
 * Restore the most important/seldom changed registers, used when
 * pressing RESET briefly
 */
void asidRestore(int chip) {
	asidInit(chip);
	updateLastSIDValues(chip, 0, InitState::ALL);

	// ARMSID FM (OPL) chip registers
	if (asidState.isSidFmMode) {
		for (size_t reg = 0;
		     reg < sizeof(asidState.lastFMvaluesFeedbackConn) / (sizeof(*asidState.lastFMvaluesFeedbackConn)); reg++) {
			sid_chips[1].send_update_immediate(OPL_REG_ADDRESS, reg + 0xc0);
			sid_chips[1].send_update_immediate(OPL_REG_DATA, asidState.lastFMvaluesFeedbackConn[reg]);
		}

		for (size_t reg = 0;
		     reg < sizeof(asidState.lastFMvaluesKslTotalLev) / (sizeof(*asidState.lastFMvaluesKslTotalLev)); reg++) {
			sid_chips[1].send_update_immediate(OPL_REG_ADDRESS, reg + 0x40);
			sid_chips[1].send_update_immediate(OPL_REG_DATA, asidState.lastFMvaluesKslTotalLev[reg]);
		}
	}

	// Reset LEDs for SID
	byte first = chip > -1 ? chip : 0;
	showFilterModeLEDs(asidState.lastSIDvalues[first][SID_FILTER_MODE_VOLUME]);
	showFilterResoLEDs(asidState.lastSIDvalues[first][SID_FILTER_RESONANCE_ROUTING]);
	showFilterRouteLEDs(asidState.lastSIDvalues[first][SID_FILTER_RESONANCE_ROUTING]);
	showFilterCutoffLEDs(asidState.lastSIDvalues[first][SID_FILTER_CUTOFF_HI]);
}

/*
 * Restore the most important/seldom changed registers, used when
 * pressing SHIFT + WAVEFORMS briefly
 */
void asidRestoreVoice(int chip, byte voice, InitState init) {
	asidInitVoice(chip, voice, init);

	// update voice related values
	if (init == InitState::ALL || init == InitState::FILTER_MODE) {
		return;
	}

	updateLastSIDValues(chip, voice, init);
}

void displayAsidRemixMode() {
	// "AS" or "AF" in the default selected modes
	// A1, A2, A3 etc when default selected chip is active
	digit(0, DIGIT_A);
	digit(1, asidState.defaultSelectedChip == -1 ? (asidState.isSidFmMode ? DIGIT_F : DIGIT_S)
	                                             : asidState.defaultSelectedChip + 1);
	asidState.displayState = DISPLAY_ASID_REMIX;
}

void displayAsidShiftMode() {
	// "SH"
	digit(0, DIGIT_S);
	digit(1, DIGIT_H);
	asidState.displayState = DISPLAY_ASID_SHIFT;
}

void displayAsidCleanMode() {
	// "AC"
	digit(0, DIGIT_A);
	digit(1, DIGIT_C);
	asidState.displayState = DISPLAY_ASID_CLEAN;
}

/*
 * Calculates the filter route from incoming data and possible
 * overrides
 */
byte calculateFilterRoute(byte chip, byte data) {
	byte filterMask = 0x01;
	for (byte voice = 0; voice < SIDVOICES_PER_CHIP; voice++) {
		if (asidState.overrideFilterRoute[chip][voice] == OverrideState::ON) {
			data |= filterMask;
		} else if (asidState.overrideFilterRoute[chip][voice] == OverrideState::OFF) {
			data &= ~filterMask;
		}
		filterMask <<= 1;
	}

	return data;
}

void updateFilterMode(byte chip, byte* data) {
	*data &= 0b10001111;

	// muted filter mode should clear LBH bits
	if (asidState.muteFilterMode[chip]) {
		return;
	}

	switch (asidState.filterMode[chip]) {
		case FilterMode::LOWPASS:
			*data |= Sid::LOWPASS;
			break;
		case FilterMode::HIGHPASS:
			*data |= Sid::HIGHPASS;
			break;
		case FilterMode::BANDPASS:
			*data |= Sid::BANDPASS;
			break;
		case FilterMode::NOTCH:
			*data |= Sid::LOWPASS | Sid::HIGHPASS;
			break;
		case FilterMode::LB:
			*data |= Sid::LOWPASS | Sid::BANDPASS;
			break;
		case FilterMode::BH:
			*data |= Sid::BANDPASS | Sid::HIGHPASS;
			break;
		case FilterMode::LBH:
			*data |= Sid::LOWPASS | Sid::BANDPASS | Sid::HIGHPASS;
			break;
		default:
			break;
	}
}
void updatePitch(bool pitchUpdate[], byte maskBytes[], byte dataBytes[], byte sid) {
	for (byte i = 0; i < SIDVOICES_PER_CHIP; i++) {
		if (pitchUpdate[i]) {
			// Current original pitch
			long pitch = (asidState.lastSIDvalues[sid][SID_VC_PITCH_HI + 7 * i] << 8) +
			             asidState.lastSIDvalues[sid][SID_VC_PITCH_LO + 7 * i];

			// Calculate fine tune, about +/- 53 cents, range -80 to 28 cents to adjust
			// for C64 PAL vs TherapSID clock differences.
			int fine = asidState.isCleanMode ? FINETUNE_0_CENTS : asidState.adjustFine[sid][i];
			pitch = (pitch * fine) >> 10;

			// Change octave if not a pure noise playing
			if (!asidState.isCleanMode && ((asidState.lastSIDvalues[sid][SID_VC_CONTROL + 7 * i] & 0xF0) != 0x80)) {
				// Calculate octave switch (-1, 0, +1)
				if (asidState.adjustOctave[sid][i] > 0) {
					pitch <<= asidState.adjustOctave[sid][i];
				} else if (asidState.adjustOctave[sid][i] < 0) {
					pitch >>= (-asidState.adjustOctave[sid][i]);
				}
			}

			// Scale down to even octave if pitch is higher than possible
			while (pitch > 65535) {
				pitch >>= 1;
			}

			// Store in ASID structure (6 bytes per voice)
			dataBytes[i * 6] = pitch & 0xff;
			dataBytes[i * 6 + 1] = pitch >> 8;

			// Set mask bits accordingly per voice
			// x2000011
			// x3300002
			// x0000000
			// x0000000
			switch (i) {
				case 0:
					maskBytes[0] |= 0x03;
					break;
				case 1:
					maskBytes[0] |= 0x40;
					maskBytes[1] |= 0x01;
					break;
				case 2:
					maskBytes[1] |= 0x60;
					break;
			}
		}
	}
}

bool runPulseWidth(byte chip, byte voice, byte*) {
	// Throw pulse width if overriden
	return (!asidState.isOverridePW[chip][voice]);
}

bool runControl(byte chip, byte voice, byte* data) {
	// Mute channel
	if (asidState.muteChannel[chip][voice]) {
		*data = 0b00000000;
	} else {
		if (*data & 0b01110000 && asidState.overrideWaveform[chip][voice] != WaveformState::SIDFILE) {
			// Only when a non-noise waveform has been set, allow for modifying it

			// Common waveforms
			*data &= 0b10001111;
			switch (asidState.overrideWaveform[chip][voice]) {
				case WaveformState::RECT:
					*data |= 0b01000000;
					break;

				case WaveformState::SAW:
					*data |= 0b00100000;
					break;

				case WaveformState::TRI:
					*data |= 0b00010000;
					break;

				// combined waveforms
				case WaveformState::RT:
					*data |= 0b01010000;
					break;

				case WaveformState::TS:
					*data |= 0b00110000;
					break;

				case WaveformState::RS:
					*data |= 0b01100000;
					break;

				case WaveformState::RTS:
					*data |= 0b01110000;
					break;

				default:
					break;
			}
		}

		// Sync
		if (asidState.overrideSync[chip][voice] == OverrideState::ON) {
			*data |= 0b00000010;
		} else if (asidState.overrideSync[chip][voice] == OverrideState::OFF) {
			*data &= ~0b00000010;
		}

		// Ringmod
		if (asidState.overrideRingMod[chip][voice] == OverrideState::ON) {
			*data |= 0b00000100;
		} else if (asidState.overrideRingMod[chip][voice] == OverrideState::OFF) {
			*data &= ~0b00000100;
		}
	}

	return true;
}

bool runEnvelopeAD(byte chip, byte voice, byte* data) {
	// Attack
	int attack = (*data >> 4) + (asidState.adjustAttack[chip][voice]) - 16;
	attack = max(0, min(0x0f, attack));

	// Decay
	int decay = (*data & 0x0f) + (asidState.adjustDecay[chip][voice]) - 16;
	decay = max(0, min(0x0f, decay));

	*data = (attack << 4) | (decay);

	return true;
}

bool runEnvelopeSR(byte chip, byte voice, byte* data) {
	// Sustain
	int sustain = (*data >> 4) + (asidState.adjustSustain[chip][voice]) - 16;
	sustain = max(0, min(0x0f, sustain));

	// Release
	int release = (*data & 0x0f) + (asidState.adjustRelease[chip][voice]) - 16;
	release = max(0, min(0x0f, release));

	*data = (sustain << 4) | (release);
	return true;
}

bool runFilterCutoff(byte chip, byte, byte* data) {
	// Adjust cutoff hi byte

	if (asidState.isCutoffAdjustModeScaling) {
		// Scale it! (factor 4)
		uint16_t cutoff = *data * (asidState.adjustCutoff[chip] >> 1);
		cutoff >>= 7;
		if (cutoff > 255) {
			cutoff = 255;
		}
		*data = cutoff;
	} else {
		int cutoff = *data + (asidState.adjustCutoff[chip]) - 256;
		cutoff = max(0, min(255, cutoff));
		*data = cutoff;
	}
	return true;
}

bool runFilterResonanceAndRoute(byte chip, byte, byte* data) {
	// Adjust resonance
	int reso = (*data >> 4) + (asidState.adjustReso[chip]) - 16;
	reso = max(0, min(0x0f, reso));
	*data = (reso << 4) | (*data & 0x0f);

	// Adjust filter route if needed
	*data = calculateFilterRoute(chip, *data);
	return true;
}

bool runFilterModeAndVolume(byte chip, byte, byte* data) {
	// Filter type
	if (asidState.isOverrideFilterMode[chip] || asidState.muteFilterMode[chip]) {
		updateFilterMode(chip, data);
	}

#ifdef ASID_VOLUME_ADJUST
	// Volume
	int volume = (*data & 0x0f) + (asidState.adjustVolume[chip]) - 16;
	volume = max(0, min(15, volume));
	*data = (*data & 0xf0) | volume;
#endif
	return true;
}

void handleAsidFrameUpdate(byte currentChip, byte* buffer) {
	// Location of real SID register within the ASID package
	const byte ASIDtoSIDregs[SID_REGISTERS_ASID] = {
	    0x00, 0x01, 0x02, 0x03, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0c, 0x0d, 0x0e, 0x0f,
	    0x10, 0x11, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x04, 0x0b, 0x12, 0x04, 0x0b, 0x12,
	};

	static byte newMaskBytes[SIDCHIPS][4];
	static byte newSidData[SIDCHIPS][SID_REGISTERS_ASID];
	// clang-format off
	typedef bool (*runRegFunc_t)(byte chip, byte voice, byte* data);
	// clang-format on

	struct regconfig_t {
		byte voice;
		runRegFunc_t runRegFunction;
		bool isPitch;
	};

	static struct regconfig_t regConfig[] = {
	    {0, NULL, true},
	    {0, NULL, true},
	    {0, runPulseWidth, false},
	    {0, runPulseWidth, false},
	    {0, runControl, false},
	    {0, runEnvelopeAD, false},
	    {0, runEnvelopeSR, false},

	    {1, NULL, true},
	    {1, NULL, true},
	    {1, runPulseWidth, false},
	    {1, runPulseWidth, false},
	    {1, runControl, false},
	    {1, runEnvelopeAD, false},
	    {1, runEnvelopeSR, false},

	    {2, NULL, true},
	    {2, NULL, true},
	    {2, runPulseWidth, false},
	    {2, runPulseWidth, false},
	    {2, runControl, false},
	    {2, runEnvelopeAD, false},
	    {2, runEnvelopeSR, false},

	    {0, NULL, false},
	    {0, runFilterCutoff, false},
	    {0, runFilterResonanceAndRoute, false},
	    {0, runFilterModeAndVolume, false},

	};

	byte asidReg = 0;     // ASID register location in buffer
	byte dataIdx = 4 + 4; // Data location in buffer (masks, msbs first)
	byte data, reg, field, chip;

	bool pitchUpdate[SIDVOICES_PER_CHIP];

	if (!asidState.enabled) {
		asidState.enabled = true;
		initializeSids();
		asidRestore(-1);
		asidState.isCutoffAdjustModeScaling = false;

#ifdef ASID_PROFILER
		maxSidTime = 0;
		ticksBetweenMsg = 0;
		minTicksBetweenMsg = 0xffff;
		maxTicksBetweenMsg = 0;
		cpuTimer = 0;

	} else {
		// Only measure after at least one complete run
		ATOMIC_BLOCK(ATOMIC_FORCEON) {
			ticksBetweenMsg = asidState.timer - cpuTimer;
			cpuTimer = asidState.timer;
		}
		minTicksBetweenMsg = min(ticksBetweenMsg, minTicksBetweenMsg);
		maxTicksBetweenMsg = max(ticksBetweenMsg, maxTicksBetweenMsg);
#endif
	}

	// Display the right mode (but update only once)
	if (asidState.isShiftMode) {
		if (asidState.displayState != DISPLAY_ASID_SHIFT) {
			displayAsidShiftMode();
		}
	} else if (asidState.isCleanMode) {
		if (asidState.displayState != DISPLAY_ASID_CLEAN) {
			displayAsidCleanMode();
		}
	} else if (asidState.displayState != DISPLAY_ASID_REMIX) {
		displayAsidRemixMode();
	}

	// Default - no pitches received
	for (byte i = 0; i < SIDVOICES_PER_CHIP; i++) {
		pitchUpdate[i] = false;
	}

	// Clean out mask bytes to prepare for new values per chip
	for (chip = currentChip; chip <= max(currentChip, asidState.lastDuplicatedChip); chip++) {
		for (byte z = 0; z < 4; z++) {
			newMaskBytes[chip][z] = 0;
		}
	}
	// Build up the 8-bit data from scattered pieces
	// First comes four 7-bit mask bytes, followed by four 7-bit MSB bytes,
	// then the data according to enabled mask
	byte maskByte;
	for (maskByte = 0; maskByte < 4; maskByte++) {

		field = 0x01;
		for (byte bit = 0; bit < 7; bit++) {
			if ((buffer[maskByte] & field) == field) {
				// It is a hit. Build the complete data byte
				data = buffer[dataIdx++];
				if (buffer[maskByte + 4] & field) {
					// MSB was set
					data += 0x80;
				}

				assert(asidReg < SID_REGISTERS_ASID);

				// Get the mapped SID register
				reg = ASIDtoSIDregs[asidReg];

				// Save original values for later
				for (chip = currentChip; chip <= max(currentChip, asidState.lastDuplicatedChip); chip++) {
					asidState.lastSIDvalues[chip][reg] = data;
				}

				// Store SID parameters per chip (with modifications according
				// to performance controls, if applicable)
				if (reg < SID_REGISTERS_ASID) {
					regconfig_t* regConf = &(regConfig[reg]);
					if (regConf->isPitch) {
						// No need to store pitch data as we're updating it
						// from source later. Retune always needed due to
						// C64/TherapSID SID clock differences
						pitchUpdate[regConf->voice] = true;
					} else if (regConf->runRegFunction) {
						// The regular case, run a modification function and find
						// if value should be kept. New structure per chip created.
						for (chip = currentChip; chip <= max(currentChip, asidState.lastDuplicatedChip); chip++) {
							bool useData = true;
							byte modData = data;
							if (!asidState.isCleanMode) {
								useData = regConf->runRegFunction(chip, regConf->voice, &modData);
							}
							if (useData) {
								newMaskBytes[chip][maskByte] |= field;
								newSidData[chip][asidReg] = modData;
							}
						}
					}
				}
			}

			// Move to next register in ASID message
			field <<= 1;
			asidReg++;
		}
	}

	// Recalculate pitches
	for (chip = currentChip; chip <= max(currentChip, asidState.lastDuplicatedChip); chip++) {
		updatePitch(pitchUpdate, newMaskBytes[chip], newSidData[chip], chip);
	}

	// Send all updated SID registers
	for (chip = currentChip; chip <= max(currentChip, asidState.lastDuplicatedChip); chip++) {
		asidReg = 0;
		for (maskByte = 0; maskByte < 4; maskByte++) {
			field = 0x01;
			for (byte bit = 0; bit < 7; bit++) {
				if ((newMaskBytes[chip][maskByte] & field) == field) {
					sid_chips[chip].send_update_immediate(ASIDtoSIDregs[asidReg], newSidData[chip][asidReg]);
				}

				// Move to next register in ASID struct
				field <<= 1;
				asidReg++;
			}
		}
	}
}

void handleAsidFmFrameUpdate(byte* buffer) {
#define MAX_FM_REG_PAIRS 16

	byte numData = (buffer[0] + 1) << 1;
	byte numMaskBytes = (numData - 1) / 7 + 1;
	byte dataIdx = numMaskBytes + 1;
	byte addr, data, field;
	byte asidFmRegIdx = 0;
	int adjustData;

	static byte regs[MAX_FM_REG_PAIRS * 2];
	// clang-format off
	static byte regOpMapToVoice[] = 
		{0, 1, 2, 0 + OPL_NUM_CHANNELS_MELODY_MODE, 1 + OPL_NUM_CHANNELS_MELODY_MODE, 2 + OPL_NUM_CHANNELS_MELODY_MODE, 0, 0,
	     3, 4, 5, 3 + OPL_NUM_CHANNELS_MELODY_MODE, 4 + OPL_NUM_CHANNELS_MELODY_MODE, 5 + OPL_NUM_CHANNELS_MELODY_MODE, 0, 0,
	     6, 7, 8, 6 + OPL_NUM_CHANNELS_MELODY_MODE, 7 + OPL_NUM_CHANNELS_MELODY_MODE, 8 + OPL_NUM_CHANNELS_MELODY_MODE};
	// clang-format on

	for (byte maskByte = 0; maskByte < numMaskBytes; maskByte++) {
		field = 0x01;
		for (byte bit = 0; (bit < 7) && (asidFmRegIdx < numData); bit++) {
			data = buffer[dataIdx++];
			if ((buffer[1 + maskByte] & field) == field) {
				// MSB was set
				data += 0x80;
			}
			// Store and move to next register in ASID FM struct
			regs[asidFmRegIdx++] = data;
			field <<= 1;

			// Operate on the data when received a complete address/data pair
			if (asidFmRegIdx % 2 == 0) {
				addr = regs[asidFmRegIdx - 2];
				data = regs[asidFmRegIdx - 1];

				// Modify remixed parameters
				if (!asidState.isCleanMode) {
					if ((addr & 0xf0) == OPL_ADDRESS_BASE_KON_F_BLOCK) {
						// Key on/off - mute channel if needed
						if (asidState.muteFMChannel[addr & 0x0f]) {
							data = data & ~0x20;
						}
					} else if ((addr & 0xf0) == OPL_ADDRESS_BASE_FEEDBACK) {
						// Feedback
						adjustData = ((data & 0x0e) >> 1) + (asidState.adjustFMFeedback[addr & 0x0f] >> 6) - 8;
						adjustData = max(0, min(7, adjustData));
						asidState.lastFMvaluesFeedbackConn[addr - 0xc0] = data;
						data = (data & 0xf1) + (adjustData << 1);
					} else if ((addr >= OPL_ADDRESS_BASE_LEVELS) &&
					           (addr < (OPL_ADDRESS_BASE_LEVELS + OPL_SIZE_BLOCK_EXTENDED))) {
						// Levels (0x40 or 0x50)
						// Level is reversed...
						adjustData =
						    (data & 0x3f) - (asidState.adjustFMOpLevel[regOpMapToVoice[addr & 0x1f]] >> 3) + 64;
						adjustData = max(0, min(63, adjustData));
						asidState.lastFMvaluesKslTotalLev[addr - OPL_ADDRESS_BASE_LEVELS] = data;
						data = (data & 0xc0) + adjustData;
					}

					// Store any modified data back
					regs[asidFmRegIdx - 1] = data;
				}
			}
		}
	}

	// Write data to the FM chip
	for (byte reg = 0; reg < asidFmRegIdx; reg += 2) {
		addr = regs[reg];
		data = regs[reg + 1];
		sid_chips[1].send_update_immediate(OPL_REG_ADDRESS, regs[reg]);
		sid_chips[1].send_update_immediate(OPL_REG_DATA, regs[reg + 1]);
	}

	// Visualize the chip activity, when selected
	if (asidState.isSidFmMode && asidState.selectedSids.b.sid2) {
		for (byte reg = 0; reg < asidFmRegIdx; reg += 2) {
			addr = regs[reg];
			data = regs[reg + 1];
			if ((addr >= OPL_ADDRESS_BASE_KON_F_BLOCK) &&
			    (addr < (OPL_ADDRESS_BASE_KON_F_BLOCK + OPL_NUM_CHANNELS_MELODY_MODE))) {
				// Voice channels 0 to 8
				ledSet(1 + addr - OPL_ADDRESS_BASE_KON_F_BLOCK, (data & 0b00100000) > 0);
			} else if (addr == OPL_ADDRESS_RHYTHM && (data & 0x20)) {
				// Rhythm channels 0 to 5
				for (byte i = 0; i < 5; i++) {
					ledSet(7 + i, (data & (0x01 << i)) > 0);
				}
			}
		}
	}
}

/*
 * Main ASID message processor
 */
void asidProcessMessage(byte* buffer, int size) {

	// Setup SID duplication from first to second chip
	// If we get a message for the second/third SID or FM, then don't duplicate
	if (buffer[1] > 0x4f) {
		asidState.lastDuplicatedChip = 0;
		asidState.slowTimer = SLOW_TIMER_INIT_2_SEC;
	} else if (asidState.slowTimer == 0) {
		// No multiSID messages received for a long time => duplicate SIDs
		asidState.lastDuplicatedChip = 1;

		// Back to ARM2SID SID-only mode if needed
		if (asidState.isSidFmMode) {
			arm2SidSetSidFmMode(false);
			asidState.isSidFmMode = false;
			if (!asidState.isCleanMode) {
				displayAsidRemixMode();
			}
		}
		asidState.slowTimer = SLOW_TIMER_INIT_2_SEC;
	}

	// Message processor
	switch (buffer[1]) {
		case 0x4c:
			// Start play .SID
			// playback is controlled by incoming data 0x4e, but
			// if it was stopped before, it should restore remix
			if (asidState.enabled) {
				updateLastSIDValues(-1, 0, InitState::ALL);
				asidUpdateOverrides();
			}
			break;

		case 0x4d:
			// Stop playback
			// reset registers raw, so remix state is kept
			for (size_t reg = 0; reg < SID_REGISTERS; reg++) {
				for (byte chip = 0; chip < SIDCHIPS; chip++) {
					// take care about active fm chips > 0
					if (chip > 0 && asidState.isSidFmMode) {
						break;
					}
					sid_chips[chip].send_update_immediate(reg, 0);
				}
			}
			break;

		case 0x4f:
			// Display data on LCD
			// Not implemented.
			break;

		case 0x4e:
			// Update SID registers (standard ASID)
		case 0x50:
			// Update SID registers for second SID (extended ASID)
#if SIDCHIPS > 2
		case 0x51:
			// Update SID registers for third SID (extended ASID)
#endif
			if (size < 9) {
				// Must be at least one ID, 4 mask bytes and 4 msb bytes
#ifdef ASID_PROFILER
				panic(74, 1);
#endif
				return;
			}

			// Change ARM2SID mode to full SID mode if needed
			if (asidState.isSidFmMode && (buffer[1] != 0x4e)) {
				arm2SidSetSidFmMode(false);
				asidState.isSidFmMode = false;
				if (!asidState.isCleanMode) {
					displayAsidRemixMode();
				}
			}
			handleAsidFrameUpdate((buffer[1] == 0x4e ? 0 : buffer[1] - 0x4f), &buffer[2]);

			// Visualize changes - done after sound is produced to maximize good timing.
			visualizeSidLEDs();
			break;

		case 0x60:
			// Update FM (OPL) registers (extended ASID)

			// Change ARM2SID mode to SID+FM mode if needed
			if (!asidState.isSidFmMode) {
				arm2SidSetSidFmMode(true);
				asidState.isSidFmMode = true;
				if (!asidState.isCleanMode) {
					displayAsidRemixMode();
				}
			}
			handleAsidFmFrameUpdate(&buffer[2]);
			break;
	}
}

/*
 * Visualize everything on LEDs
 *
 * Will only actually show things if CPU load is low,
 * to not drop frames.
 */
void visualizeSidLEDs() {
	byte data, chip;

#ifdef ASID_PROFILER
	uint16_t sidTime;
	ATOMIC_BLOCK(ATOMIC_FORCEON) { sidTime = asidState.timer - cpuTimer; }
#endif

	// SID2 is shown if corresponding button held down by the user, otherwise SID1
	chip = asidState.selectedSids.b.sid2 && !asidState.selectedSids.b.sid1 ? 1 : 0;
#if SIDCHIPS > 2
	if (asidState.selectedSids.b.sid1 && asidState.selectedSids.b.sid2) {
		chip = 2;
	}
#endif

	// If FM mode is on, only first SID is valid
	if (asidState.isSidFmMode && (chip != 0)) {
		return;
	}

	// When most of the MIDI buffer is empty it is safe to spend time on updating LEDs
	if (Serial.available() < 4) {
		// Get the latest used SID register data
		data = sid_chips[chip].get_current_register(SID_FILTER_RESONANCE_ROUTING);
		showFilterResoLEDs(data);
		showFilterRouteLEDs(data);
		showFilterModeLEDs(sid_chips[chip].get_current_register(SID_FILTER_MODE_VOLUME));
		showFilterCutoffLEDs(sid_chips[chip].get_current_register(SID_FILTER_CUTOFF_HI));

		for (byte voice = 0; voice < 3; voice++) {
			showWaveformLEDs(voice, sid_chips[chip].get_current_register(SID_VC_CONTROL + (voice * 7)));
		}
	}

#ifdef ASID_PROFILER
	uint16_t ledTime;
	ATOMIC_BLOCK(ATOMIC_FORCEON) { ledTime = asidState.timer - cpuTimer; }

	if (sidTime > 0xff) {
		sidTime = 0xff;
	}
	if (sidTime > maxSidTime) {
		maxSidTime = sidTime;
	}

	if ((asidState.selectedSids.b.sid1 && asidState.selectedSids.b.sid2)) {
		ledHex(maxSidTime);
	} else if (asidState.selectedSids.b.sid1) {
		ledHex(sidTime);
	} else if (asidState.selectedSids.b.sid2) {
		ledHex(ledTime);
	} else if (asidState.selectedSids.b.dbg1) {
		if (variableIndex > 0) {
			variableIndex--;
		};
		ledHex(variableIndex);
		asidState.selectedSids.b.dbg1 = false;
	} else if (asidState.selectedSids.b.dbg2) {
		if (variableIndex < (sizeof(ptrVariables) / sizeof(*ptrVariables) - 1)) {
			variableIndex++;
		}
		ledHex(variableIndex);
		asidState.selectedSids.b.dbg2 = false;
	} else if (asidState.selectedSids.b.dbg3) {
		ledHex((*ptrVariables[variableIndex]) >> 8);
	} else if (asidState.selectedSids.b.dbg4) {
		ledHex((*ptrVariables[variableIndex]) & 0xff);
	}
#endif
}

FilterMode getFilterMode(byte data) {
	FilterMode fm;

	switch (data & 0b01110000) {
		case Sid::LOWPASS:
			fm = FilterMode::LOWPASS;
			break;
		case Sid::BANDPASS:
			fm = FilterMode::BANDPASS;
			break;
		case Sid::HIGHPASS:
			fm = FilterMode::HIGHPASS;
			break;
		case Sid::LOWPASS | Sid::HIGHPASS:
			fm = FilterMode::NOTCH;
			break;
		case Sid::LOWPASS | Sid::BANDPASS:
			fm = FilterMode::LB;
			break;
		case Sid::BANDPASS | Sid::HIGHPASS:
			fm = FilterMode::BH;
			break;
		case Sid::LOWPASS | Sid::BANDPASS | Sid::HIGHPASS:
			fm = FilterMode::LBH;
			break;
		default:
			fm = FilterMode::OFF;
			break;
	}

	return fm;
}

#ifdef ASID_VOLUME_ADJUST
/*
 * Adjust the volume according to remix-parameter
 * Note that on some 6581s this might give quite some pops
 */
void asidUpdateVolume(byte chip) {
	if (asidState.isCleanMode || (asidState.isSidFmMode && chip != 0)) {
		return;
	}

	// Save currently used filter mode for this chip (also used by same register)
	byte data = sid_chips[chip].get_current_register(SID_FILTER_MODE_VOLUME) & 0xf0;

	// Calc new adjusted volume
	int volume = (asidState.lastSIDvalues[chip][SID_FILTER_MODE_VOLUME] & 0x0f) + (asidState.adjustVolume[chip]) - 16;
	volume = max(0, min(15, volume));
	data |= volume;

	// Force update
	sid_chips[chip].send_update_immediate(SID_FILTER_MODE_VOLUME, data);
}
#endif

/*
 * Initialize SIDs filter mode
 */
void asidInitFilterMode(int chip) {

	byte first = chip > -1 ? chip : 0;
	byte last = chip > -1 ? chip : SIDCHIPS - 1;

	for (byte chip = first; chip <= last; chip++) {
		asidState.filterMode[chip] = FilterMode::OFF;
		asidState.isOverrideFilterMode[chip] = false;
		asidState.muteFilterMode[chip] = false;
	}
}

/*
 * Restore the filter mode
 */
void asidRestoreFilterMode(int chip) {
	asidInitFilterMode(chip);
	updateLastSIDValues(chip, 0, InitState::FILTER_MODE);

	byte first = chip > -1 ? chip : 0;
	showFilterModeLEDs(asidState.lastSIDvalues[first][SID_FILTER_MODE_VOLUME]);
}

/*
 * Adjust the filter mode according to remix-parameter, then display it
 */
void asidAdvanceFilterMode(byte chip, bool copyFirst) {
	if (asidState.isSidFmMode && chip != 0) {
		return;
	}

	// Get currently used filter mode for this chip (also including volume)
	byte data = sid_chips[chip].get_current_register(SID_FILTER_MODE_VOLUME);

	if (copyFirst) {
		asidState.filterMode[chip] = asidState.filterMode[0];
	} else {

		// get current filter mode (initial)
		if (!asidState.isOverrideFilterMode[chip]) {
			asidState.filterMode[chip] = getFilterMode(data);
		}

		// Increase the filter mode to next type
		asidSetNextFilterMode(chip);
	}

	asidState.isOverrideFilterMode[chip] = true;
	asidState.muteFilterMode[chip] = false;

	if (asidState.isCleanMode) {
		return;
	}

	// Calculate the correct new mode
	updateFilterMode(chip, &data);

	// Force updating the filter mode
	sid_chips[chip].send_update_immediate(SID_FILTER_MODE_VOLUME, data);

	showFilterModeLEDs(data);
}

/*
 * Adjust the filter cutoff according to remix-parameter, then display it
 */
void asidUpdateFilterCutoff(byte chip) {
	if (asidState.isCleanMode || (asidState.isSidFmMode && chip != 0)) {
		return;
	}

	byte data;

	if (asidState.isCutoffAdjustModeScaling) {
		// Scaling mode (factor 0 to 2)
		uint16_t cutoff = asidState.lastSIDvalues[chip][SID_FILTER_CUTOFF_HI] * (asidState.adjustCutoff[chip] >> 1);
		cutoff >>= 7;
		if (cutoff > 255) {
			cutoff = 255;
		}
		data = cutoff;
	} else {
		// Offset mode (adds/subtracts)
		int cutoff = asidState.lastSIDvalues[chip][SID_FILTER_CUTOFF_HI] + (asidState.adjustCutoff[chip]) - 256;
		cutoff = max(0, min(255, cutoff));
		data = cutoff;
	}

	sid_chips[chip].send_update_immediate(SID_FILTER_CUTOFF_HI, data);

	showFilterCutoffLEDs(data);
}

/*
 * Adjust the filter resonance according to remix-parameter, then display it
 */
void asidUpdateFilterReso(byte chip) {
	if (asidState.isCleanMode || (asidState.isSidFmMode && chip != 0)) {
		return;
	}

	int reso = (asidState.lastSIDvalues[chip][SID_FILTER_RESONANCE_ROUTING] >> 4) + (asidState.adjustReso[chip]) - 16;
	reso = max(0, min(0x0f, reso));

	byte data = (reso << 4) | (sid_chips[chip].get_current_register(SID_FILTER_RESONANCE_ROUTING) & 0x0f);

	sid_chips[chip].send_update_immediate(SID_FILTER_RESONANCE_ROUTING, data);

	showFilterResoLEDs(data);
}

/*
 * Adjust the filter routing according to remix-parameters, then display it
 */
void asidUpdateFilterRoute(byte chip, bool copyFirst) {
	byte src;

	if (asidState.isCleanMode || (asidState.isSidFmMode && chip != 0)) {
		return;
	}

	// Check if needs to duplicate the first chip's route
	if (copyFirst) {
		src = 0;
	} else {
		src = chip;
	}

	// Get the current filter route for the chip (to also include resonance)
	byte data = sid_chips[src].get_current_register(SID_FILTER_RESONANCE_ROUTING);

	// Update according to set route override states
	data = calculateFilterRoute(chip, data);
	sid_chips[chip].send_update_immediate(SID_FILTER_RESONANCE_ROUTING, data);

	if (!copyFirst) {
		showFilterRouteLEDs(data);
	}
}

/*
 * Adjust the pulse width to a fixed value according to remix-parameters
 */
void asidUpdateWidth(byte chip, byte voice) {
	if (asidState.isCleanMode || (asidState.isSidFmMode && chip != 0)) {
		return;
	}

	// Set width in span 0% to 50%
	int width = asidState.overridePW[chip][voice];
	asidState.isOverridePW[chip][voice] = true;

	// Limit the thinnest pulse, to still be audible (around 0.8%)
	width = max(30, width);

	sid_chips[chip].send_update_immediate(SID_VC_PULSE_WIDTH_HI + 7 * voice, width >> 8);
	sid_chips[chip].send_update_immediate(SID_VC_PULSE_WIDTH_LO + 7 * voice, width & 0xff);
}

/*
 * Adjust the feedback for a certain FM channel according to remix-parameters
 */
void asidFmUpdateFeedback(byte channel) {
	if (asidState.isCleanMode) {
		return;
	}

	int adjustData;
	byte data = asidState.lastFMvaluesFeedbackConn[channel];

	adjustData = ((data & 0x0e) >> 1) + (asidState.adjustFMFeedback[channel] >> 6) - 8;
	adjustData = max(0, min(7, adjustData));
	data = (data & 0xf1) + (adjustData << 1);

	sid_chips[1].send_update_immediate(OPL_REG_ADDRESS, OPL_ADDRESS_BASE_FEEDBACK + channel);
	sid_chips[1].send_update_immediate(OPL_REG_DATA, data);
}

/*
 * Adjust the level for a certain FM operator according to remix-parameters
 */
void asidFmUpdateOpLevel(byte oper) {
	if (asidState.isCleanMode) {
		return;
	}

	int adjustData;
	byte data = asidState.lastFMvaluesKslTotalLev[oper];
	static byte channelToReg[] = {0x40, 0x41, 0x42, 0x48, 0x49, 0x4a, 0x50, 0x51, 0x52,
	                              0x43, 0x44, 0x45, 0x4b, 0x4c, 0x4d, 0x53, 0x54, 0x55};

	adjustData = (data & 0x3f) - (asidState.adjustFMOpLevel[oper] >> 3) + 64;
	adjustData = max(0, min(63, adjustData));
	data = (data & 0xc0) + adjustData;

	sid_chips[1].send_update_immediate(OPL_REG_ADDRESS, channelToReg[oper]);
	sid_chips[1].send_update_immediate(OPL_REG_DATA, data);
}

/*
 * Indicate a remixed SID, by using the dots on the red LED display
 * Left = SID1, Right = SID2
 */
void asidIndicateChanged(byte chip) {
	bool isRemixed = false;

	// Verify if a certain parameter is remixed, within tolerances for some
	// clang-format off
	if (asidState.isSidFmMode && chip != 0) {
		for (byte i = 0; i < OPL_NUM_CHANNELS_MELODY_MODE; i++) {
			if (asidState.muteFMChannel[i] ||
				asidState.adjustFMOpLevel[i << 1] != POT_NOON ||
			    asidState.adjustFMOpLevel[(i << 1) + 1] != POT_NOON ||
				asidState.adjustFMFeedback[i] != POT_NOON) {
				isRemixed = true;
				break;
			}
		}

	} else {
		for (byte i = 0; i < SIDVOICES_PER_CHIP; i++) {
			if (asidState.isOverridePW[chip][i] ||
				asidState.muteChannel[chip][i] ||
				asidState.overrideWaveform[chip][i] != WaveformState::SIDFILE ||
			    asidState.overrideSync[chip][i] != OverrideState::SIDFILE ||
			    asidState.overrideRingMod[chip][i] != OverrideState::SIDFILE ||
				asidState.adjustOctave[chip][i] ||
			    asidState.adjustFine[chip][i] != FINETUNE_0_CENTS ||
			    asidState.adjustAttack[chip][i] != POT_VALUE_TO_ASID_LORES(POT_NOON) ||
			    asidState.adjustSustain[chip][i] != POT_VALUE_TO_ASID_LORES(POT_NOON) ||
			    asidState.adjustDecay[chip][i] != POT_VALUE_TO_ASID_LORES(POT_NOON) ||
			    asidState.adjustRelease[chip][i] != POT_VALUE_TO_ASID_LORES(POT_NOON) ||
			    asidState.overrideFilterRoute[chip][i] != OverrideState::SIDFILE) {
				isRemixed = true;
				break;
			}
		}
		if (!isRemixed &&
		    (asidState.adjustCutoff[chip] < POT_VALUE_TO_ASID_CUTOFF(POT_NOON) - 10 ||
		     asidState.adjustCutoff[chip] > POT_VALUE_TO_ASID_CUTOFF(POT_NOON) + 10 ||
		     asidState.adjustReso[chip] != POT_VALUE_TO_ASID_LORES(POT_NOON) ||
			 asidState.isOverrideFilterMode[chip])) {
			isRemixed = true;
		}
	}
	// clang-format on

	// update
	if (asidState.isRemixed[chip] != isRemixed) {
		asidState.isRemixed[chip] = isRemixed;
		dotSet(chip, isRemixed);
	}
}

/*
 * Restore specific pot values from original ASID state
 */
void asidRestorePot(int chip, byte voice, Pot potindex) {

	byte first = chip > -1 ? chip : 0;
	byte last = chip > -1 ? chip : SIDCHIPS - 1;

	for (byte chip = first; chip <= last; chip++) {

		switch (potindex) {
			case Pot::PW1:
			case Pot::PW2:
			case Pot::PW3:
				asidState.overridePW[chip][voice] = POT_VALUE_TO_ASID_PW(POT_NOON);
				asidState.isOverridePW[chip][voice] = false;
				break;

			case Pot::TUNE1:
			case Pot::TUNE2:
			case Pot::TUNE3:
				asidState.adjustOctave[chip][voice] = 0;
				break;

			case Pot::FINE1:
			case Pot::FINE2:
			case Pot::FINE3:
				asidState.adjustFine[chip][voice] = FINETUNE_0_CENTS;
				break;

			case Pot::ATTACK1:
			case Pot::ATTACK2:
			case Pot::ATTACK3:
				asidState.adjustAttack[chip][voice] = POT_VALUE_TO_ASID_LORES(POT_NOON);
				break;

			case Pot::DECAY1:
			case Pot::DECAY2:
			case Pot::DECAY3:
				asidState.adjustDecay[chip][voice] = POT_VALUE_TO_ASID_LORES(POT_NOON);
				break;

			case Pot::SUSTAIN1:
			case Pot::SUSTAIN2:
			case Pot::SUSTAIN3:
				asidState.adjustSustain[chip][voice] = POT_VALUE_TO_ASID_LORES(POT_NOON);
				break;

			case Pot::RELEASE1:
			case Pot::RELEASE2:
			case Pot::RELEASE3:
				asidState.adjustRelease[chip][voice] = POT_VALUE_TO_ASID_LORES(POT_NOON);
				break;

			case Pot::CUTOFF:
				asidState.adjustCutoff[chip] = POT_VALUE_TO_ASID_CUTOFF(POT_NOON);
				break;

			case Pot::RESONANCE:
				asidState.adjustReso[chip] = POT_VALUE_TO_ASID_LORES(POT_NOON);
				break;

			default:
				break;
		}
	}
}

/*
 * Toggles the two Cutoff Adjust Modes:
 * - Offset ("FO") adds/subtracts and is default
 * - Scaled ("FS") scales 0-200%
 */
void asidToggleCutoffAdjustMode(bool isPressed) {
	static bool isChanging = false;
	if (asidState.isCleanMode) {
		return;
	}

	if (isPressed) {
		if (!isChanging) {
			isChanging = true;
			asidState.isCutoffAdjustModeScaling = !asidState.isCutoffAdjustModeScaling;
			digit(0, DIGIT_F);
			if (asidState.isCutoffAdjustModeScaling) {
				digit(1, DIGIT_S);
			} else {
				digit(1, DIGIT_O);
			}
		}
	} else {
		isChanging = false;
		displayAsidRemixMode();
	}
}

/*
 * Called at 10kHz
 */
void asidTick() {
	asidState.timer++;
	if ((byte)asidState.timer == 0) {
		// Slow timer decreased at about 39Hz
		if (asidState.slowTimer > 0) {
			asidState.slowTimer--;
		}
	}
}

/*
 * Step the default remix chip up or down
 */
void asidAdvanceDefaultChip(bool isUp) {
	// Advance or decrement the default selected chip
	// (used when no chip selection button is pressed)
	asidState.defaultSelectedChip += (isUp ? 1 : -1);
	if (asidState.defaultSelectedChip >= SIDCHIPS) {
		asidState.defaultSelectedChip = -1;
	} else if (asidState.defaultSelectedChip < -1) {
		asidState.defaultSelectedChip = SIDCHIPS - 1;
	}

	// Lock the buttons accordingly
	asidState.selectedSids.b.sid1 = (asidState.defaultSelectedChip % 2 == 0);
	asidState.selectedSids.b.sid2 = (asidState.defaultSelectedChip >= 1);

	if (!asidState.isCleanMode) {
		displayAsidRemixMode();
	}
}

/*
 * Select the default remix chip
 */
void asidSelectDefaultChip(byte chip) {
	asidState.defaultSelectedChip = chip;

	// Lock the buttons accordingly
	asidState.selectedSids.b.sid1 = (asidState.defaultSelectedChip % 2 == 0);
	asidState.selectedSids.b.sid2 = (asidState.defaultSelectedChip >= 1);

	if (!asidState.isCleanMode) {
		displayAsidRemixMode();
	}
}

/*
 * Set the default remix chip as default (i.e both)
 */
void asidClearDefaultChip() {
	// If not already cleared, do it
	if (asidState.defaultSelectedChip != -1) {
		asidState.defaultSelectedChip = -1;
		asidState.selectedSids.b.sid1 = asidState.selectedSids.b.sid2 = false;

		if (!asidState.isCleanMode) {
			displayAsidRemixMode();
		}
	}
}

/*
 * Updates the override states which are independent of lastSIDvalues (like PW)
 */
void asidUpdateOverrides() {
	// recover override states to chips
	if (!asidState.isCleanMode) {

		for (byte chip = 0; chip <= SIDCHIPS - 1; chip++) {
			for (byte voice = 0; voice < 3; voice++) {
				if (asidState.isOverridePW[chip][voice]) {
					asidUpdateWidth(chip, voice);
				}
			}
		}
	}
}
