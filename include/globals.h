#include <Arduino.h>
#include "preset.h"

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
extern bool retrig[3];
extern bool fatChanged;
extern bool looping[3];
extern bool resetDown;
extern byte selectedLfo;
extern byte lfoShape[3];


extern byte lfoClockSpeedPending[3];
extern bool filterModeHeld;
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
