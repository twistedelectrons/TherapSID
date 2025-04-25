#include "globals.h"
#include "ui_vars.h"
#include "util.hpp"
#include "armsid.h"
#include "sid.h"

UiState ui_state;
bool sendLfo = false;
bool sendArp = false;
bool pwLimit;
bool lfoNewRand[3];
bool noArp1key;
byte aftertouch;      // latest read afterTouch value
bool aftertouchToLfo; // option to assign aftertouch to LFO depth 2
int loadTimer;
byte volume;
int presetScrollTimer;
// LIMIT PW
float bend, bend1, bend2, bend3;
bool sync;
bool velocityToLfo;
bool toolMode; // when set high by MIDI tool we can receive settings via CC
bool modToLfo;
byte modWheelLast;
bool cvActive[3];
bool heldKeys[128];
bool chordKeys[128];
bool clearAutoChord;
bool autoChordChanged;
bool scrolled;
bool autoChord; // when set high by mode+retrig capture held notes and play themback with single finger.
byte chordRoot;
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
VoiceState<SIDVOICES_TOTAL> voice_state;
Glide glide[SIDVOICES_TOTAL];

int frozen;
bool jumble;
byte lfoButtPressed;
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
byte pitchBendUp;
byte pitchBendDown;
bool arpModeHeld;
byte arp_output_note;
optional<byte> control_voltage_note;

// voice_index[operator] tells you the preset's voice to read the operator's settings from.
byte* voice_index; // array of size SIDVOICES_TOTAL, set depending on preset.paraphonic

int filterSetupTimer;
bool filterSetupBlink;
byte filterSetupShowOfSid;
byte filterSetupShowIndex;
byte filterSetupSidOffset[3];
byte filterSetupSidRange[3];

// Global settings, ranges and where stored in EEPROM memory
const globalSetting globalSettings[EEPROM_SETTINGS_NUM_BYTES] = {

    {&modToLfo, EEPROM_ADDR_MW_TO_LFO1, 0, 1, true, 85, false},
    {&aftertouchToLfo, EEPROM_ADDR_AT_TO_LFO2, 0, 1, true, 86, false},
    {&velocityToLfo, EEPROM_ADDR_VEL_TO_LFO3, 0, 1, false, 87, false},
    {&sendLfo, EEPROM_ADDR_SEND_LFO, 0, 1, false, 92, false},
    {&sendArp, EEPROM_ADDR_SEND_ARP, 0, 1, false, 93, false},
    {&pwLimit, EEPROM_ADDR_PW_LIMIT, 0, 1, true, 88, false},
    {&noArp1key, EEPROM_ADDR_NO_ARP_1_KEY, 0, 1, 0, 101, false},

    {&masterChannel, EEPROM_ADDR_MIDI_IN_CH_MASTER, 1, 16, 1, 90, true},
    {&masterChannelOut, EEPROM_ADDR_MIDI_OUT_CH_MASTER, 1, 16, 1, 91, true},
    {&voice1Channel, EEPROM_ADDR_MIDI_IN_CH_VOICE1, 1, 16, 2, 94, true},
    {&voice2Channel, EEPROM_ADDR_MIDI_IN_CH_VOICE2, 1, 16, 3, 95, true},
    {&voice3Channel, EEPROM_ADDR_MIDI_IN_CH_VOICE3, 1, 16, 4, 96, true},

    {&volume, EEPROM_ADDR_MASTER_VOLUME, 1, 15, 15, 89, false},
    {&preset, EEPROM_ADDR_PRESET_LAST, PRESET_NUMBER_MIN, PRESET_NUMBER_MAX, PRESET_NUMBER_MIN, 255, false},
    {&pitchBendUp, EEPROM_ADDR_PITCH_BEND_UP, 1, 48, 2, 99, true},
    {&pitchBendDown, EEPROM_ADDR_PITCH_BEND_DOWN, 1, 48, 2, 100, true},

    {&armsidConfig.emulation.raw, EEPROM_ADDR_ARMSID_CHIP_EMULATION, 0, 255, ARMSID_SETTING_RAW(ARMSID_EMULATION_AUTO),
     102, false},
    {&armsidConfig.adsrBugFixed.raw, EEPROM_ADDR_ARMSID_ADSR_BUG_FIXED, 0, 17, ARMSID_SETTING_RAW(false), 103, false},
    {&armsidConfig.filter6581strength.raw, EEPROM_ADDR_ARMSID_6581_FILTER_STRENGTH, 0, 255,
     ARMSID_SETTING_RAW(ARMSID_6581_FILTER_STRENGTH_DEFAULT), 104, false},
    {&armsidConfig.filter6581low.raw, EEPROM_ADDR_ARMSID_6581_FILTER_LOW, 0, 255,
     ARMSID_SETTING_RAW(ARMSID_6581_FILTER_LOW_DEFAULT), 106, false},
    {&armsidConfig.filter8580central.raw, EEPROM_ADDR_ARMSID_8580_FILTER_CENTRAL, 0, 255,
     ARMSID_SETTING_RAW(ARMSID_8580_FILTER_CENTRAL_DEFAULT), 107, false},
    {&armsidConfig.filter8580low.raw, EEPROM_ADDR_ARMSID_8580_FILTER_LOW, 0, 255,
     ARMSID_SETTING_RAW(ARMSID_8580_FILTER_LOW_DEFAULT), 108, false},

    {&filterSetupSidOffset[0], EEPROM_ADDR_SID1_OFFSET, 0, 0xFF, 0x7F, 255, false},
    {&filterSetupSidRange[0], EEPROM_ADDR_SID1_RANGE, 0, 0xFF, 0x7F, 255, false},
    {&filterSetupSidOffset[1], EEPROM_ADDR_SID2_OFFSET, 0, 0xFF, 0x7F, 255, false},
    {&filterSetupSidRange[1], EEPROM_ADDR_SID2_RANGE, 0, 0xFF, 0x7F, 255, false},
    {&filterSetupSidOffset[2], EEPROM_ADDR_SID3_OFFSET, 0, 0xFF, 0x7F, 255, false},
    {&filterSetupSidRange[2], EEPROM_ADDR_SID3_RANGE, 0, 0xFF, 0x7F, 255, false},
};