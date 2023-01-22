#include <EEPROM.h>
#include <TimerOne.h>

#include "globals.h"
#include "leds.h"
#include "midi.h"
#include "arp.h"
#include "sid.h"
#include "lfo.h"
#include "pots.h"
#include "voice_allocation.hpp"
#include "midi_pedal.hpp"

static byte mStatus;
static byte mData;
static byte mChannel;

static byte syncLfoCounter;
static float lfoClockRates[] = {2.6562, 5.3125, 7.96875, 10.625, 21.25, 31.875, 42.5, 85};

static float lfoStepF[3];
static byte lfoClockSpeed[3];
static int clockCount;
static bool thru;
static byte velocityLast;

static int dumpCounter = 0;
static byte dump;
static bool alternator;
static byte flash;

static byte val[] = {0, 128};

static void HandleNoteOn(byte channel, byte note, byte velocity);
static void HandleNoteOff(byte channel, byte note);

static MidiPedalAdapter pedal_adapter(HandleNoteOn, HandleNoteOff);

static void HandleNoteOn(byte channel, byte note, byte velocity) {
	if (gate) // FIXME maybe this should be handled differently
		return;

	if (note < 12 || note >= 107)
		return;

	note -= 12;

	if (channel == masterChannel) {
		if (!pa) { // Monophonic mode
			if (!velocity) {
				// note off
				held--;
				heldKeys[note] = false;

				mono_note_tracker.note_off(note);
				if (mono_note_tracker.has_active_note()) {
					if (!arpMode)
						key = mono_note_tracker.active_note()->note;
				} else {
					envState = 4;
					arpRound = 0;
					bitWrite(sid[4], 0, 0);
					bitWrite(sid[11], 0, 0);
					bitWrite(sid[18], 0, 0);
				}
			} else {
				// note on
				velocityLast = velocity;
				heldKeys[note] = true;
				held++;

				for (int i = 0; i < 3; i++) {
					if (retrig[i]) {
						lfoStep[i] = lfoStepF[i] = 0;
					}
				}

				if (!arpMode) {
					key = note;
				}

				auto had_active_note = mono_note_tracker.has_active_note();
				mono_note_tracker.note_on(note, velocity);

				if (!had_active_note) {
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

		} else {             // paraphonic mode
			if (!velocity) { // note off
				auto voice_idx = voice_allocator.note_off(note);

				if (voice_idx.has_value()) {
					bitWrite(sid[4 + 7 * (*voice_idx)], 0, 0);
				}
			} else { // note on
				auto voice_idx = voice_allocator.note_on(note, velocity);

				bitWrite(sid[4 + 7 * voice_idx], 0, 1);
				pKey[voice_idx] = note;
			}
		}
	} else if (!pa && masterChannel == 1 && (channel == 2 || channel == 3 || channel == 4)) {
		auto voice = channel - 2;

		bool had_active_note = mono_note_trackers[voice].has_active_note();
		if (!velocity) { // note off
			mono_note_trackers[voice].note_off(note);
		} else {
			mono_note_trackers[voice].note_on(note, velocity);
		}

		if (mono_note_trackers[voice].has_active_note()) {
			note_val[voice] = mono_note_trackers[voice].active_note()->note;
			if (!had_active_note) {
				bitWrite(sid[4 + voice * 7], 0, 1);
			}
		} else {
			note_val[voice] = 0;
			bitWrite(sid[4 + voice * 7], 0, 0);
		}

		calculatePitch();
	}
	leftDot();
}

static void HandleNoteOff(byte channel, byte note) { HandleNoteOn(channel, note, 0); }

static void HandleControlChange(byte channel, byte data1, byte data2) {
	leftDot();
	if (channel == masterChannel) {
		if (data1 == 59)
			data1 = 32; // FIXME why?

		if (1 <= data1 && data1 <= 36) {
			int mapping[] = {-1, 12, 4,  6,  14, 1,  5, 15, 13, 16, 24, 26, 17, 27, 22, 25, 23, 20, 30,
			                 21, 31, 19, 29, 18, 28, 3, 11, 12, 10, 9,  36, 2,  8,  0,  7,  41, 32};
			movedPot(mapping[data1], data2 << 3, true);
		} else if (37 <= data1 && data1 <= 48) {
			int offset = data1 - 37;
			sidShape(offset / 4, offset % 4 + 1, data2);
		} else {
			switch (data1) {
				case 49: // sync1
					bitWrite(sid[4], 1, data2);
					ledSet(16, bitRead(sid[4], 1));
					break;
				case 50: // ring1
					bitWrite(sid[4], 2, data2);
					ledSet(17, bitRead(sid[4], 2));
					break;

				case 51: // sync2
					bitWrite(sid[11], 1, data2);
					ledSet(18, bitRead(sid[11], 1));
					break;
				case 52: // ring2
					bitWrite(sid[11], 2, data2);
					ledSet(19, bitRead(sid[11], 2));
					break;

				case 53: // sync3
					bitWrite(sid[18], 1, data2);
					ledSet(20, bitRead(sid[18], 1));
					break;
				case 54: // ring3
					bitWrite(sid[18], 2, data2);
					ledSet(21, bitRead(sid[18], 2));
					break;

				case 55:
					filterMode = static_cast<FilterMode>(map(data2, 0, 127, 0, 4));
					updateFilter();
					break;

				case 64:
					pedal_adapter.set_pedal(channel, data2 >= 64);
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
}

static void handleBend(byte channel, int value) {
	//-8192 to 8191
	float value_f = (value - 64.f) / 64.f;
	if (value_f > 1)
		value_f = 1;
	if (value_f < -1)
		value_f = -1;

	if (masterChannel == 1) {
		if (channel == 1)
			bend = value_f;
		else if (channel == 2)
			bend1 = value_f;
		else if (channel == 3)
			bend2 = value_f;
		else if (channel == 4)
			bend3 = value_f;
		calculatePitch();
	} else {
		if (channel == masterChannel) {
			bend = value_f;
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

		if (input & 0x80) {

			switch (input) {

				case 0xf8: // clock

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

					break;
				case 0xfa: // start
					syncLfoCounter = 0;
					sync = 1;
					lfoStepF[0] = lfoStepF[1] = lfoStepF[2] = 0;
					break;
				case 0xfb: // continue
					break;
				case 0xfc: // stop
					sync = 0;
					break;

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
						pedal_adapter.note_on(mChannel, mData, input);
						mData = 255;
						break; // noteOn
					case 2:
						pedal_adapter.note_off(mChannel, mData);
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
