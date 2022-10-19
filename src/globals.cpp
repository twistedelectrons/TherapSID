#include "globals.h"

bool sendLfo = false;
bool sendArp = false;

bool lfoNewRand[3];

int loadTimer;

const byte version = 1;
const byte versionDecimal = 9;
bool fatShow = false;
// LIMIT PW
float bend, bend1, bend2, bend3;
bool sync;

bool arpModeHeld;
int arpModeCounter;
bool cvActive[3];
bool scrolled;
bool gate, gateLast, randomized;
bool pa, paLast, arping;
bool pedal;
bool shape1Pressed;
int shape1PressedTimer;
byte midiSetup = 0;
int arpRate = 24;
int clockCount;
int ppq = 3;
byte slot[3];
byte pKey[3];
int lfoStep[3], resetDownTimer;
byte lfoClockSpeed[3];
int lfoSpeed[3], lfoSpeedBase[3];
int lfoDepth[3], lfoDepthBase[3];
int lfoCounter[3];
int presetScrollSpeed = 10000;
int presetScrollTimer, saveBounce;
byte lfo[3];
byte velocityLast;
int fat = 15;
byte pSlot[3];
float lfoStepF[3];
bool presetUp, presetDown;
byte preset = 1;
int presetLast = 1;
byte lastPot = 20;
bool lfoAss[3][20];
bool retrig[3];
bool fatChanged = true;
bool looping[3];
bool saveEngaged, resetDown;
byte arpNotes[128];
byte selectedLfo, selectedLfoLast;
byte lfoShape[3];
byte sid2[6];
byte lfoLast[3];
byte fatMode = 0;
int lfoCut1, lfoCut2, lfoCut3, cutBase, lfoSpeedLfo1, lfoSpeedLfo2,
    lfoSpeedLfo3, lfoSpeedLfo4, lfoSpeedLfo5, lfoSpeedLfo6, lfoDepthLfo1,
    lfoDepthLfo2, lfoDepthLfo3, lfoDepthLfo4, lfoDepthLfo5, lfoDepthLfo6;
byte glideCounter1, glideCounter2, glideCounter3;
byte lfoClockSpeedPending[3];
bool filterModeHeld;
bool filterEnabled[3];
bool assignmentChanged;

int tuneBase1, tuneBase2, tuneBase3, lfoTune1, lfoTune2, lfoTune3, lfoTune4,
    lfoTune5, lfoTune6, lfoTune7, lfoTune8, lfoTune9;

int destiPitch1, destiPitch2, destiPitch3;
int pitch1, pitch2, pitch3, glideRange1, glideRange2, glideRange3;
byte glide1, glide2, glide3;
int pw1, pw2, pw3, pw1Base, pw2Base, pw3Base, pw1Lfo1, pw1Lfo2, pw1Lfo3,
    pw2Lfo1, pw2Lfo2, pw2Lfo3, pw3Lfo1, pw3Lfo2, pw3Lfo3;
float fineBase1, fineBase2, fineBase3, fine1, fine2, fine3, lfoFine1, lfoFine2,
    lfoFine3, lfoFine4, lfoFine5, lfoFine6, lfoFine7, lfoFine8, lfoFine9;
byte sid[25], sidLast[25], resLfo1, resLfo2, resLfo3;
byte a1, a2, a3, d1, d2, d3, s1, s2, s3, r1, r2, r3;
bool paraMode;
int potLast[42];

byte filterMode;
byte res, resBase;
byte key;
byte arpMode;
int frozen;
bool jumble;
bool buttLast[33], lfoButtPressed;
byte masterChannel = 1;
byte masterChannelOut = 1;
byte note1, note2, note3;
int noteHeld1, noteHeld2, noteHeld3;

int finalCut;
int held, arpCounter, arpRangeBase, arpRangeLfo1, arpRangeLfo3, arpRangeLfo2;
int lfoButtTimer;
bool heldKeys[128];
int arpSpeed, arpSpeedBase, arpSpeedLfo1, arpSpeedLfo2, arpSpeedLfo3, arpRange,
    arpStep;
int arpNote, arpSlot, arpRound;
byte arpCount;
bool arpPendulum;
byte envState;
int env, envCounter;
int a4, d4, s4, r4;
bool saveMode, saveModeFlash;
int saveModeTimer;
byte lastNote = 0;
int arpStepLast, arpStepBase, arpStepLfo1, arpStepLfo2, arpStepLfo3;
int dotTimer;
bool first=true;
