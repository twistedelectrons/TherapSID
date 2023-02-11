#include <Arduino.h>

extern bool fatShow; // UI
extern bool scrolled; // UI
extern bool shape1Pressed; // UI
extern int shape1PressedTimer; // UI
extern int resetDownTimer; // UI
extern int presetScrollSpeed; // UI
extern int saveBounce; // UI
extern bool presetUp, presetDown; // UI
extern int presetLast; // UI??
extern bool fatChanged; // UI
extern bool resetDown; // UI
extern int frozen; // UI
extern bool lfoButtPressed; // UI
extern int lfoButtTimer; // UI
extern bool saveMode; // UI
extern int saveModeTimer; // UI
extern int dotTimer; // UI
extern bool showPresetNumber; // UI // when high we show preset number once

extern bool filterModeHeld; // UI // FIXME: need to update LEDs when changing this
extern byte lastPot; // UI
extern byte selectedLfo; // UI

extern int arpModeCounter;
extern int loadTimer;
