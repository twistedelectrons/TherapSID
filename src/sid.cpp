#include "globals.h"
#include "leds.h"
#include "sid.h"

static const int32_t sidScale[] = {
    137,   145,   154,   163,   173,   183,   194,   205,   217,   230,   122,   259,   274,   291,   308,
    326,   346,   366,   388,   411,   435,   461,   489,   518,   549,   581,   616,   652,   691,   732,
    776,   822,   871,   923,   976,   1036,  1097,  1163,  1232,  1305,  1383,  1465,  1552,  1644,  1742,
    1845,  1955,  2071,  2195,  2325,  2463,  2610,  2765,  2930,  3104,  3288,  3484,  3691,  3910,  4143,
    4389,  4650,  4927,  5220,  5530,  5859,  6207,  6577,  6968,  7382,  7821,  8286,  8779,  9301,  9854,
    10440, 11060, 11718, 12415, 13153, 13935, 14764, 15642, 16572, 17557, 18601, 19709, 20897, 22121, 23436,
    24830, 26306, 27871, 29528, 31234, 33144, 35115, 37203, 39415, 41759, 44242, 46873, 49660, 52613, 55741,
    59056, 62567, 33144, 35115, 37203, 39415, 41759, 44242, 46873, 49660, 52613, 55741, 59056, 62567, 33144,
    35115, 37203, 39415, 41759, 44242, 46873, 49660, 52613, 55741, 59056, 62567,

};

void sidReset() {

	// sidSend(4,B00001000);
	// sidSend(11,B00001000);
	// sidSend(18,B00001000);

	digitalWrite(A6, LOW);
	delay(50);
	digitalWrite(A6, HIGH);
}

void init1MhzClock() {
	DDRD |= _BV(7); // SID CLOCK

	// Set Timer 2 CTC mode with no prescaling.  OC2A toggles on compare match
	//
	// WGM22:0 = 010: CTC Mode, toggle OC
	// WGM2 bits 1 and 0 are in TCCR2A,
	// WGM2 bit 2 is in TCCR2B
	// COM2A0 sets OC2A (arduino pin 11 on Uno or Duemilanove) to toggle on compare match
	//
	TCCR2A = ((1 << WGM21) | (1 << COM2A0));

	// Set Timer 2  No prescaling  (i.e. prescale division = 1)
	//
	// CS22:0 = 001: Use CPU clock with no prescaling
	// CS2 bits 2:0 are all in TCCR2B
	TCCR2B = (1 << CS20);

	// Make sure Compare-match register A interrupt for timer2 is disabled
	TIMSK2 = 0;
	// This value determines the output frequency
	OCR2A = 7;
}

void sidDelay() { delayMicroseconds(4); }

void sidSend(byte address, byte data) {

	PORTC = address << 3;
	PORTB = data;

	sidDelay();

	PORTD |= _BV(3);
	PORTD &= ~_BV(6); // digitalWrite (0, LOW);
	sidDelay();
	PORTD |= _BV(6);
	PORTD &= ~_BV(3); // digitalWrite (0, LOW);

	if (fatMode) {
		if (address == 0) {
			sidSend2(address, sid2[0]);
		} else if (address == 1) {
			sidSend2(address, sid2[1]);
		} else if (address == 7) {
			sidSend2(address, sid2[2]);
		} else if (address == 8) {
			sidSend2(address, sid2[3]);
		} else if (address == 14) {
			sidSend2(address, sid2[4]);
		} else if (address == 15) {
			sidSend2(address, sid2[5]);
		} else {
			sidSend2(address, data);
		}
	} else {
		sidSend2(address, data);
	}
}

void sidSend1Only(byte address, byte data) {

	PORTC = address << 3;
	PORTB = data;

	sidDelay();

	PORTD |= _BV(3);
	PORTD &= ~_BV(6); // digitalWrite (0, LOW);
	sidDelay();
	PORTD |= _BV(6);
	PORTD &= ~_BV(3); // digitalWrite (0, LOW);
}

