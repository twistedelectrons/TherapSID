#include <Arduino.h>

// FIXME integrate into program
enum class FilterMode {
	LOWPASS,
	BANDPASS,
	HIGHPASS,
	NOTCH,
	OFF
};
extern FilterMode filterMode;

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
extern FatMode fatMode;

inline FatMode uint2FatMode(uint8_t i) { // FIXME
	if (i < 4) {
		return static_cast<FatMode>(i);
	}
	else {
		return FatMode::UNISONO;
	}
}

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

extern Preset preset_data;

extern bool sendLfo;
extern bool sendArp;
extern bool lfoNewRand[3];
extern int loadTimer;
extern const byte version;
extern const byte versionDecimal;
extern bool fatShow;
extern float bend, bend1, bend2, bend3;
extern bool sync;
extern int arpModeCounter;
extern bool cvActive[3];
extern bool scrolled;
extern bool gate;
extern bool arping;
extern bool shape1Pressed;
extern int shape1PressedTimer;
extern byte pKey[3];
extern int lfoStep[3], resetDownTimer;
extern int lfoSpeed[3];
extern int presetScrollSpeed;
extern int saveBounce;
extern byte lfo[3];
extern bool presetUp, presetDown;
extern byte preset;
extern int presetLast;
extern byte lastPot;
extern bool lfoAss[3][20];
extern bool retrig[3];
extern bool fatChanged;
extern bool looping[3];
extern bool resetDown;
extern byte selectedLfo;
extern byte lfoShape[3];


extern byte lfoClockSpeedPending[3];
extern bool filterModeHeld;
extern bool filterEnabled[3];
extern int lfoTune1, lfoTune2, lfoTune3, lfoTune4, lfoTune5, lfoTune6, lfoTune7, lfoTune8,
    lfoTune9;
extern int destiPitch1, destiPitch2, destiPitch3;
extern float lfoFine1, lfoFine2, lfoFine3, lfoFine4, lfoFine5, lfoFine6, lfoFine7, lfoFine8, lfoFine9;
extern int pitch1, pitch2, pitch3;
extern byte key;
extern byte arpMode;
extern int frozen;
extern bool jumble;
extern bool lfoButtPressed;
extern byte masterChannel;
extern byte masterChannelOut;
extern byte note_val[3];
extern int held, arpCounter, arpRangeBase;
extern int lfoButtTimer;
extern bool heldKeys[128];
extern int arpSpeed, arpSpeedBase;
extern int arpRange;
extern int arpRound;
extern byte arpCount;
extern byte envState;
extern int env;
extern int a4, d4, s4, r4;
extern bool saveMode;
extern int saveModeTimer;
extern byte lastNote;
extern int arpStepBase;
extern int dotTimer;
extern bool showPresetNumber; // when high we show preset number once
