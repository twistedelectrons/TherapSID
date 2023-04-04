#include "ui_leds.h"
#include "boot.h"
#include "version.h"

void showVersion() {
	digit(1, versionDecimal);
	digit(0, version);
	leftDot();
}

// IDs for LEDs per column, separated by zeroes
const byte ledOrder[] = {
    1,  5,  9,  0, 2,  6,  10, 27, 28, 29, 0, 3,  7, 11, 13, 0,  4,  8, 12, 0,  14, 0,
    16, 18, 20, 0, 17, 19, 21, 15, 0,  22, 0, 23, 0, 24, 0,  25, 30, 0, 26, 31, 0,
};

// boot animation
void boot() {

	// Animate each LED column, one at a time
	bool state = true;
	byte i, j;
	i = j = 0;
	do {
		if (ledOrder[i] != 0) {
			ledSet(ledOrder[i], state);
			i++;
		} else {
			if (state) {
				// Column is now on, do same one again to turn it off
				i = j;
			} else {
				// Column is now off, continue to next column
				i++;
				j = i;
			}
			delay(20);
			state = !state;
		}
	} while (i < sizeof(ledOrder));
}
