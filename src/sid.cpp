#include "globals.h"
#include "leds.h"
#include "sid.h"

static const int32_t sidScale[] = {
    137,   145,   154,   163,   173,   183,   194,   205,   217,   230,   122,   259,   274,   291,   308,
    326,   346,   366,   388,   411,   435,   461,   489,   518,   549,   581,   616,   652,   691,   732,
    776,   822,   871,   923,   976,   1036,  1097,  1163,  1232,  1305,  1383,  1465,  1552,  1644,  1742,
    1845,  1955,  2071,  2195,  2325,  2463,  2610,  2765,  2930,  3104,  3288,  3484,  3691,  3910,  4143,
    4389,  4650,  4927,  5220,  5530,  5859,  6207,  6577,  6968,  7382,  7821,  8286,  8779,  9301,  9854,
    10440, 11060, 11718, 12415, 13153, 13935, 14764, 15642, 16572, 17557, 18601, 19709, 20897, 22121, 23436,
    24830, 26306, 27871, 29528, 31234, 33144, 35115, 37203, 39415, 41759, 44242, 46873, 49660, 52613, 55741,
    59056, 62567, 33144, 35115, 37203, 39415, 41759, 44242, 46873, 49660, 52613, 55741, 59056, 62567, 33144,
    35115, 37203, 39415, 41759, 44242, 46873, 49660, 52613, 55741, 59056, 62567,

};

// COMS WITH SID CHIP

void sidReset() {
	digitalWrite(A6, LOW);
	delay(50);
	digitalWrite(A6, HIGH);
}

void init1MhzClock() {
	DDRD |= _BV(7); // SID CLOCK

	// Set Timer 2 CTC mode with no prescaling.  OC2A toggles on compare match
	//
	// WGM22:0 = 010: CTC Mode, toggle OC
	// WGM2 bits 1 and 0 are in TCCR2A,
	// WGM2 bit 2 is in TCCR2B
	// COM2A0 sets OC2A (arduino pin 11 on Uno or Duemilanove) to toggle on compare match
	//
	TCCR2A = ((1 << WGM21) | (1 << COM2A0));

	// Set Timer 2  No prescaling  (i.e. prescale division = 1)
	//
	// CS22:0 = 001: Use CPU clock with no prescaling
	// CS2 bits 2:0 are all in TCCR2B
	TCCR2B = (1 << CS20);

	// Make sure Compare-match register A interrupt for timer2 is disabled
	TIMSK2 = 0;
	// This value determines the output frequency
	OCR2A = 7;
}

struct pair {
	int first;
	int second;
};

static void write_mask(uint8_t& value, uint8_t mask, uint8_t new_value) {
	value = (value & (~mask)) | (new_value & mask);
}
		
enum VoiceRegister {
	FREQ_LO = 0,
	FREQ_HI,
	PULSEWIDTH_LO,
	PULSEWIDTH_HI,
	CONTROL,
	ATTACK_DECAY,
	SUSTAIN_RELEASE
};

enum Register {
	FREQ1_LO = 0,
	FREQ1_HI,
	PULSEWIDTH1_LO,
	PULSEWIDTH1_HI,
	CONTROL1,
	ATTACK_DECAY1,
	SUSTAIN_RELEASE1,

	FREQ2_LO,
	FREQ2_HI,
	PULSEWIDTH2_LO,
	PULSEWIDTH2_HI,
	CONTROL2,
	ATTACK_DECAY2,
	SUSTAIN_RELEASE2,

	FREQ3_LO,
	FREQ3_HI,
	PULSEWIDTH3_LO,
	PULSEWIDTH3_HI,
	CONTROL3,
	ATTACK_DECAY3,
	SUSTAIN_RELEASE3,

	FILTER_CUTOFF_LO,
	FILTER_CUTOFF_HI,
	FILTER_RESONANCE_ROUTING,
	FILTER_MODE_VOLUME,

	POTX,
	POTY,
	OSC3_RANDOM,
	ENV3
};


Sid::Sid(int chip_enable_bit) : chip_enable_bit(chip_enable_bit) {
	for (auto& r : registers)
		r = 0;
	registers[FILTER_CUTOFF_LO] = 0xFF;
	registers[FILTER_CUTOFF_HI] = 0xFF;
	registers[FILTER_RESONANCE_ROUTING] = 0xFF;
	registers[FILTER_MODE_VOLUME] = 0x11; // Filter off full vol
}

