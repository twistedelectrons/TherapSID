#include <EEPROM.h>
#include <TimerOne.h>

#include "globals.h"
#include "leds.h"
#include "midi.h"
#include "arp.h"
#include "sid.h"
#include "lfo.h"
#include "pots.h"

static byte mStatus;
static byte mData;
static byte mChannel;

static byte syncLfoCounter;
static float lfoClockRates[] = {2.6562, 5.3125, 7.96875, 10.625, 21.25, 31.875, 42.5, 85};

static int noteHeld1, noteHeld2, noteHeld3;
static float lfoStepF[3];
static byte lfoClockSpeed[3];
static int clockCount;
static bool pedal;
static bool thru;
static byte velocityLast;

static int dumpCounter = 0;
static byte dump;
static bool alternator;
static byte flash;

static byte val[] = {0, 128};

static void HandleNoteOn(byte channel, byte note, byte velocity) {

	note -= 12;

	if (note < 95) {

		if (masterChannel == 1) {

			if (!gate) {

				if (channel == masterChannel) {

					if (!pa) {

						//////////////////////////////////////////////////////////////////////////////
						//                              MONOPHONIC                                  //
						//////////////////////////////////////////////////////////////////////////////
						if (!velocity) {

							held--;

							heldKeys[note] = 0;

							// check highest key

							if ((held) && (!arpMode)) {
								for (int i = 0; i < 128; i++) {
									if (heldKeys[i])
										key = i;
								}
							}

							if (!pedal) {
								// note off

								if (held < 1) {
									held = 0;
									envState = 4;
									arpRound = 0;
									// memset(heldKeys, 0, sizeof(heldKeys));
									bitWrite(sid[4], 0, 0);
									bitWrite(sid[11], 0, 0);
									bitWrite(sid[18], 0, 0);
								}
								// ledNumber(held);
							} else {
							}

						} else {
							velocityLast = velocity;

							// note on;

							heldKeys[note] = 1;

							if (retrig[0]) {
								lfoStep[0] = lfoStepF[0] = 0;
							}
							if (retrig[1]) {
								lfoStep[1] = lfoStepF[1] = 0;
							}
							if (retrig[2]) {
								lfoStep[2] = lfoStepF[2] = 0;
							}
							held++;

							if (!arpMode) {
								key = note;
							}

							if (held == 1) {
								envState = 1;
								env = 0;
								clockCount = arpRate;

								if (arpMode)
									arpReset(note);

								bitWrite(sid[4], 0, 1);
								bitWrite(sid[11], 0, 1);
								bitWrite(sid[18], 0, 1);
							}
						}

					} else {
						//////////////////////////////////////////////////////////////////////////////
						//                              PARAPHONIC                                  //
						//////////////////////////////////////////////////////////////////////////////
						if (!velocity) {
							// note off

							// free a slot

							if (slot[0] == note) {
								slot[0] = 0;
								if (!pedal)
									bitWrite(sid[4], 0, 0);
							} else if (slot[1] == note) {
								slot[1] = 0;
								if (!pedal)
									bitWrite(sid[11], 0, 0);
							} else if (slot[2] == note) {
								slot[2] = 0;
								if (!pedal)
									bitWrite(sid[18], 0, 0);
							}

						} else {
							// note on

							// look for slot

							if (!slot[0]) {
								slot[0] = note;
								bitWrite(sid[4], 0, 1);
								pKey[0] = note;
							} else if (!slot[1]) {
								slot[1] = note;
								bitWrite(sid[11], 0, 1);
								pKey[1] = note;
							} else if (!slot[2]) {
								slot[2] = note;
								bitWrite(sid[18], 0, 1);
								pKey[2] = note;
							}

							// overflow onto voice 3
							else {
								slot[2] = note;
								bitWrite(sid[18], 0, 1);
								pKey[2] = note;
							}
						}
					}

				} else if (channel == 2) {
					if (!pa) {

						if (!velocity) {
							noteHeld1--;
							if (noteHeld1 < 1) {
								noteHeld1 = 0;
								note1 = 0;
								bitWrite(sid[4], 0, 0);
								// bitWrite(sid[11],0,0);
								// bitWrite(sid[18],0,0);
							}
						} else {
							noteHeld1++;
							note1 = note;
							if (noteHeld1 == 1) {
								bitWrite(sid[4], 0, 1);
								// bitWrite(sid[11],0,1);
								// bitWrite(sid[18],0,1);
							}
						}

						calculatePitch();
					}

				} else if (channel == 3) {

					if (!pa) {

						if (!velocity) {
							noteHeld2--;
							if (noteHeld2 < 1) {
								noteHeld2 = 0;
								note2 = 0;
								// bitWrite(sid[4],0,0);
								bitWrite(sid[11], 0, 0);
								// bitWrite(sid[18],0,0);
							}
						} else {
							noteHeld2++;
							note2 = note;
							if (noteHeld2 == 1) {
								// bitWrite(sid[4],0,1);
								bitWrite(sid[11], 0, 1);
								// bitWrite(sid[18],0,1);
							}
						}

						calculatePitch();
					}

				} else if (channel == 4) {

					if (!pa) {

						if (!velocity) {

							noteHeld3--;
							if (noteHeld3 < 1) {
								note3 = 0;
								noteHeld3 = 0;
								// bitWrite(sid[4],0,0);
								// bitWrite(sid[11],0,0);
								bitWrite(sid[18], 0, 0);
							}
						} else {

							noteHeld3++;
							note3 = note;

							if (noteHeld3 == 1) {
								//  bitWrite(sid[4],0,1);
								// bitWrite(sid[11],0,1);
								bitWrite(sid[18], 0, 1);
							}
						}

						calculatePitch();
					}
				}
			}
		} else {
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			////////    ONLY LISTENING TO ONE CHANNEL  ////////////////////////////////////////////////
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if (!gate) {

				if (channel == masterChannel) {

					if (!pa) {

						//////////////////////////////////////////////////////////////////////////////
						//                              MONOPHONIC                                  //
						//////////////////////////////////////////////////////////////////////////////
						if (!velocity) {

							held--;

							heldKeys[note] = 0;

							// check highest key

							if ((held) && (!arpMode)) {
								for (int i = 0; i < 128; i++) {
									if (heldKeys[i])
										key = i;
								}
							}

							if (!pedal) {
								// note off

								if (held < 1) {
									held = 0;
									envState = 4;
									arpRound = 0;
									// memset(heldKeys, 0, sizeof(heldKeys));
									bitWrite(sid[4], 0, 0);
									bitWrite(sid[11], 0, 0);
									bitWrite(sid[18], 0, 0);
								}
								// ledNumber(held);
							} else {
							}

						} else {
							velocityLast = velocity;

							// note on;

							heldKeys[note] = 1;

							if (retrig[0]) {
								lfoStep[0] = 0;
							}
							if (retrig[1]) {
								lfoStep[1] = 0;
							}
							if (retrig[2]) {
								lfoStep[2] = 0;
							}
							held++;

							if (!arpMode) {
								key = note;
							}

							if (held == 1) {
								envState = 1;
								env = 0;
								clockCount = arpRate;

								if (arpMode)
									arpReset(note);

								bitWrite(sid[4], 0, 1);
								bitWrite(sid[11], 0, 1);
								bitWrite(sid[18], 0, 1);
							}
						}

					} else {
						//////////////////////////////////////////////////////////////////////////////
						//                              PARAPHONIC                                  //
						//////////////////////////////////////////////////////////////////////////////
						if (!velocity) {
							// note off

							// free a slot

							if (slot[0] == note) {
								slot[0] = 0;
								if (!pedal)
									bitWrite(sid[4], 0, 0);
							} else if (slot[1] == note) {
								slot[1] = 0;
								if (!pedal)
									bitWrite(sid[11], 0, 0);
							} else if (slot[2] == note) {
								slot[2] = 0;
								if (!pedal)
									bitWrite(sid[18], 0, 0);
							}

						} else {
							// note on

							// look for slot

							if (!slot[0]) {
								slot[0] = note;
								bitWrite(sid[4], 0, 1);
								pKey[0] = note;
							} else if (!slot[1]) {
								slot[1] = note;
								bitWrite(sid[11], 0, 1);
								pKey[1] = note;
							} else if (!slot[2]) {
								slot[2] = note;
								bitWrite(sid[18], 0, 1);
								pKey[2] = note;
							}

							// overflow onto voice 3
							else {
								slot[2] = note;
								bitWrite(sid[18], 0, 1);
								pKey[2] = note;
							}
						}
					}
				}
			}
		}
	}
	leftDot();
}