void sidSend2(byte address, byte data) {

	PORTC = address << 3;
	PORTB = data;
	PORTD |= _BV(3);
	sidDelay();
	PORTD &= ~_BV(2); // digitalWrite (0, LOW);
	                  // delay(2);

	sidDelay();
	PORTD |= _BV(2);
	PORTD &= ~_BV(3); // digitalWrite (0, LOW);
}

byte sidIndex;

void sidUpdate() {

	sidIndex++;
	if (sidIndex > 23)
		sidIndex = 0;

	// filter non realtime ones
	if ((sidIndex == 6) || (sidIndex == 13) || (sidIndex == 20)) {
		if (pa) {

			if ((!slot[0]) && (!slot[1]) && (!slot[2]) && (sid[sidIndex] != sidLast[sidIndex])) {
				sidLast[sidIndex] = sid[sidIndex];
				sidSend(sidIndex, sid[sidIndex]);
			}

		} else {
			if ((held < 1) && (sid[sidIndex] != sidLast[sidIndex])) {

				sidLast[sidIndex] = sid[sidIndex];
				sidSend(sidIndex, sid[sidIndex]);
			}
		}

	} else {

		if (sidIndex == 2) {
			sidSend(3, sid[3]);
			sidSend(2, sid[2]);
			sidIndex++;
		} else if (sidIndex == 9) {
			sidSend(10, sid[10]);
			sidSend(9, sid[9]);
			sidIndex++;
		} else if (sidIndex == 16) {
			sidSend(17, sid[17]);
			sidSend(16, sid[16]);
			sidIndex++;
		}

		else {

			if (sid[sidIndex] != sidLast[sidIndex]) {

				sidLast[sidIndex] = sid[sidIndex];
				sidSend(sidIndex, sid[sidIndex]);

				if (sidIndex == 0) {
					sidSend(1, sid[1]);
				} else if (sidIndex == 1) {
					sidSend(0, sid[0]);
				} else if (sidIndex == 7) {
					sidSend(8, sid[8]);
				} else if (sidIndex == 8) {
					sidSend(7, sid[7]);
				} else if (sidIndex == 14) {
					sidSend(15, sid[15]);
				} else if (sidIndex == 15) {
					sidSend(14, sid[14]);
				}
			}
		}
	}
}

int pitchB;

void sidPitch(byte voice, int pitch) {

	switch (voice) {
		case 0:

			sid[0] = pitch;
			sid[1] = pitch >> 8;

			if (fatMode == 1) {
				pitchB = pitch;
				if (pitch < 32760) {
					pitchB = pitch * 2;
				}
				sid2[0] = pitchB;
				sid2[1] = pitchB >> 8;
			} else if (fatMode > 1) {
				pitchB = pitch - fat;

				sid2[0] = pitchB;
				sid2[1] = pitchB >> 8;
			}

			break;

		case 1:
			sid[7] = pitch;
			sid[8] = pitch >> 8;

			if (fatMode == 1) {
				pitchB = pitch * 2;

				sid2[2] = pitchB;
				sid2[3] = pitchB >> 8;
			} else if (fatMode > 1) {
				pitchB = pitch - fat;

				sid2[2] = pitchB;
				sid2[3] = pitchB >> 8;
			}

			break;

		case 2:
			sid[14] = pitch;
			sid[15] = pitch >> 8;

			if (fatMode == 1) {
				pitchB = pitch * 2;

				sid2[4] = pitchB;
				sid2[5] = pitchB >> 8;
			} else if (fatMode > 1) {
				pitchB = pitch - fat;

				sid2[4] = pitchB;
				sid2[5] = pitchB >> 8;
			}

			break;
	}
}

