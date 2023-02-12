#pragma once
#include <Arduino.h>
#include "preset.h"
#include "voice_state.hpp"

extern Preset preset_data;
extern VoiceState<6> voice_state;
extern Glide glide[6];

extern bool sendLfo;
extern bool sendArp;
extern bool lfoNewRand[3];
extern float bend, bend1, bend2, bend3; // TODO reactivate bend1-bend3
extern bool sync;
extern bool cvActive[3];
extern bool gate;
extern int lfoStep[3];
extern byte lfo[3];

inline bool arping() { return preset_data.arp_speed_base << 4 > 4000; }

extern byte lfoClockSpeedPending[3];
extern int lfoTune1, lfoTune2, lfoTune3, lfoTune4, lfoTune5, lfoTune6, lfoTune7, lfoTune8, lfoTune9;
extern float lfoFine1, lfoFine2, lfoFine3, lfoFine4, lfoFine5, lfoFine6, lfoFine7, lfoFine8, lfoFine9;
extern bool jumble;
extern byte masterChannel;
extern byte masterChannelOut;
extern int arpCounter;
extern int arpSpeed;
extern int arpRange;
extern int arpRound;
extern byte arpCount;
extern byte envState;
extern int env;
extern int a4, d4, s4, r4;
extern byte lastNote;
extern int arpStepBase;
extern int lfoSpeed[3];

extern byte preset;
