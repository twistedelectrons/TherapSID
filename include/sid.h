#pragma once
#include <stdint.h>
#include <Arduino.h>

#define SIDCHIPS 2 // Normal case, 2 SIDs
// #define SIDCHIPS 3 // Used for ARM2SID with third SID connected. See note:
/*
  To make third SID work the following patch needs to be done:
   - Lift pin 24 of CPU (IC13) out of socket
   - Connect pin 24 of CPU to ARM2SID top connector, pin 6
   - Other pins of ARM2SID top connector should be connected to ARM2SID SID2 socket
   - The above will also disable LFO CV input 3
 */

#define SIDVOICES_PER_CHIP 3
#define SIDVOICES_TOTAL (SIDCHIPS * SIDVOICES_PER_CHIP)
#define SID_REGISTERS 25

void sidReset();
void init1MhzClock();
void sidSend(byte address, byte data);

class Sid {
  public:
	Sid(int chip_enable_bit, int extra_chip_enable_bit);

	/// Updates the next pair of changed registers. Call this in your main loop.
	void send_next_update_pair();

	// Update the SID directly
	void send_update_immediate(byte index, byte data);
	byte get_current_register(byte index);

	void set_freq(int voice, uint16_t value);
	[[deprecated]] void set_gate(int voice, bool on);
	/// attack,decay in [0; 15]
	void set_attack_decay(int voice, uint8_t attack, uint8_t decay);
	/// sustain,release in [0; 15]
	void set_sustain_release(int voice, uint8_t sustain, uint8_t release);

	enum Shape { NOISE = 1 << 7, PULSE = 1 << 6, SAW = 1 << 5, TRI = 1 << 4 };

	/// 0..4095
	void set_pulsewidth(int voice, uint16_t width);

	/// 0..4095
	void set_filter_cutoff(uint16_t cutoff);

	void set_resonance_and_filter_enable(uint8_t resonance /* 0..15*/, bool en1, bool en2, bool en3, bool en_ext);

	/// Sets gate, shape, sync and ring.
	void set_reg_control(int voice, byte value);

	byte shape(int voice) const;

	enum FilterMode { HIGHPASS = 1 << 6, BANDPASS = 1 << 5, LOWPASS = 1 << 4 };

	void set_filter_mode(uint8_t value);

	void set_volume(uint8_t value);

	uint8_t filter_mode();

  private:
	bool is_voice_playing(size_t voice);

	bool is_update_allowed(size_t register_index);

	/// Updates a SID register if it has been changed, and if it's currently allowed to update it.
	bool maybe_update_register(size_t index);

	void send(size_t index, int data);

	size_t next_pair = 0;
	int chip_enable_bit;
	int extra_chip_enable_bit = -1;
	bool force_initial_update = true;
	byte registers[SID_REGISTERS];
	byte registers_sent[SID_REGISTERS];
};

extern Sid sid_chips[SIDCHIPS];