void sidShape(byte voice, byte shape, bool value) {

	if (value) {
		switch (voice) {

			case 0:
				switch (shape) {

					case 1:
						bitWrite(sid[4], 7, 0);
						bitWrite(sid[4], 6, value);
						ledSet(1, value);
						ledSet(4, 0);
						break;
					case 2:
						bitWrite(sid[4], 7, 0);
						bitWrite(sid[4], 4, value);
						ledSet(2, value);
						ledSet(4, 0);
						break;
					case 3:
						bitWrite(sid[4], 7, 0);
						bitWrite(sid[4], 5, value);
						ledSet(3, value);
						ledSet(4, 0);
						break;
					case 4:
						bitWrite(sid[4], 6, 0);
						bitWrite(sid[4], 4, 0);
						bitWrite(sid[4], 5, 0);
						bitWrite(sid[4], 7, value);
						ledSet(4, value);
						ledSet(1, 0);
						ledSet(2, 0);
						ledSet(3, 0);
						break;
				}
				break;

			case 1:
				switch (shape) {
					case 1:
						bitWrite(sid[11], 7, 0);
						bitWrite(sid[11], 6, value);
						ledSet(5, value);
						ledSet(8, 0);
						break;
					case 2:
						bitWrite(sid[11], 7, 0);
						bitWrite(sid[11], 4, value);
						ledSet(6, value);
						ledSet(8, 0);
						break;
					case 3:
						bitWrite(sid[11], 7, 0);
						bitWrite(sid[11], 5, value);
						ledSet(7, value);
						ledSet(8, 0);
						break;
					case 4:
						bitWrite(sid[11], 6, 0);
						bitWrite(sid[11], 4, 0);
						bitWrite(sid[11], 5, 0);
						bitWrite(sid[11], 7, value);
						ledSet(8, value);
						ledSet(5, 0);
						ledSet(6, 0);
						ledSet(7, 0);
						break;
				}
				break;

			case 2:
				switch (shape) {
					case 1:
						bitWrite(sid[18], 7, 0);
						bitWrite(sid[18], 6, value);
						ledSet(9, value);
						ledSet(12, 0);
						break;
					case 2:
						bitWrite(sid[18], 7, 0);
						bitWrite(sid[18], 4, value);
						ledSet(10, value);
						ledSet(12, 0);
						break;
					case 3:
						bitWrite(sid[18], 7, 0);
						bitWrite(sid[18], 5, value);
						ledSet(11, value);
						ledSet(12, 0);
						break;
					case 4:
						bitWrite(sid[18], 6, 0);
						bitWrite(sid[18], 4, 0);
						bitWrite(sid[18], 5, 0);
						bitWrite(sid[18], 7, value);
						ledSet(12, value);
						ledSet(9, 0);
						ledSet(10, 0);
						ledSet(11, 0);
						break;
				}
				break;
		}

	} else {
		switch (voice) {

			case 0:
				switch (shape) {
					case 1:
						bitWrite(sid[4], 6, value);
						ledSet(1, value);
						break;
					case 2:
						bitWrite(sid[4], 4, value);
						ledSet(2, value);
						break;
					case 3:
						bitWrite(sid[4], 5, value);
						ledSet(3, value);
						break;
					case 4:
						bitWrite(sid[4], 7, value);
						ledSet(4, value);
						break;
				}
				break;

			case 1:
				switch (shape) {
					case 1:
						bitWrite(sid[11], 6, value);
						ledSet(5, value);
						break;
					case 2:
						bitWrite(sid[11], 4, value);
						ledSet(6, value);
						break;
					case 3:
						bitWrite(sid[11], 5, value);
						ledSet(7, value);
						break;
					case 4:
						bitWrite(sid[11], 7, value);
						ledSet(8, value);
						break;
				}
				break;

			case 2:
				switch (shape) {
					case 1:
						bitWrite(sid[18], 6, value);
						ledSet(9, value);
						break;
					case 2:
						bitWrite(sid[18], 4, value);
						ledSet(10, value);
						break;
					case 3:
						bitWrite(sid[18], 5, value);
						ledSet(11, value);
						break;
					case 4:
						bitWrite(sid[18], 7, value);
						ledSet(12, value);
						break;
				}
				break;
		}
	}
}

