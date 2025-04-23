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
#define SID_FILTER_CALIBRATION 1               // activate/deactivate matching filter curves between all chips
#define SID_FILTER_CALIBRATION_OFFSET_FACTOR 3 // upscaling factor for offset steps
/*
  Filter Setup Mode:
  - allows software-based filter calibration via offset & range for each individual chip (cutoff HI register)
  - initially the filter offset and it's range is set to 00 (center), where the range represents a whole byte (FF)
  - current values can be displayed and changed directly via the LFO CHAIN (buttons, pots)
  - a changed offset permanently shifts its original offset value (+/-) and can also be inc/dec by factor 3
  - the range param scales up- or downwards (max 2x zoom out)
  - the calibration can be saved but also reset again
  - the following calibration workflow can be used with these hotkeys:
    - enter "filter setup mode": long-pressed ARP MODE + LFO CHAIN button
        - "filter setup mode" is indicated by "FC" and LFO LEDs flash for available SIDs
    - show current calibration: push LFO CHAIN button 1,2 (or 3) for respective SID
    - calibrate: turn any RATE (= offset) and DEPTH (= range) for the respective SID
    - cancel "filter setup mode": push ARP MODE button again
    - store calibration: long-pressed LFO CHAIN button stores and exits setup; indicated by "SC"
    - reset calibration: pushed RESET button resets any calibration, while long pressed stores and exits setup
        - indicated by "rC"
  - in ASID mode, the calibration can be switched on and off using the LFO TRI button:
    - indicated by "nC" - no calibration
    - indicated by "uC" - use calibration
 */

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

	void set_filtersetup_offset(uint8_t value);
	void set_filtersetup_range(uint8_t value);
	uint8_t get_filtersetup_offset();
	uint8_t get_filtersetup_range();
	void enable_filtersetup(bool value);
	void reset_filtersetup();

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
	uint8_t filtersetup_offset = 0x7F; // cnt
	uint8_t filtersetup_range = 0x7F;  // cnt
	bool filtersetup_enabled = true;   // on
};

extern Sid sid_chips[SIDCHIPS];
