#include "globals.h"
#include "ui_pots.h"
#include "midi.h"
#include "ui_vars.h"
#include "asid.h"

static int arpDivisions[] = {1, 3, 6, 8, 12, 24, 32, 48};

// Maps hardware potentiometer number to other properties such as MIDI CC and mapping
struct potMap {
	Pot hwPot;
	byte index;
	byte midiCC;
	byte bindID;
	int8_t fmOperator; // Only used in FM mode. -1 means no operator. Values over 18 is feedback
};

const potMap potMaps[] = {
    {Pot::PW1, 0, 2, 0, 0},           {Pot::PW2, 1, 10, 3, 4},          {Pot::PW3, 2, 18, 6, 8},
    {Pot::TUNE1, 0, 3, 1, 1},         {Pot::TUNE2, 1, 11, 4, 5},        {Pot::TUNE3, 2, 19, 7, -1},
    {Pot::FINE1, 0, 4, 2, 2},         {Pot::FINE2, 1, 12, 5, 6},        {Pot::FINE3, 2, 20, 8, -1},
    {Pot::GLIDE1, 0, 5, 20, 3},       {Pot::GLIDE2, 1, 13, 20, 7},      {Pot::GLIDE3, 2, 21, 20, -1},
    {Pot::ATTACK1, 0, 6, 20, 9},      {Pot::ATTACK2, 1, 14, 20, 13},    {Pot::ATTACK3, 2, 22, 20, 17},
    {Pot::DECAY1, 0, 7, 20, 10},      {Pot::DECAY2, 1, 15, 20, 14},     {Pot::DECAY3, 2, 23, 20, -1},
    {Pot::SUSTAIN1, 0, 8, 20, 11},    {Pot::SUSTAIN2, 1, 16, 20, 15},   {Pot::SUSTAIN3, 2, 24, 20, -1},
    {Pot::RELEASE1, 0, 9, 20, 12},    {Pot::RELEASE2, 1, 17, 20, 16},   {Pot::RELEASE3, 2, 25, 20, -1},
    {Pot::LFO_RATE1, 0, 26, 9, 18},   {Pot::LFO_RATE2, 1, 28, 11, 20},  {Pot::LFO_RATE3, 2, 30, 13, 22},
    {Pot::LFO_DEPTH1, 0, 27, 10, 19}, {Pot::LFO_DEPTH2, 1, 29, 12, 21}, {Pot::LFO_DEPTH3, 2, 31, 14, 23},
    {Pot::CUTOFF, 0, 59, 15, -1},     {Pot::RESONANCE, 0, 33, 16, -1},  {Pot::ARP_RANGE, 0, 36, 19, 26},
    {Pot::ARP_RATE, 0, 35, 18, 25},   {Pot::ARP_SCRUB, 0, 34, 17, 24},
};

const potMap* getPotMap(Pot* pot) {
	for (byte i = 0; i < sizeof(potMaps) / sizeof(*potMaps); i++) {
		if (potMaps[i].hwPot == *pot) {
			return &(potMaps[i]);
		}
	}
	return NULL;
}

static byte scale4bit(int input) { return (input >> 6); }

static byte octScale(int value) {
	value = map(value, 0, 1023, 0, 25);
	if (value > 24)
		value = 24;
	return (value);
}

void movedPotAsid(Pot pot, int value) {
	const potMap* potmap = getPotMap(&pot);

	bool useChip[] = {
	    (asidState.selectedSids.all == 0) || (asidState.selectedSids.b.sid1 && !asidState.selectedSids.b.sid2),
	    (asidState.selectedSids.all == 0) || (asidState.selectedSids.b.sid2 && !asidState.selectedSids.b.sid1),
	    (asidState.selectedSids.all == 0) || (asidState.selectedSids.b.sid1 && asidState.selectedSids.b.sid2)};

	if (asidState.isSidFmMode && asidState.selectedSids.b.sid2 && !asidState.selectedSids.b.sid1) {
		if (potmap->fmOperator != -1) {
			if (potmap->fmOperator < OPL_NUM_CHANNELS_MELODY_MODE * OPL_NUM_OPERATORS) {
				asidState.adjustFMOpLevel[potmap->fmOperator] = POT_VALUE_TO_ASID_FM_HIRES(value);
				asidFmUpdateOpLevel(potmap->fmOperator);
			} else {
				asidState.adjustFMFeedback[potmap->fmOperator - OPL_NUM_CHANNELS_MELODY_MODE * OPL_NUM_OPERATORS] =
				    POT_VALUE_TO_ASID_FM_LORES(value);
				asidFmUpdateFeedback(potmap->fmOperator - OPL_NUM_CHANNELS_MELODY_MODE * OPL_NUM_OPERATORS);
			}
		}
	} else {

		for (byte i = 0; i < SIDCHIPS; i++) {
			// Run once for each chip if needed
			if (useChip[i]) {

				if (asidState.isShiftMode) {
					asidRestorePot(asidState.selectedSids.all == 0 ? -1 : i, potmap->index, pot);
					continue;
				}

				switch (pot) {
					case Pot::PW1:
					case Pot::PW2:
					case Pot::PW3:
						asidState.overridePW[i][potmap->index] = POT_VALUE_TO_ASID_PW(value);
						asidUpdateWidth(i, potmap->index);
						break;

					case Pot::ATTACK1:
					case Pot::ATTACK2:
					case Pot::ATTACK3:
						asidState.adjustAttack[i][potmap->index] = POT_VALUE_TO_ASID_LORES(value);
						break;

					case Pot::DECAY1:
					case Pot::DECAY2:
					case Pot::DECAY3:
						asidState.adjustDecay[i][potmap->index] = POT_VALUE_TO_ASID_LORES(value);
						break;

					case Pot::SUSTAIN1:
					case Pot::SUSTAIN2:
					case Pot::SUSTAIN3:
						asidState.adjustSustain[i][potmap->index] = POT_VALUE_TO_ASID_LORES(value);
						break;

					case Pot::RELEASE1:
					case Pot::RELEASE2:
					case Pot::RELEASE3:
						asidState.adjustRelease[i][potmap->index] = POT_VALUE_TO_ASID_LORES(value);
						break;

					case Pot::CUTOFF:
						asidState.adjustCutoff[i] = POT_VALUE_TO_ASID_CUTOFF(value);
						asidUpdateFilterCutoff(i);
						break;

					case Pot::RESONANCE:
						asidState.adjustReso[i] = POT_VALUE_TO_ASID_LORES(value);
						asidUpdateFilterReso(i);
						break;

#ifdef ASID_VOLUME_ADJUST
					case Pot::ARP_SCRUB:
						asidState.adjustVolume[i] = POT_VALUE_TO_ASID_LORES(value);
						asidUpdateVolume(i);
#endif
						break;

					case Pot::TUNE1:
					case Pot::TUNE2:
					case Pot::TUNE3: {
						// Octaves, -1, 0, +1: even spaced
						int8_t octave;

						if (value < 1 * 1023 / 3) {
							octave = -1;
						} else if (value < 2 * 1023 / 3) {
							octave = 0;
						} else {
							octave = 1;
						}
						asidState.adjustOctave[i][potmap->index] = octave;

					} break;

					case Pot::FINE1:
					case Pot::FINE2:
					case Pot::FINE3:
						asidState.adjustFine[i][potmap->index] = POT_VALUE_TO_ASID_FINETUNE(value);
						break;

					default:
						// nothing to do
						break;
				}
			}
		}
	}

	// Update dot indication for changed SIDs, if needed
	// Only two dots exists, so shows max two
	for (byte i = 0; i < 2; i++) {
		asidIndicateChanged(i);
	}
}

