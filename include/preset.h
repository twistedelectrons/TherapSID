#pragma once
#include <stdint.h>
#include <Arduino.h>

void save();
void load(byte number);
void saveChannels();

struct PresetVoice {
	enum Shape {
		NOISE = 1<<7,
		PULSE = 1<<6,
		SAW = 1<<5,
		TRI = 1<<4
	};

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
			}
			else {
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

	uint8_t shape() const {
		return reg_control & (TRI | SAW | PULSE | NOISE);
	}
};

struct PresetLfo {
	int depth;
	int speed;
};

enum class FilterMode {
	LOWPASS,
	BANDPASS,
	HIGHPASS,
	NOTCH,
	OFF
};

inline FilterMode uint2FilterMode(uint8_t i) { // FIXME
	if (i < 5) {
		return static_cast<FilterMode>(i);
	}
	else {
		return FilterMode::OFF;
	}
}

enum class FatMode {
	UNISONO,
	OCTAVE_UP,
	DETUNE_SLIGHT,
	DETUNE_MUCH
};

inline FatMode uint2FatMode(uint8_t i) { // FIXME
	if (i < 4) {
		return static_cast<FatMode>(i);
	}
	else {
		return FatMode::UNISONO;
	}
}


// FIXME move to preset.h
struct Preset {
	PresetVoice voice[3];
	bool paraphonic;
	int arp_rate = 24;
	byte resonance_base;

	PresetLfo lfo[3];
	bool lfo_map[3][20]; // FIXME put this into PresetLfo

	int cutoff;

	FilterMode filter_mode = FilterMode::LOWPASS;
	FatMode fat_mode = FatMode::UNISONO;

	void set_leds(int lastPot, int selectedLfo, bool show_filter_assign);
	uint16_t fatten_pitch(uint16_t pitch) const;
};

