#pragma once

#include <Arduino.h>

void showVersion();
void leds();
void ledSet(byte number, bool value);

/// Shows the right 7segment display's dot for a brief time.
void rightDot();
/// Shows the left 7segment display's dot for a brief time.
void leftDot();

/// Must be called periodically to turn off the dots after some time
void dotTick();

void ledNumber(int number);
void digit(byte channel, byte number);
