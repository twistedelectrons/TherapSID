#include "globals.h"
#include "ui_vars.h"
#include "util.hpp"

UiState ui_state;
bool sendLfo = false;
bool sendArp = false;
bool pwLimit;
byte settings;
bool lfoNewRand[3];
byte aftertouch;      // latest read afterTouch value
bool aftertouchToLfo; // option to assign aftertouch to LFO depth 2
int loadTimer;
byte volume;
// LIMIT PW
float bend, bend1, bend2, bend3;
bool sync;
int velocityToLfo;
bool toolMode; // when set high by MIDI tool we can receive settings via CC
bool modToLfo;
byte modWheelLast;
bool cvActive[3];
bool scrolled;
bool shape1Pressed;
int shape1PressedTimer;
int lfoStep[3], resetDownTimer;
int lfoSpeed[3];
int presetScrollSpeed = 10000;
int saveBounce;
byte lfo[3];
byte tuneLfoRange = 6;
byte velocityLast;
bool presetUp, presetDown;
byte preset = 1;
int presetLast = 1;
byte lastPot = 20;
bool fatChanged = false;
bool resetDown;
byte selectedLfo;
byte lfoClockSpeedPending[3];

int lfoTune1, lfoTune2, lfoTune3, lfoTune4, lfoTune5, lfoTune6, lfoTune7, lfoTune8, lfoTune9;

float lfoFine1, lfoFine2, lfoFine3, lfoFine4, lfoFine5, lfoFine6, lfoFine7, lfoFine8, lfoFine9;

Preset preset_data;
VoiceState<6> voice_state;
Glide glide[6];

int frozen;
bool jumble;
bool lfoButtPressed;
byte masterChannel = 1;
byte masterChannelOut = 1;
byte voice1Channel = 2; // channel for voice 1 only
byte voice2Channel = 3; // channel for voice 2 only
byte voice3Channel = 4; // channel for voice 3 only

int arpCounter;
int lfoButtTimer;
int arpSpeed;
int arpRange;
int arpRound;
byte arpCount;
byte envState;
bool filterAssignmentChanged = false;
bool volumeChanged; // flag to update EEprom
int env;
int a4, d4, s4, r4;
byte lastNote = 0;
int arpStepBase;

bool arpModeHeld;
byte arp_output_note;
optional<byte> control_voltage_note;

// voice_index[operator] tells you the preset's voice to read the operator's settings from.
byte* voice_index; // array of size 6, set depending on preset.paraphonic