/// Updates the next pair of changed registers. Call this in your main loop.
void Sid::send_next_update_pair() {
	// some get sent twice just because it's easier to not special-case them.
	pair send_order[] = { {1, 0}, {3,2}, {4,5}, {6, 13}, {8, 7}, {10,9}, {11,12}, {15,14}, {17, 16}, {18,19}, {20, 23}, {22, 21}, {24, 24} };
	const size_t SEND_ORDER_LEN = sizeof(send_order) / sizeof(*send_order);

	for (size_t i = 0; i < SEND_ORDER_LEN; i++) {
		bool updated = false;
		updated |= maybe_update_register(send_order[next_pair].first);
		updated |= maybe_update_register(send_order[next_pair].second);
		next_pair = (next_pair + 1) % SEND_ORDER_LEN;

		if (next_pair == 0) {
			force_initial_update = false;
		}

		if (updated) {
			return;
		}
	}
}

void Sid::set_freq(int voice, uint16_t value) {
	registers[7*voice + FREQ_LO] = value & 0xFF;
	registers[7*voice + FREQ_HI] = value >> 8;
}

/// `value` needs to be OR-ed together out of NOISE, PULSE, SAW and/or TRI.
/// Note that enabling NOISE together with any other waveform can clear the
/// noise shift register inside the chip.
void Sid::set_shape(int voice, byte value) {
	registers[7*voice + CONTROL] &= 0x0f;
	registers[7*voice + CONTROL] |= value;
}

byte Sid::shape(int voice) const {
	return registers[7*voice + CONTROL] & 0xf0;
}

void Sid::set_filter_mode(uint8_t value) {
	write_mask(registers[FILTER_MODE_VOLUME], HIGHPASS | BANDPASS | LOWPASS, value);
}

uint8_t Sid::filter_mode() {
	return registers[FILTER_MODE_VOLUME] & (HIGHPASS | BANDPASS | LOWPASS);
}

/// In addition to the usual 3 voices, this accepts voice==3 for the external input.
void Sid::set_voice_filter(int voice, bool enable) {
	bitWrite(registers[FILTER_RESONANCE_ROUTING], voice, enable);
}

bool Sid::is_voice_playing(size_t voice) {
	return registers[7*voice + CONTROL] & 1;
}

bool Sid::is_update_allowed(size_t register_index) {
	if (register_index == SUSTAIN_RELEASE1 || register_index == SUSTAIN_RELEASE2 || register_index == SUSTAIN_RELEASE3) {
		auto voice = register_index / 7;
		return !is_voice_playing(voice);
	}
	else {
		return true;
	}
}

/// Updates a SID register if it has been changed, and if it's currently allowed to update it.
bool Sid::maybe_update_register(size_t index) {
	bool has_changed = registers_sent[index] == registers[index];
	bool is_allowed = is_update_allowed(index);
	if ((has_changed && is_allowed) || force_initial_update) {
		registers_sent[index] = registers[index];
		send(index, registers[index]);
		return true;
	}
	return false;
}

void Sid::send(size_t index, int data) {
	PORTC = index << 3;
	PORTB = data;
	delayMicroseconds(4);

	PORTD |= _BV(3);
	PORTD &= ~_BV(chip_enable_bit);
	delayMicroseconds(4);

	PORTD |= _BV(chip_enable_bit);
	PORTD &= ~_BV(3);
	delayMicroseconds(4);
}


Sid sid_chips[2] = {Sid(_BV(6)), Sid(_BV(2))};



void ledUpdateShape() {
	// update leds
	for (int voice = 0; voice < 3; voice++) {
		ledSet(7*voice + 1, sid_chips[0].shape(voice) & Sid::PULSE);
		ledSet(7*voice + 2, sid_chips[0].shape(voice) & Sid::TRI);
		ledSet(7*voice + 3, sid_chips[0].shape(voice) & Sid::SAW);
		ledSet(7*voice + 4, sid_chips[0].shape(voice) & Sid::NOISE);
	}
}

void ledUpdateFilterMode() {
	// FIXME: do not use sid registers for GUI purposes
	ledSet(27, sid_chips[0].filter_mode() & Sid::LOWPASS);
	ledSet(28, sid_chips[0].filter_mode() & Sid::BANDPASS);
	ledSet(29, sid_chips[0].filter_mode() & Sid::HIGHPASS);
}



