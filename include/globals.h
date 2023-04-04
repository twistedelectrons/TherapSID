#pragma once
#include <Arduino.h>
#include "preset.h"
#include "voice_state.hpp"

template <typename T> class optional;

extern Preset preset_data;
extern VoiceState<6> voice_state;
extern Glide glide[6];
extern byte settings;
extern byte aftertouch;
extern bool aftertouchToLfo;
extern bool sendLfo;
extern bool volumeChanged;
extern bool sendArp;
extern bool lfoNewRand[3];
extern float bend, bend1, bend2, bend3;
extern bool sync;
extern bool pwLimit;
extern bool toolMode;
extern bool armSID;
extern bool cvActive[3];
extern int lfoStep[3];
extern byte lfo[3];
extern int velocityToLfo;
extern bool modToLfo;
extern byte velocityLast;
extern byte modWheelLast;
inline bool arping() { return preset_data.arp_speed_base < 250; }
extern bool filterAssignmentChanged;
extern byte lfoClockSpeedPending[3];
extern int lfoTune1, lfoTune2, lfoTune3, lfoTune4, lfoTune5, lfoTune6, lfoTune7, lfoTune8, lfoTune9;
extern float lfoFine1, lfoFine2, lfoFine3, lfoFine4, lfoFine5, lfoFine6, lfoFine7, lfoFine8, lfoFine9;
extern bool jumble;
extern byte masterChannel;
extern byte masterChannelOut;
extern byte voice1Channel;
extern byte voice2Channel;
extern byte voice3Channel;
extern int arpCounter;
extern int arpSpeed;
extern int arpRange;
extern int arpRound;
extern byte tuneLfoRange;
extern byte arpCount;
extern byte envState;
extern int env;
extern int a4, d4, s4, r4;
extern byte lastNote;
extern int arpStepBase;
extern int lfoSpeed[3];

extern byte preset;
extern byte volume;
extern byte arp_output_note;
extern optional<byte> control_voltage_note;

// voice_index[operator] tells you the preset's voice to read the operator's settings from.
extern byte* voice_index; // array of size 6, set depending on preset.paraphonic

// Memory Mapping
#define EEPROM_ADDR_PRESET_LAST 3999
#define EEPROM_ADDR_MIDI_IN_CH_MASTER 3998
#define EEPROM_ADDR_MIDI_OUT_CH_MASTER 3997
#define EEPROM_ADDR_SEND_LFO 3996
#define EEPROM_ADDR_SEND_ARP 3995
#define EEPROM_ADDR_MW_TO_LFO1 3994
#define EEPROM_ADDR_AT_TO_LFO2 3993
#define EEPROM_ADDR_VEL_TO_LFO3 3992
#define EEPROM_ADDR_MASTER_VOLUME 3991
#define EEPROM_ADDR_PW_LIMIT 3990
#define EEPROM_ADDR_MIDI_IN_CH_VOICE1 3989
#define EEPROM_ADDR_MIDI_IN_CH_VOICE2 3988
#define EEPROM_ADDR_MIDI_IN_CH_VOICE3 3987
#define EEPROM_ADDR_ARMSID_MODE 3986