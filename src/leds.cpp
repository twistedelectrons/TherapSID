#include "globals.h"
#include "leds.h"
#include "display.h"

void showVersion() {
	digit(1, versionDecimal);
	digit(0, version);
	mydisplay.setLed(0, 7, 6, 1);
}

void leds() { ledSet(32, 1); }

void showFilterAssigns() {
	for (int voice = 0; voice < 3; voice++) {
		for (int i=0; i<4; i++) {
			ledSet(1+4*voice+i, preset_data.voice[voice].filter_enabled);
		}
	}
}

void unShowFilterAssigns() {
	for (int voice = 0; voice < 3; voice++) {
		ledSet(4*voice + 1, preset_data.voice[voice].reg_control & PresetVoice::PULSE);
		ledSet(4*voice + 2, preset_data.voice[voice].reg_control & PresetVoice::TRI);
		ledSet(4*voice + 3, preset_data.voice[voice].reg_control & PresetVoice::SAW);
		ledSet(4*voice + 4, preset_data.voice[voice].reg_control & PresetVoice::NOISE);
	}
}

void ledSet(byte number, bool value) {
	int number0based = number - 1;
	if (number0based < 31) {
		mydisplay.setLed(0, number0based % 8, 1 + number0based / 8, value);
	}
}

/*
  0
5   1
  6
4   2
  3   7
*/

void rightDot() {

	mydisplay.setLed(0, 7, 7, 1);

	dotTimer = 50;
}

void leftDot() {

	mydisplay.setLed(0, 7, 6, 1);
	dotTimer = 50;
}

void ledNumber(int number) {
	if (!frozen) {
		if (number < 0)
			number = -number;
		if (number < 100) {
			digit(0, number / 10);
			digit(1, number % 10);
		}
	}
}
void digit(uint8_t channel, uint8_t number) {
	const uint8_t digits[] = {
		0b0111111, // 0
		0b0000110, // 1
		0b1011011, // 2
		0b1001111, // 3
		0b1100110, // 4
		0b1101101, // 5
		0b1111101, // 6
		0b0000111, // 7
		0b1111111, // 8
		0b1100111, // 9
		0b0111001, // 10 = C
		0b0111000, // 11 = L
		0b1110001, // 12 = F
		0b0111110, // 13 = U
		0b1110011, // 14 = P
		0b1011110, // 15 = d
		0b1010000, // 16 = r
		0b1110111, // 17 = A
		0b1111001, // 18 = E
		0b0000000  // 19 = <blank>
	};
	const auto DIGITS_LEN = sizeof(digits) / sizeof(*digits);

	if (number >= DIGITS_LEN) number = DIGITS_LEN - 1;

	for (int i=0; i<7; i++)
		mydisplay.setLed(0, i, 6 + channel, digits[number] & (1<<i));
}
