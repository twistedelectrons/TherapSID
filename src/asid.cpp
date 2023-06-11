#include "asid.h"
#include "ui_leds.h"
#include "sid.h"
#include <util/atomic.h>
#include "util.hpp"

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
#define FINETUNE_0_CENTS 31 + 978 // corresponds to -26 cents, to simulate PAL C64 on TS 1MHz

/*

 Main functionalites of the TherapSID ASID implementation
 ========================================================

 When receiving the first ASID protocol message, it will directly put the
 instrument in ASID mode (incicated by "AS"). Each message will instruct
 which registers in the SID chip to update.

 The incoming SID data is visualized the following way:
 * The relevant Voice LEDs lights up to show what waveforms and sync/ring mod
   states are used

 * The LFO LEDs are used to indicate filter status:
  - LINK 1, 2, 3 indicates that the voice is routed to the filter
  - Waveforms indicate the cutoff frequency as a 5-stage amount
  - RETRIG/LOOP indicate the resonance level as a 2-stage amount

 * The Filter type LEDs shows the currently active filter type


 In addition there is the possibility to remix the SID files live:
 * NOISE waveform button mutes/unmutes the corresponding track

 * RECT/TRI/SAW/SYNC/RING waveform buttons force that feature on/off (ignoring
   incoming changes from the SID file). Noise will always be let through.

 * WIDTH forces the pulse width to that value (ignoring incoming)

 * TUNE transposes the pitch +/- one octave. Noise will be left untransposed.

 * FINE finetunes the pitch +/-55 cents

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
   "AC"), not accepting any remixed parameters

 * Brief press on RESET restores remix parameters to original. This also works
   with the SID1/2 separation.

 * Long press on RESET jumps out of ASID mode


 */

