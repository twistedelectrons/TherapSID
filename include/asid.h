#pragma once
#include <Arduino.h>
#include "sid.h" // number of chips
#include "opl.h"
#include "preset.h"
#include "ui_pots.h"

// #define ASID_PROFILER
// #define ASID_VOLUME_ADJUST // Sometimes useful, but 6581s might crackle

enum class InitState { ALL, VOICE, WAVEFORM, PW, PITCH, ADSR, SYNC, RING, FILTER_ROUTE, FILTER_MODE };

// Externally called functions
void asidInit(int chip);
void asidInitVoice(int chip, byte voice, InitState init);
void asidRestore(int chip);
void asidRestoreVoice(int chip, byte voice, InitState init);
void asidProcessMessage(byte* buffer, int size);
void asidUpdateFilterCutoff(byte chip);
void asidUpdateFilterReso(byte chip);
void asidInitFilterMode(int chip);
void asidRestoreFilterMode(int chip);
void asidAdvanceFilterMode(byte chip, bool copyFirst);
void asidUpdateFilterRoute(byte chip, bool copyFirst);
void asidUpdateWidth(byte chip, byte voice);
void asidUpdateVolume(byte chip);
void asidIndicateChanged(byte chip);
void asidRestorePot(int chip, byte voice, Pot potindex);
void asidToggleCutoffAdjustMode(bool isPressed);
void asidTick();
void asidFmUpdateFeedback(byte channel);
void asidFmUpdateOpLevel(byte oper);
void asidAdvanceDefaultChip(bool isUp);
void asidSelectDefaultChip(byte chip);
void asidClearDefaultChip();
void asidUpdateLastRemixState(int chip);
void asidUpdateOverrides(int chip);
void asidMuteSidChip(byte chip);

#define SID_REGISTERS_ASID (SID_REGISTERS + 3)

#define POT_VALUE_TO_ASID_PW(x) (x << 1)
#define POT_VALUE_TO_ASID_LORES(x) (x >> 5)
#define POT_VALUE_TO_ASID_FM_LORES(x) (x >> 6)
#define POT_VALUE_TO_ASID_FM_HIRES(x) (x >> 3)
#define POT_VALUE_TO_ASID_CUTOFF(x) (x >> 1)
#define POT_VALUE_TO_ASID_FINETUNE(x) ((value >> 4) + 978)
// Finetune is multiplication value to be divided by 1024. This means +/-53
// cents but from center position -26 (to fix PAL C64 vs 1MHz clock)
// Thereby range = -80/+28. (i.e from 978/1024 to 1042/1024)

#define ASID_SOLO_FLAG_SID_CHIP 0x10
#define ASID_SOLO_FLAG_FM_CHANNEL 0x20
#define ASID_SOLO_FLAG_NO_SOLO 0x40

enum class WaveformState { SIDFILE, RECT, TRI, SAW, NOISE_ONLY, RT, TS, RS, RTS };
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

	bool muteChannel[SIDCHIPS][SIDVOICES_PER_CHIP];

	WaveformState overrideWaveform[SIDCHIPS][SIDVOICES_PER_CHIP];
	OverrideState overrideSync[SIDCHIPS][SIDVOICES_PER_CHIP];
	OverrideState overrideRingMod[SIDCHIPS][SIDVOICES_PER_CHIP];

	int overridePW[SIDCHIPS][SIDVOICES_PER_CHIP];
	bool isOverridePW[SIDCHIPS][SIDVOICES_PER_CHIP];
	int adjustOctave[SIDCHIPS][SIDVOICES_PER_CHIP];
	int adjustFine[SIDCHIPS][SIDVOICES_PER_CHIP];

	int adjustAttack[SIDCHIPS][SIDVOICES_PER_CHIP];
	int adjustDecay[SIDCHIPS][SIDVOICES_PER_CHIP];
	int adjustSustain[SIDCHIPS][SIDVOICES_PER_CHIP];
	int adjustRelease[SIDCHIPS][SIDVOICES_PER_CHIP];

	OverrideState overrideFilterRoute[SIDCHIPS][SIDVOICES_PER_CHIP];
	int adjustCutoff[SIDCHIPS];
	int adjustReso[SIDCHIPS];
	FilterMode filterMode[SIDCHIPS];
	bool muteChip[SIDCHIPS];

#ifdef ASID_VOLUME_ADJUST
	int adjustVolume[SIDCHIPS];
#endif
	bool isOverrideFilterMode[SIDCHIPS];
	bool isRemixed[SIDCHIPS];

	selected_sids_t selectedSids;

	bool isCleanMode;
	bool isShiftMode;
	bool isCutoffAdjustModeScaling;
	byte displayState;

	byte lastSIDvalues[SIDCHIPS][SID_REGISTERS];

	volatile long timer;
	volatile byte slowTimer;

	int8_t defaultSelectedChip;
	bool isSoloButtonHeld;
	byte soloedChannel;

	byte selectButtonCounter;

	byte lastDuplicatedChip;

	// FM OPL stuff
	bool isSidFmMode;
	bool muteFMChannel[OPL_NUM_CHANNELS_MAX];
	int adjustFMOpLevel[OPL_NUM_OPERATORS * OPL_NUM_CHANNELS_MELODY_MODE];
	int adjustFMFeedback[OPL_NUM_CHANNELS_MELODY_MODE];

	byte lastFMvaluesFeedbackConn[OPL_NUM_CHANNELS_MELODY_MODE];
	byte lastFMvaluesKslTotalLev[OPL_SIZE_BLOCK_EXTENDED];
};

extern asidState_t asidState;