void updateFilter() {

	if (filterMode == 0) {
		// lp
		sid[24] = B00010000;

	}

	else if (filterMode == 1) {

		// bp
		sid[24] = B00100000;

	}

	else if (filterMode == 2) {
		// hp
		sid[24] = B01000000;

	}

	else if (filterMode == 3) {

		// lp+hp
		sid[24] = B01010000;
	}

	if (filterMode == 4) {

		// off
		sid[24] = B00000000;
	}

	if (filterMode != 4) {
		bitWrite(sid[23], 0, 1);
		bitWrite(sid[23], 1, 1);
		bitWrite(sid[23], 2, 1);
		bitWrite(sid[23], 3, 1);
	} else {

		bitWrite(sid[23], 0, 0);
		bitWrite(sid[23], 1, 0);
		bitWrite(sid[23], 2, 0);
		bitWrite(sid[23], 3, 0);
	}

	ledSet(27, bitRead(sid[24], 4));
	ledSet(28, bitRead(sid[24], 5));
	ledSet(29, bitRead(sid[24], 6));

	// unMute
	sidSend(24, sid[24]);

	delay(100);

	bitWrite(sid[24], 0, 1);
	bitWrite(sid[24], 1, 1);
	bitWrite(sid[24], 2, 1);
	bitWrite(sid[24], 3, 1);
	sidSend(24, sid[24]);

	setFilterBit(0);
	setFilterBit(1);
	setFilterBit(2);
}

