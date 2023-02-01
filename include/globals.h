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

	byte reg_control; // register 4
	byte reg_attack_decay; // register 5
	byte reg_sustain_release; // register 6

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

struct Preset {
	PresetVoice voice[3];
	bool paraphonic;
	int arp_rate = 24;

	PresetLfo lfo[3];

	int cutoff;

	/*void set_leds(uint8_t lastPot, uint8_t selectedLfo) { // FIXME FIXME FIXME
		assert(lastPot < TODO);
		assert(selectedLfo < 3);

		if (!paraphonic) {
			for (int i=0; i<3; i++) {
				ledSet(4*i+1, bitRead(voice[i].reg_control, 6));
				ledSet(4*i+2, bitRead(voice[i].reg_control, 4));
				ledSet(4*i+3, bitRead(voice[i].reg_control, 5));
				ledSet(4*i+4, bitRead(voice[i].reg_control, 7));
			}
		}
		else {
			for (int i=0; i<3; i++) {
				ledSet(4*i+1, bitRead(voice[0].reg_control, 6));
				ledSet(4*i+2, bitRead(voice[0].reg_control, 4));
				ledSet(4*i+3, bitRead(voice[0].reg_control, 5));
				ledSet(4*i+4, bitRead(voice[0].reg_control, 7));
			}
		}

		// FIXME this should probably depend on paraphonic
		ledSet(16, bitRead(voice[0].reg_control, 1));
		ledSet(17, bitRead(voice[0].reg_control, 2));
		ledSet(18, bitRead(voice[1].reg_control, 1));
		ledSet(19, bitRead(voice[1].reg_control, 2));
		ledSet(20, bitRead(voice[2].reg_control, 1));
		ledSet(21, bitRead(voice[2].reg_control, 2));
	
		ledSet(27, sid_chips[0].filter_mode() & Sid::LOWPASS);
		ledSet(28, sid_chips[0].filter_mode() & Sid::BANDPASS);
		ledSet(29, sid_chips[0].filter_mode() & Sid::HIGHPASS);


		if (lastPot != 20) { // TODO why?
			ledSet(13, lfoAss[0][lastPot]);
			ledSet(14, lfoAss[1][lastPot]);
			ledSet(15, lfoAss[2][lastPot]);
		}


		for (int shape = 1; shape<=5; shape++) {
			ledSet(21 + shape, lfoShape[selectedLfo] == shape);
		}
		ledSet(30, retrig[selectedLfo]);
		ledSet(31, looping[selectedLfo]);
	}*/
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
extern int tuneBase1, tuneBase2, tuneBase3, lfoTune1, lfoTune2, lfoTune3, lfoTune4, lfoTune5, lfoTune6, lfoTune7,
    lfoTune8, lfoTune9;
extern int destiPitch1, destiPitch2, destiPitch3;
extern int pitch1, pitch2, pitch3;
extern byte glide1, glide2, glide3;
extern int pw1Base, pw2Base, pw3Base;
extern float fineBase1, fineBase2, fineBase3, lfoFine1, lfoFine2, lfoFine3, lfoFine4, lfoFine5, lfoFine6, lfoFine7,
    lfoFine8, lfoFine9;
extern byte a1, a2, a3, d1, d2, d3, s1, s2, s3, r1, r2, r3;

extern byte resBase;
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
