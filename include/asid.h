#pragma once
#include <Arduino.h>
#include "preset.h"

// #define ASID_PROFILER
// #define ASID_VOLUME_ADJUST // Sometimes useful, but 6581s might crackle

// Externally called functions
void asidInit(int chip);
void asidRestore(int chip);
void asidProcessMessage(byte* buffer, int size);
void asidUpdateFilterCutoff(byte chip);
void asidUpdateFilterReso(byte chip);
void asidAdvanceFilterMode(byte chip, bool copyFirst);
void asidUpdateFilterRoute(byte chip, bool copyFirst);
void asidUpdateWidth(byte chip, byte voice);
void asidUpdateVolume(byte chip);
void asidIndicateChanged(byte chip);
void asidToggleCutoffAdjustMode(bool isPressed);
void asidTick();

#define SIDVOICES 3
#define SIDCHIPS 2
#define SID_REGISTERS 25
#define SID_REGISTERS_ASID 28

#define POT_VALUE_TO_ASID_PW(x) (x << 1)
#define POT_VALUE_TO_ASID_LORES(x) (x >> 5)
#define POT_VALUE_TO_ASID_CUTOFF(x) (x >> 1)
#define POT_VALUE_TO_ASID_FINETUNE(x) ((value >> 4) + 978)
// Finetune is multiplication value to be divided by 1024. This means +/-53
// cents but from center position -26 (to fix PAL C64 vs 1MHz clock)
// Thereby range = -80/+28. (i.e from 978/1024 to 1042/1024)

enum class WaveformState { SIDFILE, RECT, TRI, SAW, NOISE_ONLY };
enum class OverrideState { SIDFILE, ON, OFF };

union selected_sids_t {
	byte all;
	struct bits {
		byte sid1 : 1;
		byte sid2 : 1;
#ifdef ASID_PROFILER
		byte dbg1 : 1;
		byte dbg2 : 1;
		byte dbg3 : 1;
		byte dbg4 : 1;
#endif
	} b;
};

struct asidState_t {
	bool enabled;

	bool muteChannel[SIDCHIPS][SIDVOICES];

	WaveformState overrideWaveform[SIDCHIPS][SIDVOICES];
	OverrideState overrideSync[SIDCHIPS][SIDVOICES];
	OverrideState overrideRingMod[SIDCHIPS][SIDVOICES];

	int overridePW[SIDCHIPS][SIDVOICES];
	bool isOverridePW[SIDCHIPS][SIDVOICES];
	int adjustOctave[SIDCHIPS][SIDVOICES];
	int adjustFine[SIDCHIPS][SIDVOICES];

	int adjustAttack[SIDCHIPS][SIDVOICES];
	int adjustDecay[SIDCHIPS][SIDVOICES];
	int adjustSustain[SIDCHIPS][SIDVOICES];
	int adjustRelease[SIDCHIPS][SIDVOICES];

	OverrideState overrideFilterRoute[SIDCHIPS][SIDVOICES];
	int adjustCutoff[SIDCHIPS];
	int adjustReso[SIDCHIPS];
	FilterMode filterMode[SIDCHIPS];

#ifdef ASID_VOLUME_ADJUST
	int adjustVolume[SIDCHIPS];
#endif
	bool isOverrideFilterMode[SIDCHIPS];
	bool isRemixed[SIDCHIPS];

	selected_sids_t selectedSids;

	bool isCleanMode;
	bool isCutoffAdjustModeScaling;
	byte displayState;

	byte lastSIDvalues[SID_REGISTERS];

	volatile long timer;
	volatile byte slowTimer;
};

extern asidState_t asidState;
