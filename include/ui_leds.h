#pragma once

#include <Arduino.h>

void showVersion();
void leds();
void ledSet(byte number, bool value);
void dotSet(byte dot, bool value);

/// Shows the right 7segment display's dot for a brief time.
void rightDot();
/// Shows the left 7segment display's dot for a brief time.
void leftDot();

/// Must be called periodically to turn off the dots after some time
void dotTick();

void ledNumber(int number);
void ledHex(byte value);
void digit(byte channel, byte number);

#define DIGIT_A 10
#define DIGIT_B 11
#define DIGIT_C 12
#define DIGIT_D 13
#define DIGIT_E 14
#define DIGIT_F 15
#define DIGIT_H 16
#define DIGIT_J 22
#define DIGIT_L 17
#define DIGIT_M 23
#define DIGIT_O 0
#define DIGIT_P 18
#define DIGIT_R 19
#define DIGIT_S 5
#define DIGIT_T 20
#define DIGIT_U 21

#define DIGIT_BLANK 99
