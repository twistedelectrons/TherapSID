#include "globals.h"
#include "ui_vars.h"
#include "util.hpp"

UiState ui_state;
bool sendLfo = false;
bool sendArp = false;
bool pwLimit;
bool lfoNewRand[3];
byte aftertouch;      // latest read afterTouch value
bool aftertouchToLfo; // option to assign aftertouch to LFO depth 2
int loadTimer;
byte volume;
// LIMIT PW
float bend, bend1, bend2, bend3;
bool sync;
bool armSID;
bool velocityToLfo;
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

// Global settings, ranges and where stored in EEPROM memory
const globalSetting globalSettings[14] = {

    {&modToLfo, EEPROM_ADDR_MW_TO_LFO1, 0, 1, true, 85, false},
    {&aftertouchToLfo, EEPROM_ADDR_AT_TO_LFO2, 0, 1, true, 86, false},
    {&velocityToLfo, EEPROM_ADDR_VEL_TO_LFO3, 0, 1, false, 87, false},
    {&sendLfo, EEPROM_ADDR_SEND_LFO, 0, 1, false, 92, false},
    {&sendArp, EEPROM_ADDR_SEND_ARP, 0, 1, false, 93, false},
    {&pwLimit, EEPROM_ADDR_PW_LIMIT, 0, 1, true, 88, false},
    {&armSID, EEPROM_ADDR_ARMSID_MODE, 0, 1, false, 97, false},

    {&masterChannel, EEPROM_ADDR_MIDI_IN_CH_MASTER, 1, 16, 1, 90, true},
    {&masterChannelOut, EEPROM_ADDR_MIDI_OUT_CH_MASTER, 1, 16, 1, 91, true},
    {&voice1Channel, EEPROM_ADDR_MIDI_IN_CH_VOICE1, 1, 16, 2, 94, true},
    {&voice2Channel, EEPROM_ADDR_MIDI_IN_CH_VOICE2, 1, 16, 3, 95, true},
    {&voice3Channel, EEPROM_ADDR_MIDI_IN_CH_VOICE3, 1, 16, 4, 96, true},

    {&volume, EEPROM_ADDR_MASTER_VOLUME, 1, 15, 15, 89, false},
    {&preset, EEPROM_ADDR_PRESET_LAST, PRESET_NUMBER_MIN, PRESET_NUMBER_MAX, PRESET_NUMBER_MIN, 255, false},

};