static uint16_t fat_pitch(uint16_t pitch, FatMode fatMode) {
	switch (fatMode) {
		case FatMode::UNISONO:
			return pitch;
		case FatMode::OCTAVE_UP:
			if (pitch < 0xffff / 2) {
				return pitch * 2;
			}
			else {
				return pitch;
			}
		case FatMode::DETUNE_SLIGHT:
			return pitch - 15;
		case FatMode::DETUNE_MUCH:
			return pitch - 50;
		default:
			return pitch;
	}
}

void sidPitch(byte voice, int pitch) {
	sid_chips[0].set_freq(voice, pitch);
	sid_chips[1].set_freq(voice, fat_pitch(pitch, fatMode));
}

void sidShape(byte voice, byte shape, bool value) {
	if (shape <= 1 || shape > 4) return;
	const byte mapping[] = { 0, Sid::PULSE, Sid::TRI, Sid::SAW, Sid::NOISE };
	byte shape_mask = mapping[shape];

	for (auto& chip : sid_chips) {
		auto new_value = (chip.shape(voice) & (~shape_mask)) | (value? shape_mask : 0);

		// Make sure that noise is not enabled together with any other waveform.
		if (value) {
			if (shape_mask == Sid::NOISE) {
				// If the noise waveform has been turned on, disable the other three.
				new_value &= ~(Sid::TRI | Sid::SAW | Sid::PULSE);
			}
			else {
				// If one of the other three waveforms was enabled, make sure to disable noise.
				new_value &= ~Sid::NOISE;
			}
		}

		chip.set_shape(voice, new_value);
	}

	ledUpdateShape(); // FIXME
}

// FIXME parameter
void updateFilter() {
	// assert((unsigned) filterMode < 5); // FIXME

	uint8_t filter_mapping[] = { Sid::LOWPASS, Sid::BANDPASS, Sid::HIGHPASS, Sid::LOWPASS | Sid::HIGHPASS, 0 };

	for (auto& chip : sid_chips) {
		chip.set_filter_mode(filter_mapping[(unsigned)filterMode]);
		chip.set_voice_filter(3, filterMode != FilterMode::OFF);

		// Disable voice->filter routing if voice is off or filter is off.
		for (int voice = 0; voice < 3; voice++) {
			if (chip.shape(voice) == 0 || filterMode == FilterMode::OFF) {
				chip.set_voice_filter(voice, false);
			}
		}
	}

	ledUpdateFilterMode();
}


static int calc_pitch(int note, float frac) {
	if (note > 127) {
		note = 127;
	} else if (note - 13 < 0) {
		note = 13;
	}

	float fine = frac / 2 + 0.5;
	fine *= .90;

	return sidScale[note - 12 - 1] + (sidScale[note - 12 + 2] - sidScale[note - 12]) * fine;
}


static void updateDestiPitches(int note1, int note2, int note3) {
	if (note1) {
		destiPitch1 = calc_pitch(
			note1 + tuneBase1 + lfoTune1 + lfoTune2 + lfoTune3,
			fineBase1 + lfoFine2 + lfoFine1 + lfoFine3 + bend/0.9
		);
	}

	if (note2) {
		destiPitch2 = calc_pitch(
			note2 + tuneBase2 + lfoTune4 + lfoTune5 + lfoTune6,
			fineBase2 + lfoFine4 + lfoFine5 + lfoFine6 + bend / 0.9
		);
	}
	
	if (note3) {
		destiPitch3 = calc_pitch(
			note3 + tuneBase3 + lfoTune7 + lfoTune8 + lfoTune9,
			fineBase3 + lfoFine7 + lfoFine8 + lfoFine9 + bend / 0.9
		);
	}
}

void calculatePitch() {

	if ((!note_val[0]) && (!note_val[1]) && (!note_val[2])) {
		// no individual channels
		if (!pa) {
			updateDestiPitches(key, key, key);
		} else {
			updateDestiPitches(pKey[0], pKey[1], pKey[2]);
		}
	} else {
		updateDestiPitches(note_val[0], note_val[1], note_val[2]);
	}
	
	// FIXME can we move this out to loop.cpp?
	sidPitch(0, pitch1);
	sidPitch(1, pitch2);
	sidPitch(2, pitch3);
}
