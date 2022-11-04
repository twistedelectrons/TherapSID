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
bool pa, arping;
bool shape1Pressed;
int shape1PressedTimer;
int arpRate = 24;
byte pKey[3];
int lfoStep[3], resetDownTimer;
int lfoSpeed[3], lfoSpeedBase[3];
int lfoDepthBase[3];
int presetScrollSpeed = 10000;
int saveBounce;
byte lfo[3];
int fat = 15;
bool presetUp, presetDown;
byte preset = 1;
int presetLast = 1;
byte lastPot = 20;
bool lfoAss[3][20];
bool retrig[3];
bool fatChanged = true;
bool looping[3];
bool resetDown;
byte selectedLfo;
byte lfoShape[3];
byte fatMode = 0;
int cutBase;
byte lfoClockSpeedPending[3];
bool filterModeHeld;
bool filterEnabled[3];

int tuneBase1, tuneBase2, tuneBase3, lfoTune1, lfoTune2, lfoTune3, lfoTune4, lfoTune5, lfoTune6, lfoTune7, lfoTune8,
    lfoTune9;

int destiPitch1, destiPitch2, destiPitch3;
int pitch1, pitch2, pitch3;
byte glide1, glide2, glide3;
int pw1Base, pw2Base, pw3Base;
float fineBase1, fineBase2, fineBase3, lfoFine1, lfoFine2, lfoFine3, lfoFine4, lfoFine5, lfoFine6, lfoFine7, lfoFine8,
    lfoFine9;
byte sid[25], sidLast[25];
byte a1, a2, a3, d1, d2, d3, s1, s2, s3, r1, r2, r3;

byte filterMode;
byte resBase;
byte key;
byte arpMode;
int frozen;
bool jumble;
bool lfoButtPressed;
byte masterChannel = 1;
byte masterChannelOut = 1;
byte note_val[3];

int held, arpCounter, arpRangeBase;
int lfoButtTimer;
bool heldKeys[128];
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
int arpStepBase;
int dotTimer;
bool showPresetNumber; // when high we show preset number once