static void pedalUp() {
	if (pa) {

		if (!slot[0])
			bitWrite(sid[4], 0, 0);
		if (!slot[1])
			bitWrite(sid[11], 0, 0);
		if (!slot[2])
			bitWrite(sid[18], 0, 0);

	} else {
		// note off

		if (held < 1) {
			held = 0;
			envState = 4;
			// arpRound=0;
			// memset(heldKeys, 0, sizeof(heldKeys));
			bitWrite(sid[4], 0, 0);
			bitWrite(sid[11], 0, 0);
			bitWrite(sid[18], 0, 0);
		}
	}
}

static void pedalDown() {}

static void HandleControlChange(byte channel, byte data1, byte data2) {
	leftDot();
	if (channel == masterChannel) {
		switch (data1) {

			case 1:
				movedPot(12, data2 << 3, 1);
				break;

			case 2:
				movedPot(4, data2 << 3, 1);
				break;
			case 3:
				movedPot(6, data2 << 3, 1);
				break;
			case 4:
				movedPot(14, data2 << 3, 1);
				break;
			case 5:
				movedPot(1, data2 << 3, 1);
				break;

			case 6:
				movedPot(5, data2 << 3, 1);
				break;
			case 7:
				movedPot(15, data2 << 3, 1);
				break;
			case 8:
				movedPot(13, data2 << 3, 1);
				break;
			case 9:
				movedPot(16, data2 << 3, 1);
				break;

			case 10:
				movedPot(24, data2 << 3, 1);
				break;
			case 11:
				movedPot(26, data2 << 3, 1);
				break;
			case 12:
				movedPot(17, data2 << 3, 1);
				break;
			case 13:
				movedPot(27, data2 << 3, 1);
				break;

			case 14:
				movedPot(22, data2 << 3, 1);
				break;
			case 15:
				movedPot(25, data2 << 3, 1);
				break;
			case 16:
				movedPot(23, data2 << 3, 1);
				break;
			case 17:
				movedPot(20, data2 << 3, 1);
				break;

			case 18:
				movedPot(30, data2 << 3, 1);
				break;
			case 19:
				movedPot(21, data2 << 3, 1);
				break;
			case 20:
				movedPot(31, data2 << 3, 1);
				break;
			case 21:
				movedPot(19, data2 << 3, 1);
				break;

			case 22:
				movedPot(29, data2 << 3, 1);
				break;
			case 23:
				movedPot(18, data2 << 3, 1);
				break;
			case 24:
				movedPot(28, data2 << 3, 1);
				break;
			case 25:
				movedPot(3, data2 << 3, 1);
				break;

			case 26:
				movedPot(11, data2 << 3, 1);
				break;
			case 27:
				movedPot(12, data2 << 3, 1);
				break;
			case 28:
				movedPot(10, data2 << 3, 1);
				break;
			case 29:
				movedPot(9, data2 << 3, 1);
				break;

			case 30:
				movedPot(36, data2 << 3, 1);
				break;
			case 31:
				movedPot(2, data2 << 3, 1);
				break;
			case 59:
			case 32:
				movedPot(8, data2 << 3, 1);
				break;
			case 33:
				movedPot(0, data2 << 3, 1);
				break;

			case 34:
				movedPot(7, data2 << 3, 1);
				break;
			case 35:
				movedPot(41, data2 << 3, 1);
				break;
			case 36:
				movedPot(32, data2 << 3, 1);
				break;

			case 37:
				sidShape(0, 1, data2);
				break;
			case 38:
				sidShape(0, 2, data2);
				break;
			case 39:
				sidShape(0, 3, data2);
				break;
			case 40:
				sidShape(0, 4, data2);
				break;

			case 41:
				sidShape(1, 1, data2);
				break;
			case 42:
				sidShape(1, 2, data2);
				break;
			case 43:
				sidShape(1, 3, data2);
				break;
			case 44:
				sidShape(1, 4, data2);
				break;

			case 45:
				sidShape(2, 1, data2);
				break;
			case 46:
				sidShape(2, 2, data2);
				break;
			case 47:
				sidShape(2, 3, data2);
				break;
			case 48:
				sidShape(2, 4, data2);
				break;

			case 49:
				bitWrite(sid[4], 1, data2);
				ledSet(16, bitRead(sid[4], 1));
				break; // SYNC 1
			case 50:
				bitWrite(sid[4], 2, data2);
				ledSet(17, bitRead(sid[4], 2));
				break; // RING 1

			case 51:
				bitWrite(sid[11], 1, data2);
				ledSet(18, bitRead(sid[11], 1));
				break; // SYNC 2
			case 52:
				bitWrite(sid[11], 2, data2);
				ledSet(19, bitRead(sid[11], 2));
				break; // RING 2

			case 53:
				bitWrite(sid[18], 1, data2);
				ledSet(20, bitRead(sid[18], 1));
				break; // SYNC 3
			case 54:
				bitWrite(sid[18], 2, data2);
				ledSet(21, bitRead(sid[18], 2));
				break; // RING 3

			case 55:
				filterMode = map(data2, 0, 127, 0, 4);
				updateFilter();
				break;

			case 64:
				pedal = data2 >> 6;
				if (pedal) {
					pedalDown();
				} else {
					pedalUp();
				}
				break;

			case 60:
				if (data2) {
					lfoShape[selectedLfo] = 1;
				} else {
					lfoShape[selectedLfo] = 0;
				}
				showLfo();
				sendControlChange(61, 0);
				sendControlChange(62, 0);
				sendControlChange(63, 0);
				sendControlChange(65, 0);
				break; // lfo shape1
			case 61:
				if (data2) {
					lfoShape[selectedLfo] = 2;
				} else {
					lfoShape[selectedLfo] = 0;
				}
				showLfo();
				sendControlChange(60, 0);
				sendControlChange(62, 0);
				sendControlChange(63, 0);
				sendControlChange(65, 0);
				break; // lfo shape2
			case 62:
				if (data2) {
					lfoShape[selectedLfo] = 3;
				} else {
					lfoShape[selectedLfo] = 0;
				}
				showLfo();
				sendControlChange(61, 0);
				sendControlChange(60, 0);
				sendControlChange(63, 0);
				sendControlChange(65, 0);
				break; // lfo shape3
			case 63:
				if (data2) {
					lfoShape[selectedLfo] = 4;
				} else {
					lfoShape[selectedLfo] = 0;
				}
				showLfo();
				sendControlChange(61, 0);
				sendControlChange(62, 0);
				sendControlChange(60, 0);
				sendControlChange(65, 0);
				break; // lfo shape4
			case 65:
				if (data2) {
					lfoShape[selectedLfo] = 5;
				} else {
					lfoShape[selectedLfo] = 0;
				}
				showLfo();
				sendControlChange(61, 0);
				sendControlChange(62, 0);
				sendControlChange(63, 0);
				sendControlChange(60, 0);
				break; // lfo shape5

			case 66:
				if (data2) {
					retrig[selectedLfo] = 1;
				} else {
					retrig[selectedLfo] = 0;
				}
				showLfo();
				break; // lfo retrig
			case 67:
				if (data2) {
					looping[selectedLfo] = 1;
				} else {
					looping[selectedLfo] = 0;
				}
				showLfo();
				break; // lfo loop

			case 68:
				if (data2) {
					sendLfo = true;
					EEPROM.update(3996, 0);
				} else {
					sendLfo = false;
					EEPROM.update(3996, 1);
				}
				break; // lfo send
			case 69:
				if (data2) {
					sendArp = true;
					EEPROM.update(3995, 0);
				} else {
					sendArp = false;
					EEPROM.update(3995, 1);
				}
				break; // arp send
		}
	}
}