void movedPot(byte number, int value, bool isMidi) {
	Pot pot = static_cast<Pot>(number);
	if (asidState.enabled) {
		return movedPotAsid(pot, value);
	}

	if (!ui_state.saveMode) {
		const potMap* potmap = getPotMap(&pot);

		switch (pot) {
			case Pot::PW1:
			case Pot::PW2:
			case Pot::PW3:
				preset_data.voice[potmap->index].pulsewidth_base = value << 1;
				break;

			case Pot::TUNE1:
			case Pot::TUNE2:
			case Pot::TUNE3:
				preset_data.voice[potmap->index].tune_base = octScale(value);
				break;

			case Pot::FINE1:
			case Pot::FINE2:
			case Pot::FINE3:
				preset_data.voice[potmap->index].fine_base = value / 1023.f;
				break;

			case Pot::GLIDE1:
			case Pot::GLIDE2:
			case Pot::GLIDE3:
				preset_data.voice[potmap->index].glide = value >> 4;
				break;

			case Pot::ATTACK1:
			case Pot::ATTACK2:
			case Pot::ATTACK3:
				preset_data.voice[potmap->index].attack = scale4bit(value);
				if (pot == Pot::ATTACK3) {
					a4 = value;
				}
				break;

			case Pot::DECAY1:
			case Pot::DECAY2:
			case Pot::DECAY3:
				preset_data.voice[potmap->index].decay = scale4bit(value);
				if (pot == Pot::DECAY3) {
					d4 = value;
				}
				break;

			case Pot::SUSTAIN1:
			case Pot::SUSTAIN2:
			case Pot::SUSTAIN3:
				if (pot == Pot::SUSTAIN3) {
					s4 = value >> 2; // Will map to max 255
				}
				preset_data.voice[potmap->index].sustain = scale4bit(value);
				break;

			case Pot::RELEASE1:
			case Pot::RELEASE2:
			case Pot::RELEASE3:
				if (pot == Pot::RELEASE3) {
					r4 = value;
				}
				preset_data.voice[potmap->index].release = scale4bit(value);
				break;

			case Pot::LFO_RATE1:
			case Pot::LFO_RATE2:
			case Pot::LFO_RATE3:
				lfoClockSpeedPending[potmap->index] = 1 + (value >> 7);
				preset_data.lfo[potmap->index].speed = value * 1.3;
				ui_state.selectedLfo = potmap->index;
				break;

			case Pot::LFO_DEPTH1:
			case Pot::LFO_DEPTH2:
			case Pot::LFO_DEPTH3:
				preset_data.lfo[potmap->index].depth = value;
				ui_state.selectedLfo = potmap->index;
				break;

			case Pot::CUTOFF:
				preset_data.cutoff = value;
				break;

			case Pot::RESONANCE:
				preset_data.resonance_base = scale4bit(value);
				break;

			case Pot::ARP_SCRUB:
				if (ui_state.filterModeHeld) {
					filterAssignmentChanged = true;
					volume = value >> 6;
					if (!volume)
						volume++;
					volumeChanged = true;
				} else {
					if (voice_state.n_held_keys() >= 1) {
						arpStepBase = value >> 2;
					}
				}
				break;

			case Pot::ARP_RATE:
				preset_data.arp_speed_base = (1023 - value) >> 2;
				preset_data.arp_rate = arpDivisions[preset_data.arp_speed_base >> 5];
				break;

			case Pot::ARP_RANGE:
				preset_data.arp_range_base = value >> 8;
				break;
		}

		// Send the MIDI CC message for the knob if used in the regular case
		if (!isMidi && !(pot == Pot::ARP_SCRUB && ui_state.filterModeHeld)) {
			sendCC(potmap->midiCC, value);
			ui_state.lastPot = potmap->bindID;
		}
	}
}