void calculatePitch() {

	if ((!note1) && (!note2) && (!note3)) {

		// no individual channels

		if (!pa) {
			// Pitch
			int temp = -1 + key + tuneBase1 + lfoTune1 + lfoTune2 + lfoTune3;

			if (temp > 127) {
				temp = 127;
			} else if (temp - 12 < 0) {
				temp = 12;
			}

			fine1 = fineBase1 + lfoFine2 + lfoFine1 + lfoFine3;
			if (fine1 > 1)
				fine1 = 1;
			fine1 *= .90;

			if (bend > 0) {
				destiPitch1 = sidScale[temp - 12] +
				              ((sidScale[key + tuneBase1 - 10] - sidScale[key + tuneBase1 - 12]) * fine1) +
				              ((sidScale[key + tuneBase1 - 10] - sidScale[key + tuneBase1 - 12]) * bend);
			} else if (bend < 0) {

				destiPitch1 = sidScale[temp - 12] +
				              ((sidScale[key + tuneBase1 - 10] - sidScale[key + tuneBase1 - 12]) * fine1) -
				              ((sidScale[key + tuneBase1 - 12] - sidScale[key + tuneBase1 - 10]) * bend);
			} else {
				destiPitch1 =
				    sidScale[temp - 12] + ((sidScale[key + tuneBase1 - 10] - sidScale[key + tuneBase1 - 12]) * fine1);
			}

			temp = -1 + key + tuneBase2 + lfoTune4 + lfoTune5 + lfoTune6;
			if (temp > 127) {
				temp = 127;
			} else if (temp - 12 < 0) {
				temp = 12;
			}

			fine2 = fineBase2 + lfoFine4 + lfoFine5 + lfoFine6;
			if (fine2 > 1)
				fine2 = 1;
			fine2 *= .90;

			if (bend > 0) {
				destiPitch2 = sidScale[temp - 12] +
				              ((sidScale[key + tuneBase2 - 10] - sidScale[key + tuneBase2 - 12]) * fine2) +
				              ((sidScale[key + tuneBase2 - 10] - sidScale[key + tuneBase2 - 12]) * bend);
			} else if (bend < 0) {
				destiPitch2 = sidScale[temp - 12] +
				              ((sidScale[key + tuneBase2 - 10] - sidScale[key + tuneBase2 - 12]) * fine2) -
				              ((sidScale[key + tuneBase2 - 12] - sidScale[key + tuneBase2 - 10]) * bend);
			} else {
				destiPitch2 =
				    sidScale[temp - 12] + ((sidScale[key + tuneBase2 - 10] - sidScale[key + tuneBase2 - 12]) * fine2);
			}

			temp = -1 + key + tuneBase3 + lfoTune7 + lfoTune8 + lfoTune9;
			if (temp > 127) {
				temp = 127;
			} else if (temp - 12 < 0) {
				temp = 12;
			}

			fine3 = fineBase3 + lfoFine7 + lfoFine8 + lfoFine9;
			if (fine3 > 1)
				fine3 = 1;
			fine3 *= .90;

			if (bend > 0) {
				destiPitch3 = sidScale[temp - 12] +
				              ((sidScale[key + tuneBase3 - 10] - sidScale[key + tuneBase3 - 12]) * fine3) +
				              ((sidScale[key + tuneBase3 - 10] - sidScale[key + tuneBase3 - 12]) * bend);
			} else if (bend < 0) {
				destiPitch3 = sidScale[temp - 12] +
				              ((sidScale[key + tuneBase3 - 10] - sidScale[key + tuneBase3 - 12]) * fine3) -
				              ((sidScale[key + tuneBase3 - 12] - sidScale[key + tuneBase3 - 10]) * bend);
			} else {
				destiPitch3 =
				    sidScale[temp - 12] + ((sidScale[key + tuneBase3 - 10] - sidScale[key + tuneBase3 - 12]) * fine3);
			}

			sidPitch(0, pitch1);
			sidPitch(1, pitch2);
			sidPitch(2, pitch3);
		} else {

			// paraphonic

			// Pitch
			int temp = -1 + pKey[0] + tuneBase1 + lfoTune1 + lfoTune2 + lfoTune3;
			if (temp > 127) {
				temp = 127;
			} else if (temp - 12 < 0) {
				temp = 12;
			}
			fine1 = fineBase1 + lfoFine2 + lfoFine1 + lfoFine3;
			if (fine1 > 1)
				fine1 = 1;

			if (bend > 0) {
				destiPitch1 = sidScale[temp - 12] +
				              ((sidScale[pKey[0] + tuneBase1 - 10] - sidScale[pKey[0] + tuneBase1 - 12]) * fine1) +
				              ((sidScale[pKey[0] + tuneBase1 - 10] - sidScale[pKey[0] + tuneBase1 - 12]) * bend);
			} else if (bend < 0) {

				destiPitch1 = sidScale[temp - 12] +
				              ((sidScale[pKey[0] + tuneBase1 - 10] - sidScale[pKey[0] + tuneBase1 - 12]) * fine1) -
				              ((sidScale[pKey[0] + tuneBase1 - 12] - sidScale[pKey[0] + tuneBase1 - 10]) * bend);
			} else {
				destiPitch1 = sidScale[temp - 12] +
				              ((sidScale[pKey[0] + tuneBase1 - 10] - sidScale[pKey[0] + tuneBase1 - 12]) * fine1);
			}

			// destiPitch1=sidScale[temp-12]+((sidScale[pKey[0]+tuneBase1-10]-sidScale[pKey[0]+tuneBase1-12])*fine1);

			temp = -1 + pKey[1] + tuneBase2 + lfoTune4 + lfoTune5 + lfoTune6;
			if (temp > 127) {
				temp = 127;
			} else if (temp - 12 < 0) {
				temp = 12;
			}
			fine2 = fineBase2 + lfoFine4 + lfoFine5 + lfoFine6;
			if (fine2 > 1)
				fine2 = 1;

			if (bend > 0) {
				destiPitch2 = sidScale[temp - 12] +
				              ((sidScale[pKey[1] + tuneBase2 - 10] - sidScale[pKey[1] + tuneBase2 - 12]) * fine2) +
				              ((sidScale[pKey[1] + tuneBase2 - 10] - sidScale[pKey[1] + tuneBase2 - 12]) * bend);
			} else if (bend < 0) {
				destiPitch2 = sidScale[temp - 12] +
				              ((sidScale[pKey[1] + tuneBase2 - 10] - sidScale[pKey[1] + tuneBase2 - 12]) * fine2) -
				              ((sidScale[pKey[1] + tuneBase2 - 12] - sidScale[pKey[1] + tuneBase2 - 10]) * bend);
			} else {
				destiPitch2 = sidScale[temp - 12] +
				              ((sidScale[pKey[1] + tuneBase2 - 10] - sidScale[pKey[1] + tuneBase2 - 12]) * fine2);
			}

			// destiPitch2=sidScale[temp-12]+((sidScale[pKey[1]+tuneBase2-10]-sidScale[pKey[1]+tuneBase2-12])*fine2);

			temp = -1 + pKey[2] + tuneBase3 + lfoTune7 + lfoTune8 + lfoTune9;
			if (temp > 127) {
				temp = 127;
			} else if (temp - 12 < 0) {
				temp = 12;
			}
			fine3 = fineBase3 + lfoFine7 + lfoFine8 + lfoFine9;
			if (fine3 > 1)
				fine3 = 1;

			if (bend > 0) {
				destiPitch3 = sidScale[temp - 12] +
				              ((sidScale[pKey[2] + tuneBase3 - 10] - sidScale[pKey[2] + tuneBase3 - 12]) * fine3) +
				              ((sidScale[pKey[2] + tuneBase3 - 10] - sidScale[pKey[2] + tuneBase3 - 12]) * bend);
			} else if (bend < 0) {
				destiPitch3 = sidScale[temp - 12] +
				              ((sidScale[pKey[2] + tuneBase3 - 10] - sidScale[pKey[2] + tuneBase3 - 12]) * fine3) -
				              ((sidScale[pKey[2] + tuneBase3 - 12] - sidScale[pKey[2] + tuneBase3 - 10]) * bend);
			} else {
				destiPitch3 = sidScale[temp - 12] +
				              ((sidScale[pKey[2] + tuneBase3 - 10] - sidScale[pKey[2] + tuneBase3 - 12]) * fine3);
			}

			// destiPitch3=sidScale[temp-12]+((sidScale[pKey[2]+tuneBase3-10]-sidScale[pKey[2]+tuneBase3-12])*fine3);

			sidPitch(0, pitch1);
			sidPitch(1, pitch2);
			sidPitch(2, pitch3);
		}
	} else {

		int temp;

		// individual channels
		if (note1) {
			// Pitch
			temp = -1 + note1 + tuneBase1 + lfoTune1 + lfoTune2 + lfoTune3;
			if (temp > 127) {
				temp = 127;
			} else if (temp - 12 < 0) {
				temp = 12;
			}
			fine1 = fineBase1 + lfoFine2 + lfoFine1 + lfoFine3;
			if (fine1 > 1)
				fine1 = 1;

			if (bend1 > 0) {
				destiPitch1 = sidScale[temp - 12] +
				              ((sidScale[note1 + tuneBase1 - 10] - sidScale[note1 + tuneBase1 - 12]) * fine1) +
				              ((sidScale[note1 + tuneBase1 - 10] - sidScale[note1 + tuneBase1 - 12]) * bend1);
			} else if (bend1 < 0) {

				destiPitch1 = sidScale[temp - 12] +
				              ((sidScale[note1 + tuneBase1 - 10] - sidScale[note1 + tuneBase1 - 12]) * fine1) -
				              ((sidScale[note1 + tuneBase1 - 12] - sidScale[note1 + tuneBase1 - 10]) * bend1);
			} else {
				destiPitch1 = sidScale[temp - 12] +
				              ((sidScale[note1 + tuneBase1 - 10] - sidScale[note1 + tuneBase1 - 12]) * fine1);
			}
		}

		if (note2) {
			// Pitch
			temp = -1 + note2 + tuneBase2 + lfoTune4 + lfoTune5 + lfoTune6;
			if (temp > 127) {
				temp = 127;
			} else if (temp - 12 < 0) {
				temp = 12;
			}
			fine2 = fineBase2 + lfoFine4 + lfoFine5 + lfoFine6;
			if (fine2 > 1)
				fine2 = 1;

			if (bend2 > 0) {
				destiPitch2 = sidScale[temp - 12] +
				              ((sidScale[note2 + tuneBase2 - 10] - sidScale[note2 + tuneBase2 - 12]) * fine2) +
				              ((sidScale[note2 + tuneBase2 - 10] - sidScale[note2 + tuneBase2 - 12]) * bend2);
			} else if (bend2 < 0) {
				destiPitch2 = sidScale[temp - 12] +
				              ((sidScale[note2 + tuneBase2 - 10] - sidScale[note2 + tuneBase2 - 12]) * fine2) -
				              ((sidScale[note2 + tuneBase2 - 12] - sidScale[note2 + tuneBase2 - 10]) * bend2);
			} else {
				destiPitch2 = sidScale[temp - 12] +
				              ((sidScale[note2 + tuneBase2 - 10] - sidScale[note2 + tuneBase2 - 12]) * fine2);
			}
		}
		if (note3) {

			temp = -1 + note3 + tuneBase3 + lfoTune7 + lfoTune8 + lfoTune9;
			if (temp > 127) {
				temp = 127;
			} else if (temp - 12 < 0) {
				temp = 12;
			}
			fine3 = fineBase3 + lfoFine7 + lfoFine8 + lfoFine9;
			if (fine3 > 1)
				fine3 = 1;

			if (bend3 > 0) {
				destiPitch3 = sidScale[temp - 12] +
				              ((sidScale[note3 + tuneBase3 - 10] - sidScale[note3 + tuneBase3 - 12]) * fine3) +
				              ((sidScale[note3 + tuneBase3 - 10] - sidScale[note3 + tuneBase3 - 12]) * bend3);
			} else if (bend3 < 0) {
				destiPitch3 = sidScale[temp - 12] +
				              ((sidScale[note3 + tuneBase3 - 10] - sidScale[note3 + tuneBase3 - 12]) * fine3) -
				              ((sidScale[note3 + tuneBase3 - 12] - sidScale[note3 + tuneBase3 - 10]) * bend3);
			} else {
				destiPitch3 = sidScale[temp - 12] +
				              ((sidScale[note3 + tuneBase3 - 10] - sidScale[note3 + tuneBase3 - 12]) * fine3);
			}
		}

		sidPitch(0, pitch1);
		sidPitch(1, pitch2);
		sidPitch(2, pitch3);
	}
}

