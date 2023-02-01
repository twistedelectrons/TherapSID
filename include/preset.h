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

	byte reg_control; // register 4

	uint8_t control = 0;
	bool filter_enabled = true;

	void toggle_shape(Shape shape) {
		control ^= shape;

		// Make sure that noise is not enabled together with any other waveform.
		auto enabled = control & shape;
		if (enabled) {
			if (shape == NOISE) {
				// If the noise waveform has been turned on, disable the other three.
				control &= ~(TRI | SAW | PULSE);
			}
			else {
				// If one of the other three waveforms was enabled, make sure to disable noise.
				control &= ~NOISE;
			}
		}
	}
};

struct PresetLfo {
	int depth;
	int speed;
};

// FIXME move to preset.h
struct Preset {
	PresetVoice voice[3];
	bool paraphonic;
	int arp_rate = 24;
	byte resonance_base;

	PresetLfo lfo[3];

	int cutoff;

	void set_leds(int lastPot, int selectedLfo);
};

