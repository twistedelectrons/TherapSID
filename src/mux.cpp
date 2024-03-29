#include <Arduino.h>
#include <digitalWriteFast.h>

#include "mux.h"
#include "ui_pots.h"
#include "ui_buttons.h"

static byte muxChannel;
static bool butt;
static bool buttLast[33];
static int potLast[42];

const int POT_MAX = sizeof(potLast) / sizeof(*potLast); // number of array entries
const int HYSTERESIS = 3;

void setupMux() {

	buttLast[32] = 1;

	// scan all the pots so movement isn't detected after boot

	for (int i = 0; i < 16; i++) {
		mux(i);
		potLast[i] = analogRead(A0);
		potLast[i + 16] = analogRead(A1);
		if (i + 32 < POT_MAX)
			potLast[i + 32] = analogRead(A2);
	}
}
void readMux() {

	muxChannel++;
	if (muxChannel > 15) {
		muxChannel = 0;
	}
	mux(muxChannel);

	// pots
	int pot = analogRead(A0);

	if ((pot > potLast[muxChannel] + HYSTERESIS) || (pot < potLast[muxChannel] - HYSTERESIS)) {
		potLast[muxChannel] = pot;
		movedPot(muxChannel, pot, 0);
	}

	mux(muxChannel);
	pot = analogRead(A1);

	if ((pot > potLast[muxChannel + 16] + HYSTERESIS) || (pot < potLast[muxChannel + 16] - HYSTERESIS)) {
		potLast[muxChannel + 16] = pot;
		movedPot(muxChannel + 16, pot, 0);
	}

	if ((muxChannel == 0) || (muxChannel == 4) || (muxChannel == 9)) {
		mux(muxChannel);
		pot = analogRead(A2);

		if ((pot > potLast[muxChannel + 32] + HYSTERESIS) || (pot < potLast[muxChannel + 32] - HYSTERESIS)) {
			potLast[muxChannel + 32] = pot;
			movedPot(muxChannel + 32, pot, 0);
		}
	}

	// butts
	mux(muxChannel);
	butt = (PINA & _BV(3)) == 0;
	if (butt != buttLast[muxChannel]) {
		buttLast[muxChannel] = butt;
		buttChanged(muxChannel, !butt);
	}
	mux(muxChannel);
	butt = (PINA & _BV(4)) == 0;
	if (butt != buttLast[muxChannel + 16]) {
		buttLast[muxChannel + 16] = butt;
		buttChanged(muxChannel + 16, !butt);
	}

	if (muxChannel == 6) {
		digitalWrite(A2, HIGH);
		mux(muxChannel);
		butt = digitalRead(A2);
		if (butt != buttLast[32]) {
			buttLast[32] = butt;
			buttChanged(32, butt);
		}
		digitalWriteFast(A2, LOW);
	}
}

static void Aon() { PORTB |= _BV(0); }
static void Aoff() { PORTB &= ~_BV(0); }

static void Bon() { PORTB |= _BV(1); }
static void Boff() { PORTB &= ~_BV(1); }

static void Con() { PORTB |= _BV(2); }
static void Coff() { PORTB &= ~_BV(2); }

static void Don() { PORTB |= _BV(3); }
static void Doff() { PORTB &= ~_BV(3); }

void mux(byte number) {

	switch (number) {

		case 0:
			Aoff();
			Boff();
			Coff();
			Doff();

			break;

		case 1:
			Aoff();
			Boff();
			Coff();
			Don();

			break;

		case 2:
			Aoff();
			Boff();
			Con();
			Doff();

			break;

		case 3:
			Aoff();
			Boff();
			Con();
			Don();

			break;

		case 4:
			Aoff();
			Bon();
			Coff();
			Doff();

			break;

		case 5:
			Aoff();
			Bon();
			Coff();
			Don();

			break;

		case 6:
			Aoff();
			Bon();
			Con();
			Doff();

			break;

		case 7:
			Aoff();
			Bon();
			Con();
			Don();

			break;

		case 8:
			Aon();
			Boff();
			Coff();
			Doff();

			break;

		case 9:
			Aon();
			Boff();
			Coff();
			Don();

			break;

		case 10:
			Aon();
			Boff();
			Con();
			Doff();

			break;

		case 11:
			Aon();
			Boff();
			Con();
			Don();

			break;

		case 12:
			Aon();
			Bon();
			Coff();
			Doff();

			break;

		case 13:
			Aon();
			Bon();
			Coff();
			Don();
			break;

		case 14:
			Aon();
			Bon();
			Con();
			Doff();
			break;

		case 15:
			Aon();
			Bon();
			Con();
			Don();
			break;
	}
}