static void handleBend(byte channel, int value) {
	//-8192 to 8191
	if (masterChannel == 1) {

		if (channel == 1) {
			bend = value - 64;
			bend = bend / 64;

			if (bend > 1) {
				bend = 1;
			} else if (bend < -1) {
				bend = -1;
			}
			calculatePitch();
		}

		else if (channel == 2) {
			bend1 = value - 64;
			bend1 = bend1 / 64;

			if (bend1 > 1) {
				bend1 = 1;
			} else if (bend1 < -1) {
				bend1 = -1;
			}
			calculatePitch();
		}

		else if (channel == 3) {
			bend2 = value - 64;
			bend2 = bend2 / 64;

			if (bend2 > 1) {
				bend2 = 1;
			} else if (bend2 < -1) {
				bend2 = -1;
			}
			calculatePitch();
		}

		else if (channel == 4) {
			bend3 = value - 64;
			bend3 = bend3 / 64;

			if (bend3 > 1) {
				bend3 = 1;
			} else if (bend3 < -1) {
				bend3 = -1;
			}
			calculatePitch();
		}

	} else {

		if (channel == masterChannel) {
			bend = value - 64;
			bend = bend / 64;

			if (bend > 1) {
				bend = 1;
			} else if (bend < -1) {
				bend = -1;
			}
			calculatePitch();
		}
	}
}

