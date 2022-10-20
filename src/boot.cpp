#include "globals.h"
#include "leds.h"
#include "boot.h"

// boot animation
byte del = 20; // sets the animation speed

void boot() {

	bool state = true;

	ledSet(1, state);
	ledSet(5, state);
	ledSet(9, state);

	delay(del);
	state = !state;
	ledSet(1, state);
	ledSet(5, state);
	ledSet(9, state);

	delay(del);
	state = !state;
	ledSet(2, state);
	ledSet(6, state);
	ledSet(10, state);
	ledSet(27, state);
	ledSet(28, state);
	ledSet(29, state);
	delay(del);
	state = !state;
	ledSet(2, state);
	ledSet(6, state);
	ledSet(10, state);
	ledSet(27, state);
	ledSet(28, state);
	ledSet(29, state);
	delay(del);
	state = !state;

	ledSet(3, state);
	ledSet(7, state);
	ledSet(11, state);
	ledSet(13, state);
	delay(del);
	state = !state;

	ledSet(3, state);
	ledSet(7, state);
	ledSet(11, state);
	ledSet(13, state);
	delay(del);
	state = !state;

	ledSet(4, state);
	ledSet(8, state);
	ledSet(12, state);
	delay(del);
	state = !state;

	ledSet(4, state);
	ledSet(8, state);
	ledSet(12, state);
	delay(del);
	state = !state;

	ledSet(14, state);
	delay(del);
	state = !state;
	ledSet(14, state);
	delay(del);
	state = !state;

	ledSet(16, state);
	ledSet(18, state);
	ledSet(20, state);
	delay(del);
	state = !state;
	ledSet(16, state);
	ledSet(18, state);
	ledSet(20, state);
	delay(del);
	state = !state;

	ledSet(17, state);
	ledSet(19, state);
	ledSet(21, state);
	ledSet(15, state);
	delay(del);
	state = !state;
	ledSet(17, state);
	ledSet(19, state);
	ledSet(21, state);
	ledSet(15, state);
	delay(del);
	state = !state;

	ledSet(22, state);
	delay(del);
	state = !state;
	ledSet(22, state);
	delay(del);
	state = !state;

	ledSet(23, state);
	delay(del);
	state = !state;
	ledSet(23, state);
	delay(del);
	state = !state;

	ledSet(24, state);
	delay(del);
	state = !state;
	ledSet(24, state);
	delay(del);
	state = !state;

	ledSet(25, state);
	ledSet(30, state);
	delay(del);
	state = !state;
	ledSet(25, state);
	ledSet(30, state);
	delay(del);
	state = !state;

	ledSet(26, state);
	ledSet(31, state);
	delay(del);
	state = !state;
	ledSet(26, state);
	ledSet(31, state);
	delay(del);
	state = !state;
}
