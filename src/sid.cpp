#include "sid.h"
#include "util.hpp"
#include <util/atomic.h>

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

enum VoiceRegister { FREQ_LO = 0, FREQ_HI, PULSEWIDTH_LO, PULSEWIDTH_HI, CONTROL, ATTACK_DECAY, SUSTAIN_RELEASE };

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

Sid::Sid(int chip_enable_bit, int extra_chip_enable_bit = -1)
    : chip_enable_bit(chip_enable_bit), extra_chip_enable_bit(extra_chip_enable_bit) {
	for (auto& r : registers)
		r = 0;
	registers[FILTER_CUTOFF_LO] = 0xFF;
	registers[FILTER_CUTOFF_HI] = 0xFF;
	registers[FILTER_RESONANCE_ROUTING] = 0xFF;
	registers[FILTER_MODE_VOLUME] = 0x0F; // Filter off full vol
}

void Sid::set_pulsewidth(int voice, uint16_t pulsewidth) {
	registers[7 * voice + PULSEWIDTH_HI] = pulsewidth >> 8;
	registers[7 * voice + PULSEWIDTH_LO] = pulsewidth & 0xFF;
}

void Sid::set_attack_decay(int voice, uint8_t attack, uint8_t decay) {
	assert(attack <= 0x0F);
	assert(decay <= 0x0F);

	registers[7 * voice + ATTACK_DECAY] = decay | (attack << 4);
}

void Sid::set_sustain_release(int voice, uint8_t sustain, uint8_t release) {
	assert(sustain <= 0x0F);
	assert(release <= 0x0F);

	registers[7 * voice + SUSTAIN_RELEASE] = release | (sustain << 4);
}

/// Updates the next pair of changed registers. Call this in your main loop.
void Sid::send_next_update_pair() {
	// some get sent twice just because it's easier to not special-case them.
	pair send_order[] = {{1, 0},   {3, 2},   {4, 5},   {6, 13},  {8, 7},   {10, 9}, {11, 12},
	                     {15, 14}, {17, 16}, {18, 19}, {20, 23}, {22, 21}, {24, 24}};
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
	registers[7 * voice + FREQ_LO] = value & 0xFF;
	registers[7 * voice + FREQ_HI] = value >> 8;
}

void Sid::set_gate(int voice, bool on) {
	registers[7 * voice + CONTROL] &= ~0x01;
	registers[7 * voice + CONTROL] |= on ? 0x01 : 0x00;
}

void Sid::set_reg_control(int voice, uint8_t control) { registers[7 * voice + CONTROL] = control; }

byte Sid::shape(int voice) const { return registers[7 * voice + CONTROL] & 0xf0; }

void Sid::set_filter_mode(uint8_t value) {
	write_mask(registers[FILTER_MODE_VOLUME], HIGHPASS | BANDPASS | LOWPASS, value);
}

void Sid::set_volume(uint8_t value) {
	registers[FILTER_MODE_VOLUME] = (registers[FILTER_MODE_VOLUME] & 0xF0) | (value & 0x0F);
}

void Sid::set_filtersetup_offset(uint8_t value) {
	// avoid carryover (range 0..0xFE)
	filtersetup_offset = min(0xFE, value);
}

void Sid::set_filtersetup_range(uint8_t value) {
	// treat identically as offset (range 0..0xFE)
	filtersetup_range = min(0xFE, value);
}

uint8_t Sid::get_filtersetup_offset() { return filtersetup_offset; }

uint8_t Sid::get_filtersetup_range() { return filtersetup_range; }

void Sid::enable_filtersetup(bool value) { filtersetup_enabled = value; }

void Sid::reset_filtersetup() {
	filtersetup_offset = 0x7F;
	filtersetup_range = 0x7F;
}

uint8_t Sid::filter_mode() { return registers[FILTER_MODE_VOLUME] & (HIGHPASS | BANDPASS | LOWPASS); }

bool Sid::is_voice_playing(size_t voice) { return registers[7 * voice + CONTROL] & 1; }

void Sid::set_resonance_and_filter_enable(uint8_t resonance, bool fen1, bool fen2, bool fen3, bool fen4) {
	assert(resonance <= 0x0F);

	registers[FILTER_RESONANCE_ROUTING] =
	    (resonance << 4) | (fen1 ? 1 : 0) | (fen2 ? 2 : 0) | (fen3 ? 4 : 0) | (fen4 ? 8 : 0);
}