/* */
void asidInit(int chip) {
	byte first, last;
	if (chip < 0) {
		first = 0;
		last = 1;
	} else {
		first = last = chip;
	}

	for (byte chip = first; chip <= last; chip++) {

		for (byte i = 0; i < SIDVOICES; i++) {
			asidState.muteChannel[chip][i] = false;

			asidState.overrideWaveform[chip][i] = WaveformState::SIDFILE;

			asidState.overrideSync[chip][i] = OverrideState::SIDFILE;
			asidState.overrideRingMod[chip][i] = OverrideState::SIDFILE;

			asidState.overridePW[chip][i] = POT_VALUE_TO_ASID_PW(POT_NOON);
			asidState.isOverridePW[chip][i] = false;
			asidState.adjustOctave[chip][i] = 0;
			asidState.adjustFine[chip][i] = FINETUNE_0_CENTS;

			asidState.adjustAttack[chip][i] = POT_VALUE_TO_ASID_LORES(POT_NOON);
			asidState.adjustSustain[chip][i] = POT_VALUE_TO_ASID_LORES(POT_NOON);
			asidState.adjustDecay[chip][i] = POT_VALUE_TO_ASID_LORES(POT_NOON);
			asidState.adjustRelease[chip][i] = POT_VALUE_TO_ASID_LORES(POT_NOON);

			asidState.overrideFilterRoute[chip][i] = OverrideState::SIDFILE;
		}

		asidState.filterMode[chip] = FilterMode::OFF;
		asidState.isOverrideFilterMode[chip] = false;

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
	asidState.slowTimer = 0;
	asidState.displayState = DISPLAY_STANDARD;
}

/*
 * Initialize SIDs to known state, used first time entering ASID mode
 */
void initializeSids() {
	for (size_t reg = 0; reg < sizeof(asidState.lastSIDvalues) / (sizeof(*asidState.lastSIDvalues)); reg++) {
		asidState.lastSIDvalues[reg] = 0;
	}
	asidState.lastSIDvalues[SID_FILTER_MODE_VOLUME] = 0x0F;
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

/*
 * Restore the most important/seldom changed registers, used when
 * pressing RESET briefly
 */
void asidRestore(int chip) {
	asidInit(chip);
	for (size_t reg = 0; reg < sizeof(asidState.lastSIDvalues) / (sizeof(*asidState.lastSIDvalues)); reg++) {
		if (chip <= 0) {
			sid_chips[0].send_update_immediate(reg, asidState.lastSIDvalues[reg]);
		}
		if ((chip < 0) || (chip == 1)) {
			sid_chips[1].send_update_immediate(reg, asidState.lastSIDvalues[reg]);
		}
	}
	showFilterModeLEDs(asidState.lastSIDvalues[SID_FILTER_MODE_VOLUME]);
	showFilterResoLEDs(asidState.lastSIDvalues[SID_FILTER_RESONANCE_ROUTING]);
	showFilterRouteLEDs(asidState.lastSIDvalues[SID_FILTER_RESONANCE_ROUTING]);
	showFilterCutoffLEDs(asidState.lastSIDvalues[SID_FILTER_CUTOFF_HI]);
}

void displayAsidRemixMode() {
	// "AS"
	digit(0, DIGIT_A);
	digit(1, DIGIT_S);
	asidState.displayState = DISPLAY_ASID_REMIX;
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
	for (byte voice = 0; voice < SIDVOICES; voice++) {
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
		default:
			break;
	}
}
void updatePitch(bool pitchUpdate[], byte maskBytes[], byte dataBytes[], byte sid) {
	for (byte i = 0; i < SIDVOICES; i++) {
		if (pitchUpdate[i]) {
			// Current original pitch
			long pitch = (asidState.lastSIDvalues[SID_VC_PITCH_HI + 7 * i] << 8) +
			             asidState.lastSIDvalues[SID_VC_PITCH_LO + 7 * i];

			// Calculate fine tune, about +/- 53 cents, range -80 to 28 cents to adjust
			// for C64 PAL vs TherapSID clock differences.
			int fine = asidState.isCleanMode ? FINETUNE_0_CENTS : asidState.adjustFine[sid][i];
			pitch = (pitch * fine) >> 10;

			// Change octave if not a pure noise playing
			if (!asidState.isCleanMode && ((asidState.lastSIDvalues[SID_VC_CONTROL + 7 * i] & 0xF0) != 0x80)) {
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
	if (asidState.isOverrideFilterMode[chip]) {
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

/*
 * Main ASID message processor
 */
void asidProcessMessage(byte* buffer, int size) {
	// Location of real SID register within the ASID package
	const byte ASIDtoSIDregs[SID_REGISTERS_ASID] = {
	    0x00, 0x01, 0x02, 0x03, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0c, 0x0d, 0x0e, 0x0f,
	    0x10, 0x11, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x04, 0x0b, 0x12, 0x04, 0x0b, 0x12,
	};

	static byte newMaskBytes[SIDCHIPS][4];
	static byte newSidData[SIDCHIPS][SID_REGISTERS_ASID];
	typedef bool (*runRegFunc_t)(byte chip, byte voice, byte* data);

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

#ifdef ASID_PROFILER
	static byte maxSidTime;
	static uint16_t ticksBetweenMsg;
	static uint16_t minTicksBetweenMsg;
	static uint16_t maxTicksBetweenMsg;
	static uint16_t cpuTimer;
	static uint16_t* ptrVariables[] = {&ticksBetweenMsg, &minTicksBetweenMsg, &maxTicksBetweenMsg};
	static byte variableIndex = 0;
#endif

	if (size < 9) {
		// Must be at least one ID, 4 mask bytes and 4 msb bytes
#ifdef ASID_PROFILER
		panic(74, 1);
#endif
		return;
	}

	switch (buffer[1]) {
		case 0x4c:
			// Start play .SID
			break;

		case 0x4d:
			// Stop playback
			// Not implemented.
			break;

		case 0x4f:
			// Display data on LCD
			// Not implemented.
			break;

		case 0x4e:
			// Update SID registers

			byte asidReg = 0;         // ASID register location in buffer
			byte dataIdx = 2 + 4 + 4; // Data location in buffer (header, masks, msbs first)
			byte data, reg, field, chip;
			bool pitchUpdate[SIDVOICES];

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
			if (asidState.isCleanMode) {
				if (asidState.displayState != DISPLAY_ASID_CLEAN) {
					displayAsidCleanMode();
				}
			} else if (asidState.displayState != DISPLAY_ASID_REMIX) {
				displayAsidRemixMode();
			}

			// Default - no pitches received
			for (byte i = 0; i < SIDVOICES; i++) {
				pitchUpdate[i] = false;
			}

			// Clean out mask bytes to prepare for new values per chip
			for (chip = 0; chip < SIDCHIPS; chip++) {
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
					if ((buffer[2 + maskByte] & field) == field) {
						// It is a hit. Build the complete data byte
						data = buffer[dataIdx++];
						if (buffer[2 + maskByte + 4] & field) {
							// MSB was set
							data += 0x80;
						}

						assert(asidReg < SID_REGISTERS_ASID);

						// Get the mapped SID register
						reg = ASIDtoSIDregs[asidReg];

						// Save original values for later
						asidState.lastSIDvalues[reg] = data;

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
								for (chip = 0; chip < SIDCHIPS; chip++) {
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
			for (chip = 0; chip < SIDCHIPS; chip++) {
				updatePitch(pitchUpdate, newMaskBytes[chip], newSidData[chip], chip);
			}

			// Send all updated SID registers
			for (chip = 0; chip < SIDCHIPS; chip++) {
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

#ifdef ASID_PROFILER
			uint16_t sidTime;
			ATOMIC_BLOCK(ATOMIC_FORCEON) { sidTime = asidState.timer - cpuTimer; }
#endif

			// Visualize everything - done after sound is produced to maximize good timing.
			// SID2 is shown if corresponding button held down by the user, otherwise SID1
			chip = asidState.selectedSids.b.sid2 && !asidState.selectedSids.b.sid1 ? 1 : 0;

			// Refresh some seldom used registers, to display when SID select buttons held
			if (asidState.slowTimer == 0) {
				asidState.slowTimer = 4; // 39Hz/4 => 100ms response time
				// Get the latest used SID register data
				newSidData[chip][20] = sid_chips[chip].get_current_register(SID_FILTER_RESONANCE_ROUTING);
				newSidData[chip][21] = sid_chips[chip].get_current_register(SID_FILTER_MODE_VOLUME);
				newSidData[chip][19] = sid_chips[chip].get_current_register(SID_FILTER_CUTOFF_HI);
				// Force masks on corresponding to the above
				newMaskBytes[chip][2] |= 0x60;
				newMaskBytes[chip][3] |= 0x01;
			}

			// Do the actual LED update
			asidReg = 0;
			for (maskByte = 0; maskByte < 4; maskByte++) {
				field = 0x01;
				for (byte bit = 0; bit < 7; bit++) {
					if ((newMaskBytes[chip][maskByte] & field) == field) {
						reg = ASIDtoSIDregs[asidReg];
						data = newSidData[chip][asidReg];
						if (reg == SID_FILTER_MODE_VOLUME) {
							showFilterModeLEDs(data);
						} else if (reg == SID_FILTER_RESONANCE_ROUTING) {
							showFilterResoLEDs(data);
							showFilterRouteLEDs(data);
						} else if (reg == SID_FILTER_CUTOFF_HI) {
							showFilterCutoffLEDs(data);
						} else if ((reg - SID_VC_CONTROL) % 7 == 0) {
							showWaveformLEDs((reg - SID_VC_CONTROL) / 7, data);
						}
					}

					// Move to next register in ASID struct
					field <<= 1;
					asidReg++;
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
			break;
	}
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
	if (asidState.isCleanMode) {
		return;
	}

	// Save currently used filter mode for this chip (also used by same register)
	byte data = sid_chips[chip].get_current_register(SID_FILTER_MODE_VOLUME) & 0xf0;

	// Calc new adjusted volume
	int volume = (asidState.lastSIDvalues[SID_FILTER_MODE_VOLUME] & 0x0f) + (asidState.adjustVolume[chip]) - 16;
	volume = max(0, min(15, volume));
	data |= volume;

	// Force update
	sid_chips[chip].send_update_immediate(SID_FILTER_MODE_VOLUME, data);
}
#endif

/*
 * Adjust the filter mode according to remix-parameter, then display it
 */
void asidAdvanceFilterMode(byte chip, bool copyFirst) {
	if (asidState.isCleanMode) {
		return;
	}

	asidState.isOverrideFilterMode[chip] = true;

	// Get currently used filter mode for this chip (also including volume)
	byte data = sid_chips[chip].get_current_register(SID_FILTER_MODE_VOLUME);

	if (copyFirst) {
		asidState.filterMode[chip] = asidState.filterMode[0];
	} else {
		// Increase the filter mode to next type
		asidState.filterMode[chip] = getFilterMode(data);
		asidState.filterMode[chip] = static_cast<FilterMode>((static_cast<int>(asidState.filterMode[chip]) + 1) % 5);
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
	if (asidState.isCleanMode) {
		return;
	}

	byte data;

	if (asidState.isCutoffAdjustModeScaling) {
		// Scaling mode (factor 0 to 2)
		uint16_t cutoff = asidState.lastSIDvalues[SID_FILTER_CUTOFF_HI] * (asidState.adjustCutoff[chip] >> 1);
		cutoff >>= 7;
		if (cutoff > 255) {
			cutoff = 255;
		}
		data = cutoff;
	} else {
		// Offset mode (adds/subtracts)
		int cutoff = asidState.lastSIDvalues[SID_FILTER_CUTOFF_HI] + (asidState.adjustCutoff[chip]) - 256;
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
	if (asidState.isCleanMode) {
		return;
	}

	int reso = (asidState.lastSIDvalues[SID_FILTER_RESONANCE_ROUTING] >> 4) + (asidState.adjustReso[chip]) - 16;
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

	if (asidState.isCleanMode) {
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
	if (asidState.isCleanMode) {
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
 * Indicate a remixed SID, by using the dots on the red LED display
 * Left = SID1, Right = SID2
 */
void asidIndicateChanged(byte chip) {
	if (!asidState.isCleanMode) {
		dotSet(chip, true);
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

/* Called at 10kHz */
void asidTick() {
	asidState.timer++;
	if ((byte)asidState.timer == 0) {
		// Slow timer decreased at about 39Hz
		if (asidState.slowTimer > 0) {
			asidState.slowTimer--;
		}
	}
}
