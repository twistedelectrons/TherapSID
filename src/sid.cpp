#include "sid.h"
#include "util.hpp"

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

Sid::Sid(int chip_enable_bit) : chip_enable_bit(chip_enable_bit) {
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

/// `value` needs to be OR-ed together out of NOISE, PULSE, SAW and/or TRI.
/// Note that enabling NOISE together with any other waveform can clear the
/// noise shift register inside the chip.
void Sid::set_shape(int voice, byte value) {
	registers[7 * voice + CONTROL] &= 0x0f;
	registers[7 * voice + CONTROL] |= value;
}

void Sid::set_reg_control(int voice, uint8_t control) { registers[7 * voice + CONTROL] = control; }

byte Sid::shape(int voice) const { return registers[7 * voice + CONTROL] & 0xf0; }

void Sid::set_filter_mode(uint8_t value) {
	write_mask(registers[FILTER_MODE_VOLUME], HIGHPASS | BANDPASS | LOWPASS, value);
}

void Sid::set_volume(uint8_t value) {
	registers[FILTER_MODE_VOLUME] = (registers[FILTER_MODE_VOLUME] & 0xF0) | (value & 0x0F);
}

uint8_t Sid::filter_mode() { return registers[FILTER_MODE_VOLUME] & (HIGHPASS | BANDPASS | LOWPASS); }

/// In addition to the usual 3 voices, this accepts voice==3 for the external input.
void Sid::set_voice_filter(int voice, bool enable) { bitWrite(registers[FILTER_RESONANCE_ROUTING], voice, enable); }

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

void Sid::set_armSid(bool value) {
armSID=value;
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

	if (armSID) {
		// ARMSID sounds cleaner without delays
		PORTC = index << 3;
		PORTB = data;
		PORTD |= _BV(3);
		PORTD &= ~_BV(chip_enable_bit); // enable the sid chip
		PORTD |= _BV(chip_enable_bit);  // disable the sid chip again
		PORTD &= ~_BV(3);               // falling edge on the data latch. (ignored)
	} else {

		// Note: The AVR runs at 8MHz, the SID at 1MHz.
		// One AVR instruction takes at least 125ns.

		PORTC = index << 3;
		PORTB = data;
		delayMicroseconds(1); // data latch input needs to be stable >= 25ns

		PORTD |= _BV(3);      // rising edge on the data latch. -> latch-in the data
		delayMicroseconds(1); // need to wait >= 25ns until latch output is propagated.
		                      // also, the sid's data input must be stable for >=80ns

		PORTD &= ~_BV(chip_enable_bit); // enable the sid chip
		delayMicroseconds(1);           // need to wait up to 1us until next sid clock edge

		PORTD |= _BV(chip_enable_bit); // disable the sid chip again
		PORTD &= ~_BV(3);              // falling edge on the data latch. (ignored)
		delayMicroseconds(1);          // data need to be held for >= 10ns
	}
}

Sid sid_chips[2] = {Sid(6), Sid(2)};