void sendMidiButt(byte number, int value) {
	rightDot();

	sendControlChange(number, value);
}

void sendCC(byte number, int value) {
	rightDot();
	sendControlChange(number, value >> 3);
}

void sendControlChange(byte number, byte value) {
	if (!thru) {

		Serial.write(175 + masterChannelOut);
		Serial.write(number);
		Serial.write(value);
	}
}

void sendNoteOff(byte note, byte velocity, byte channel) {
	if (!thru) {
		Serial.write(127 + channel);
		Serial.write(note);
		Serial.write(1);
	}
}

void sendNoteOn(byte note, byte velocity, byte channel) {

	if (!thru) {
		Serial.write(143 + channel);
		Serial.write(note);
		Serial.write(1);
	}
}

void midiRead() {
	while (Serial.available()) {
		byte input = Serial.read();

		if (thru)
			Serial.write(input);

		if (input > 127) {

			switch (input) {

				case 248:

					sync = 1;
					if ((arpMode) && (arping)) {
						clockCount++;
						if (clockCount >= arpRate) {
							clockCount = 0;
							arpTick();
						}
					}

					syncLfoCounter++;
					if (syncLfoCounter == 24) {
						syncLfoCounter = 0;
						for (int i = 0; i < 3; i++) {
							if (lfoClockSpeedPending[i]) {
								lfoClockSpeed[i] = lfoClockSpeedPending[i] - 1;
								lfoClockSpeedPending[i] = 0;
								lfoStepF[i] = 0;
							}
						}
					}

					for (int i = 0; i < 3; i++) {
						lfoStepF[i] += lfoClockRates[lfoClockSpeed[i]];
						if (lfoStepF[i] > 254) {
							if (looping[i]) {
								lfoStepF[i] = 0;
								lfoNewRand[i] = 1;
							} else {
								lfoStepF[i] = 255;
							}
						}
						lfoStep[i] = int(lfoStepF[i]);
					}

					break; // clock
				case 250:
					syncLfoCounter = 0;
					sync = 1;
					lfoStepF[0] = lfoStepF[1] = lfoStepF[2] = 0;
					break; // start
				case 251:
					break; // continue
				case 252:
					sync = 0;
					break; // stop

				case 128 ... 143:
					mChannel = input - 127;
					mStatus = 2;
					mData = 255;
					break; // noteOff
				case 144 ... 159:
					mChannel = input - 143;
					mStatus = 1;
					mData = 255;
					break; // noteOn
				case 176 ... 191:
					mChannel = input - 175;
					mStatus = 3;
					mData = 255;
					break; // CC
				case 192 ... 207:
					mChannel = input - 191;
					mStatus = 6;
					mData = 0;
					break; // program Change
				case 208 ... 223:
					mChannel = input - 207;
					mStatus = 5;
					mData = 0;
					break; // Aftertouch
				case 224 ... 239:
					mChannel = input - 223;
					mStatus = 4;
					mData = 255;
					break; // Pitch Bend

				default:
					mStatus = 0;
					mData = 255;
					break;
			}
		}

		// status
		else {
			if (mData == 255) {
				mData = input;
			} // data byte 1
			else {

				// data byte 2
				switch (mStatus) {
					case 1:
						if (input) {
							HandleNoteOn(mChannel, mData, input);
						} else {
							HandleNoteOn(mChannel, mData, 0);
						}
						mData = 255;
						break; // noteOn
					case 2:
						HandleNoteOn(mChannel, mData, 0);
						mData = 255;
						break; // noteOff
					case 3:
						HandleControlChange(mChannel, mData, input);
						mData = 255;
						break; // CC
					case 4:
						handleBend(mChannel, input);
						mData = 255;
						break; // bend
					case 5:
						lfoDepthBase[1] = input << 3;
						setLfo(1);
						mData = 255;
						break; // AT
					case 6:
						if (mChannel == masterChannel) {
							preset = mData + 1;
							if (preset > 99) {
								preset = 1;
							}
							ledNumber(preset);
						}
						mData = 255;
						break; // PC
				}
			} // data
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////

void midiOut(byte note) {
	rightDot();
	sendNoteOff(lastNote + 1, 127, masterChannelOut);
	sendNoteOn(note + 1, velocityLast, masterChannelOut);
	lastNote = note;
}

void sendDump() {

	digit(0, 5);
	digit(1, 18);

	byte mem[4000];
	for (int i = 0; i < 4000; i++) {
		mem[i] = EEPROM.read(i);
	}
	byte nill = 0;

	Serial.write(240);
	delay(1);
	Serial.write(100);
	delay(1);

	for (int i = 0; i < 4000; i++) {

		if (mem[i] > 127) {
			Serial.write(mem[i] - 128);
			delay(1);
			Serial.write(1);
			delay(1);
		} else {
			Serial.write(mem[i]);
			delay(1);
			Serial.write(nill);
			delay(1);
		}
	}
	Serial.write(247);
	Serial.flush();
}

void recieveDump() {
	byte mem[4000];
	digit(0, 16);
	digit(1, 18);

	Timer1.stop();

	while (dump != 247) {

		if (Serial.available() > 0) {

			dump = Serial.read();

			if (dumpCounter == 0) {
				if (dump == 240) {
					dumpCounter++;
				} else {
					dumpCounter = 247;
				}
			} // must be sysex or cancel
			else if (dumpCounter == 1) {
				if (dump == 100) {
					dumpCounter++;
				} else {
					dumpCounter = 247;
				}
			} // must be 100 manuCode or cancel

			else {

				alternator = !alternator;

				if (alternator) {
					flash = dump;
				} else {
					flash += val[dump];
					//
					mem[dumpCounter - 2] = flash;
					dumpCounter++;
				}
			}
		}
	}

	byte ledLast = 0;
	for (int i = 0; i < 4000; i++) {

		if ((i != 3998) && (i != 3997)) { // don't overWrite MIDI channels!!
			EEPROM.update(i, mem[i]);
			if (ledLast != i / 40) {
				ledLast = i / 40;
				ledNumber(ledLast);
			}
		}
	}
	digit(0, 99);
	digit(1, 99);

	Timer1.resume();

	setup();
}
