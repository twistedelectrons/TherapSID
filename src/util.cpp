#include "util.hpp"
#include "globals.h"
#include "leds.h"

void panic(int num7seg, int numvoice) {
	frozen = false;
	ledNumber(num7seg);

	int dec[3];
	dec[0] = numvoice % 10;
	dec[1] = (numvoice / 10) % 10;
	dec[2] = (numvoice / 100) % 10;
	for (int i = 0; i < 3; i++) {
		ledSet(1 + i * 4, dec[i] & 8);
		ledSet(2 + i * 4, dec[i] & 4);
		ledSet(3 + i * 4, dec[i] & 2);
		ledSet(4 + i * 4, dec[i] & 1);
	}

	int blink = 0;
	while (true) {
		ledSet(27, blink);
		ledSet(28, blink);
		ledSet(29, blink);
		blink = !blink;
		delay(333);
	}
}
