#pragma once
#include <stdint.h>
#include <Arduino.h>

#define PRESET_NUMBER_MIN 1
#define PRESET_NUMBER_MAX 99
#define PRESET_DATA_SIZE 40

// filtermode helper
void setNextFilterMode();
void setFilterMode(uint8_t idx);
uint8_t getFilterModeIdx();

void save();
void load(byte number);
void saveChannels();

struct PresetVoice {
	enum Shape { NOISE = 1 << 7, PULSE = 1 << 6, SAW = 1 << 5, TRI = 1 << 4 };

	byte attack = 0, decay = 0, sustain = 0, release = 0;

	int tune_base;
	byte glide;
	int pulsewidth_base;
	float fine_base;

	byte reg_control = 0; // register 4
	bool filter_enabled = true;

	void toggle_shape(Shape shape) {
		reg_control ^= shape;

		// Make sure that noise is not enabled together with any other waveform.
		auto enabled = reg_control & shape;
		if (enabled) {
			if (shape == NOISE) {
				// If the noise waveform has been turned on, disable the other three.
				reg_control &= ~(TRI | SAW | PULSE);
			} else {
				// If one of the other three waveforms was enabled, make sure to disable noise.
				reg_control &= ~NOISE;
			}
		}
	}

	void set_shape(Shape shape, bool enabled) {
		bool is_enabled = !!(reg_control & shape);
		if (enabled != is_enabled)
			toggle_shape(shape);
	}

	uint8_t shape() const { return reg_control & (TRI | SAW | PULSE | NOISE); }

	bool wants_filter() const { return filter_enabled && shape() != 0; }
};

struct PresetLfo {
	int depth;
	int speed;
	bool retrig;
	bool looping;
	byte shape;
	bool mapping[20];
};

enum class FilterMode { LOWPASS, BANDPASS, HIGHPASS, NOTCH, OFF, LB, BH, LBH };

enum class FatMode { UNISONO, OCTAVE_UP, DETUNE_SLIGHT, DETUNE_MUCH, MORE_VOICES, PARA_2OP };

struct Preset {
	PresetVoice voice[3];

	bool paraphonic;

	int arp_rate = 24;
	int arp_speed_base = 100;
	int arp_range_base = 0;
	byte arp_mode = 0;

	PresetLfo lfo[3];

	byte resonance_base;
	int cutoff;
	FilterMode filter_mode = FilterMode::LOWPASS;
	FatMode fat_mode = FatMode::UNISONO;

	uint16_t fatten_pitch(uint16_t pitch, uint8_t chip) const;
	bool is_polyphonic() const { return paraphonic || fat_mode == FatMode::MORE_VOICES; }
};
