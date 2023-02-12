#pragma once
#include <Arduino.h>

struct UiState {
	bool filterModeHeld;
	byte lastPot;
	byte selectedLfo;
	byte midiSetup = 0;
};

extern bool scrolled;             // UI
extern bool shape1Pressed;        // UI
extern int shape1PressedTimer;    // UI
extern int resetDownTimer;        // UI
extern int presetScrollSpeed;     // UI
extern int saveBounce;            // UI
extern bool presetUp, presetDown; // UI
extern int presetLast;            // UI??
extern bool fatChanged;           // UI
extern bool resetDown;            // UI
extern int frozen;                // UI
extern bool lfoButtPressed;       // UI
extern int lfoButtTimer;          // UI
extern bool saveMode;             // UI

extern int arpModeCounter;
extern int loadTimer;

extern UiState ui_state;

// FIXME delete
// preset -> display preset
// midiSetup = 3 -> digit(99, 0), digit(99,1)
// midiSetup > 0
//     masterChannel -> show
//     masterChannelOut -> show
//     midiSetup == 1 -> show masterChannel
//     midiSetup == 2 -> show masterChannelOut
//     midiSetup == 3 -> digit(00,0), digit(99,1)
//     midiSetup == 0 -> digit(0,99), 1,99

// lastPot, selectedLfo, filterModeHeld

// arp_mode
// scale100(voice[i].pulsewidth_base / 2)
// voice[i].tune_base - 12
// scaleFine(voice[i].fine_base * 1023.f)
// voice[i].glide
// voice[i].attack/decay/sustain/release
// if (sync) { lfoClockSpeedPending[i] } else { lfo[i].speed / 1.3 }
// scale100(lfo[i].depth)
// scale100(cutoff)
// resonance_base
// scale100( 1023 - arp_speed_base ) << 2;
// arp_range_base
