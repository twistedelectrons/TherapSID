#include "util.hpp"
#include "ui_leds.h"

void panic(int num7seg, int numvoice) {
	digit(0, num7seg / 10);
	digit(1, num7seg % 10);

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

void trace(int num7seg, int numvoice) {
	digit(0, num7seg / 10);
	digit(1, num7seg % 10);

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

	int blink = 1;
	for (int i = 0; i < 6; i++) {
		ledSet(27, blink);
		ledSet(28, blink);
		ledSet(29, blink);
		blink = !blink;
		delay(333);
	}
}

// linear conversion
int rescale(int val, int minO, int maxO, int minN, int maxN) {
	return (val - minO) / (float)(maxO - minO) * (maxN - minN) + minN;
}