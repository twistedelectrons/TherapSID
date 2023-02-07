#include "globals.h"

bool sendLfo = false;
bool sendArp = false;

bool lfoNewRand[3];

int loadTimer;

const byte version = 2;
const byte versionDecimal = 0;

bool fatShow = false;
// LIMIT PW
float bend, bend1, bend2, bend3;
bool sync;

int arpModeCounter;
bool cvActive[3];
bool scrolled;
bool gate;
bool arping; // PATCH
bool shape1Pressed;
int shape1PressedTimer;
int lfoStep[3], resetDownTimer; // PATCH
int lfoSpeed[3]; // not PATCH
int presetScrollSpeed = 10000;
int saveBounce;
byte lfo[3];
bool presetUp, presetDown;
byte preset = 1;
int presetLast = 1;
byte lastPot = 20;
bool retrig[3]; // PATCH
bool fatChanged = true;
bool looping[3];
bool resetDown;
byte selectedLfo;
byte lfoShape[3]; // PATCH
byte lfoClockSpeedPending[3];
bool filterModeHeld;

int lfoTune1, lfoTune2, lfoTune3, lfoTune4, lfoTune5, lfoTune6, lfoTune7, lfoTune8,
    lfoTune9;

float lfoFine1, lfoFine2, lfoFine3, lfoFine4, lfoFine5, lfoFine6, lfoFine7, lfoFine8,
    lfoFine9;



Preset preset_data;
VoiceState<6> voice_state;
Glide glide[6];


byte arpMode;
int frozen;
bool jumble;
bool lfoButtPressed;
byte masterChannel = 1;
byte masterChannelOut = 1;

int arpCounter;
int arpRangeBase; // PATCH
int lfoButtTimer;
int arpSpeed, arpSpeedBase;
int arpRange;
int arpRound;
byte arpCount;
byte envState;
int env;
int a4, d4, s4, r4;
bool saveMode;
int saveModeTimer;
byte lastNote = 0;
int arpStepBase; // oof. PATCH??
int dotTimer;
bool showPresetNumber; // when high we show preset number once