void updateFatMode() {

	// change fatMode

	if (fatMode == 2) {
		fat = 15;
	} else if (fatMode == 3) {
		fat = 50;
	}
	fatChanged = true;
}

void setFilterBit(byte channel) {

	switch (channel) {
		case 0:
			if ((!bitRead(sid[4], 7)) && (!bitRead(sid[4], 6)) && (!bitRead(sid[4], 5)) && (!bitRead(sid[4], 4))) {
				bitWrite(sid[23], 0, 0);
			} else {
				if (filterMode == 4) {
					bitWrite(sid[23], 0, 0);
				} else {
					bitWrite(sid[23], 0, filterEnabled[channel]);
				}
			}
			break;
		case 1:
			if ((!bitRead(sid[11], 7)) && (!bitRead(sid[11], 6)) && (!bitRead(sid[11], 5)) && (!bitRead(sid[11], 4))) {
				bitWrite(sid[23], 1, 0);
			} else {
				if (filterMode == 4) {
					bitWrite(sid[23], 1, 0);
				} else {
					bitWrite(sid[23], 1, filterEnabled[channel]);
				}
			}
			break;
		case 2:
			if ((!bitRead(sid[18], 7)) && (!bitRead(sid[18], 6)) && (!bitRead(sid[18], 5)) && (!bitRead(sid[18], 4))) {
				bitWrite(sid[23], 2, 0);
			} else {
				if (filterMode == 4) {
					bitWrite(sid[23], 2, 0);
				} else {
					bitWrite(sid[23], 2, filterEnabled[channel]);
				}
			}
			break;
	}
}
