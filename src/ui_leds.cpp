#include "ui_leds.h"
#include "display.h"
#include "ui_vars.h"

static int dotTimer = 0;

void leds() { ledSet(32, 1); }

void ledSet(byte number, bool value) {
	int number0based = number - 1;
	if (number0based < 31) {
		mydisplay.setLed(0, number0based % 8, 1 + number0based / 8, value);
	}
}

void dotSet(byte dot, bool state) { mydisplay.setLed(0, 7, dot ? 7 : 6, state); }

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

void dotTick() {
	if (dotTimer) {
		dotTimer--;
		if (!dotTimer) {
			mydisplay.setLed(0, 7, 6, 0);
			mydisplay.setLed(0, 7, 7, 0);
		}
	}
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

void ledHex(byte value) {
	digit(0, value >> 4);
	digit(1, value & 0x0f);
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
	    0b1110111, // 10 = A
	    0b1111100, // 11 = B
	    0b0111001, // 12 = C
	    0b1011110, // 13 = d
	    0b1111001, // 14 = E
	    0b1110001, // 15 = F
	    0b1110110, // 16 = H
	    0b0111000, // 17 = L
	    0b1110011, // 18 = P
	    0b1010000, // 19 = r
	    0b1111000, // 20 = t
	    0b0111110, // 21 = U
	    0b0000000, // 22 = <blank>
	};
	const auto DIGITS_LEN = sizeof(digits) / sizeof(*digits);

	if (number >= DIGITS_LEN)
		number = DIGITS_LEN - 1;

	for (int i = 0; i < 7; i++)
		mydisplay.setLed(0, i, 6 + channel, digits[number] & (1 << i));
}
