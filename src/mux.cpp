#include <Arduino.h>
#include <digitalWriteFast.h>

#include "globals.h"
#include "mux.h"
#include "pots.h"
#include "buttons.h"

byte muxChannel;
int pot;
bool butt;

void readMux() {

	muxChannel++;
	if (muxChannel > 15) {
		muxChannel = 0;
	}
	mux(muxChannel);

	// pots
	pot = ((analogRead(A0) + analogRead(A0)) / 2) >> 3;

	if ((pot > potLast[muxChannel] + 1) || (pot < potLast[muxChannel] - 1)) {
		potLast[muxChannel] = pot;
		movedPot(muxChannel, pot << 3, 0);
	}

	pot = ((analogRead(A1) + analogRead(A1)) / 2) >> 3;

	if ((pot > potLast[muxChannel + 16] + 1) || (pot < potLast[muxChannel + 16] - 1)) {
		potLast[muxChannel + 16] = pot;
		movedPot(muxChannel + 16, pot << 3, 0);
	}

	if ((muxChannel == 0) || (muxChannel == 4) || (muxChannel == 9)) {
		pot = ((analogRead(A2) + analogRead(A2)) / 2) >> 3;

		if ((pot > potLast[muxChannel + 32] + 1) || (pot < potLast[muxChannel + 32] - 1)) {
			potLast[muxChannel + 32] = pot;
			movedPot(muxChannel + 32, pot << 3, 0);
		}
	}

	// butts
	butt = (PINA & _BV(3)) == 0;
	if (butt != buttLast[muxChannel]) {
		buttLast[muxChannel] = butt;
		buttChanged(muxChannel, !butt);
	}
	butt = (PINA & _BV(4)) == 0;
	if (butt != buttLast[muxChannel + 16]) {
		buttLast[muxChannel + 16] = butt;
		buttChanged(muxChannel + 16, !butt);
	}

	if (muxChannel == 6) {
		digitalWrite(A2, HIGH);
		butt = digitalRead(A2);
		if (butt != buttLast[32]) {
			buttLast[32] = butt;
			buttChanged(32, butt);
		}
		digitalWriteFast(A2, LOW);
	}
}

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

// PORTB |= _BV (0); //  HIGH;
// PORTB &= ~_BV (0); //  LOW;

void Aon() { PORTB |= _BV(0); }
void Aoff() { PORTB &= ~_BV(0); }

void Bon() { PORTB |= _BV(1); }
void Boff() { PORTB &= ~_BV(1); }

void Con() { PORTB |= _BV(2); }
void Coff() { PORTB &= ~_BV(2); }

void Don() { PORTB |= _BV(3); }
void Doff() { PORTB &= ~_BV(3); }