void Sid::set_filter_cutoff(uint16_t cutoff) {
	assert(cutoff <= 0x07FF);
	registers[FILTER_CUTOFF_LO] = cutoff & 0x07;
	registers[FILTER_CUTOFF_HI] = cutoff >> 3;
}

bool Sid::is_update_allowed(size_t register_index) {
	if (register_index == SUSTAIN_RELEASE1 || register_index == SUSTAIN_RELEASE2 ||
	    register_index == SUSTAIN_RELEASE3) {
		auto voice = register_index / 7;
		return !is_voice_playing(voice);
	} else {
		return true;
	}
}

/// Updates a SID register if it has been changed, and if it's currently allowed to update it.
bool Sid::maybe_update_register(size_t index) {
	bool has_changed = registers_sent[index] != registers[index];
	bool is_allowed = is_update_allowed(index);
	if ((has_changed && is_allowed) || force_initial_update) {
		registers_sent[index] = registers[index];
		send(index, registers[index]);
		return true;
	}
	return false;
}

void Sid::send(size_t index, int data) {

	byte tmpPD_idle, tmpPD_withData, tmpPD_withDataAndCS;

	// Note: The AVR runs at 16MHz, the SID at 1MHz.

	// Set address
#if SIDCHIPS > 2
	if (extra_chip_enable_bit >= 0) {
		// Also set address "chip select" for third SID-chip
		// on ARM2SID (enabled on chip CS)
		PORTC = (index << 3) + _BV(extra_chip_enable_bit);
	} else {
		PORTC = index << 3;
	}
#else
	PORTC = index << 3;
#endif

	// linear conversion of filter curve
#if SID_FILTER_CALIBRATION == 1
	if (filtersetup_enabled && index == FILTER_CUTOFF_HI) {

		int offset = (filtersetup_offset - 0x7F) * SID_FILTER_CALIBRATION_OFFSET_FACTOR; // offset upscaled integer
		char range = (filtersetup_range - 0x7F);

		if (offset || range) {

			// add offset
			int val = data + offset;

			// set frame (+off)
			int minO = 0x00 + offset;
			int maxO = 0xFF + offset;

			// resize frame (+range 2x max - zoom out)
			int minN = minO + (range << 1);
			int maxN = maxO - (range << 1);

			// rescale within a byte frame
			data = max(0, min(0xFF, rescale(val, minO, maxO, minN, maxN)));
		}
	}
#endif

	// Put data on the bus
	PORTB = data;

	// Prepare some variables for fast CS timing
	tmpPD_idle = (PORTD & ~_BV(3)) | _BV(chip_enable_bit);
	tmpPD_withData = tmpPD_idle | _BV(3);
	tmpPD_withDataAndCS = tmpPD_withData & ~_BV(chip_enable_bit);

	// Latch data
	PORTD = tmpPD_withData;

	// Prevent interrupts during CS to not hold too long
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		// Wait for clock to go low
		while ((PIND & 0x80) == 0x80)
			;
		// Enable the SID chip
		PORTD = tmpPD_withDataAndCS;
		// Wait for clock to go high
		while ((PIND & 0x80) == 0x00)
			;
		// Wait for clock to go low
		while ((PIND & 0x80) == 0x80)
			;
		// Disable the SID chip
		PORTD = tmpPD_withData;
	}

	// Falling edge on the data latch
	PORTD = tmpPD_idle;

	// Latch in a zero, for reducing ARMSID glitch sensitivity during mux moves
	PORTB = 0x00;
	PORTD = tmpPD_withData;
	PORTD = tmpPD_idle;
}

void Sid::send_update_immediate(byte index, byte data) {
	if (index < (sizeof(registers) / sizeof(*registers))) {
		registers_sent[index] = registers[index] = data;
	}
	send(index, data);
}

byte Sid::get_current_register(byte index) { return registers[index]; }

// Setup all SID slots 1 and 2 (and above if needed)...
#if SIDCHIPS > 2
Sid sid_chips[SIDCHIPS] = {Sid(2), Sid(6), Sid(2, 2)};
#else
Sid sid_chips[SIDCHIPS] = {Sid(2), Sid(6)};
#endif
