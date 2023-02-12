#pragma once
#include <stdint.h>
#include <Arduino.h>

void sidReset();
void init1MhzClock();
void sidSend(byte address, byte data);

class Sid {
  public:
	Sid(int chip_enable_bit);

	/// Updates the next pair of changed registers. Call this in your main loop.
	void send_next_update_pair();

	void set_freq(int voice, uint16_t value);
	[[deprecated]] void set_gate(int voice, bool on);
	/// attack,decay in [0; 15]
	void set_attack_decay(int voice, uint8_t attack, uint8_t decay);
	/// sustain,release in [0; 15]
	void set_sustain_release(int voice, uint8_t sustain, uint8_t release);

	enum Shape { NOISE = 1 << 7, PULSE = 1 << 6, SAW = 1 << 5, TRI = 1 << 4 };

	/// `value` needs to be OR-ed together out of NOISE, PULSE, SAW and/or TRI.
	/// Note that enabling NOISE together with any other waveform can clear the
	/// noise shift register inside the chip.
	[[deprecated]] void set_shape(int voice, byte value);

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

	uint8_t filter_mode();

	/// In addition to the usual 3 voices, this accepts voice==3 for the external input.
	[[deprecated]] void set_voice_filter(int voice, bool enable);

  private:
	bool is_voice_playing(size_t voice);

	bool is_update_allowed(size_t register_index);

	/// Updates a SID register if it has been changed, and if it's currently allowed to update it.
	bool maybe_update_register(size_t index);

	void send(size_t index, int data);

	size_t next_pair = 0;
	int chip_enable_bit;
	bool force_initial_update = true;
	byte registers[25];
	byte registers_sent[25];
};

extern Sid sid_chips[2];
