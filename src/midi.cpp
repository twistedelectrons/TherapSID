#include <EEPROM.h>
#include <TimerOne.h>
#include "version.h"
#include "globals.h"
#include "ui_leds.h"
#include "midi.h"
#include "arp.h"
#include "ui_pots.h"
#include "voice_state.hpp"
#include "midi_pedal.hpp"
#include "asid.h"

static byte mStatus;
static byte mData;
static byte mChannel;

static byte syncLfoCounter;
static float lfoClockRates[] = {2.6562, 5.3125, 7.96875, 10.625, 21.25, 31.875, 42.5, 85};

static float lfoStepF[3];
static byte lfoClockSpeed[3];
static int clockCount;
static bool thru;

static int dumpCounter = 0;
static byte dump;
static bool alternator;
static byte flash;
static byte val[] = {0, 128};

static void PrepareHandleNoteOn(byte channel, byte note, byte velocity);
static void PrepareHandleNoteOff(byte channel, byte note);
static void HandleNoteOn(byte channel, byte note, byte velocity);
static void HandleNoteOff(byte channel, byte note);

static MidiPedalAdapter pedal_adapter(PrepareHandleNoteOn, PrepareHandleNoteOff);

typedef enum { IDLE, RECEIVING, WAITING_FOR_END } SysexState;

typedef struct {
	SysexState state;
	byte buffer[256];
	unsigned int index;
} SysexMachine;

static SysexMachine sysexMachine;

static void PrepareHandleNoteOn(byte channel, byte note, byte velocity) {
	heldKeys[note] = 1;
	if (autoChord) {
		for (int i = 0; i < 128; i++) {
			if (chordKeys[i])
				HandleNoteOn(channel, note + i - chordRoot, velocity);
		}
	} else {

		HandleNoteOn(channel, note, velocity);
	}
}

static void PrepareHandleNoteOff(byte channel, byte note) {
	heldKeys[note] = 0;
	if (autoChord) {
		for (int i = 0; i < 128; i++) {
			if (chordKeys[i])
				HandleNoteOff(channel, note + i - chordRoot);
		}
		if (clearAutoChord) {
			// silence all notes just in case
			for (int i = 0; i < 128; i++) {
				HandleNoteOff(channel, i);
			}

			autoChord = clearAutoChord = false;
		}
	} else {

		HandleNoteOff(channel, note);
	}
}

static void HandleNoteOn(byte channel, byte note, byte velocity) {
	if (note < 12 || note >= 119)
		return;

	note -= 12;

	if (velocity) {
		if (channel == masterChannel) {
			// retrigger all LFO
			for (int i = 0; i < 3; i++) {
				if (preset_data.lfo[i].retrig) {
					lfoStep[i] = lfoStepF[i] = 0;
				}
			}
		} else if (channel == voice1Channel) {
			// only retrigger LFO if it is chained to voice 1
			for (int i = 0; i < 3; i++) {
				if (preset_data.lfo[i].mapping[0] || preset_data.lfo[i].mapping[1] || preset_data.lfo[i].mapping[2]) {
					if (preset_data.lfo[i].retrig) {
						lfoStep[i] = lfoStepF[i] = 0;
					}
				}
			}
		}

		else if (channel == voice2Channel) {
			// only retrigger LFO if it is chained to voice 2
			for (int i = 0; i < 3; i++) {
				if (preset_data.lfo[i].mapping[3] || preset_data.lfo[i].mapping[4] || preset_data.lfo[i].mapping[5]) {
					if (preset_data.lfo[i].retrig) {
						lfoStep[i] = lfoStepF[i] = 0;
					}
				}
			}
		}

		else if (channel == voice3Channel) {
			// only retrigger LFO if it is chained to voice 3
			for (int i = 0; i < 3; i++) {
				if (preset_data.lfo[i].mapping[6] || preset_data.lfo[i].mapping[7] || preset_data.lfo[i].mapping[8]) {
					if (preset_data.lfo[i].retrig) {
						lfoStep[i] = lfoStepF[i] = 0;
					}
				}
			}
		}
	}

	if (channel == masterChannel) {
		if (velocity) {
			velocityLast = velocity;
			if (voice_state.n_held_keys() < 1) {
				arp_output_note = note + (arpRound * 12);
			}
		}

		switch (voice_state.note_on(note, velocity)) {
			case VoiceStateEvent::LAST_NOTE_OFF:
				envState = 4;
				arpRound = 0;
				break;
			case VoiceStateEvent::FIRST_NOTE_ON:
				envState = 1;
				env = 0;
				clockCount = preset_data.arp_rate;

				if (preset_data.arp_mode)
					arpReset();
				break;
			default:
				break;
		}
	} else if (!preset_data.is_polyphonic() &&
	           (channel == voice1Channel || channel == voice2Channel || channel == voice3Channel)) {
		auto voice = 0;

		if (channel == voice2Channel) {
			voice = 1;
		} else if (channel == voice3Channel) {
			voice = 2;
		}

		if (velocity) {
			voice_state.note_on_individual(voice, note);
			voice_state.note_on_individual(voice + 3, note);
		} else {
			voice_state.note_off_individual(voice, note);
			voice_state.note_off_individual(voice + 3, note);
		}
	}
	leftDot();
}

static void HandleNoteOff(byte channel, byte note) { HandleNoteOn(channel, note, 0); }

static byte lastData1, lastData2; // keep track of last CC and Value for handshake with tool.

static void HandleControlChange(byte channel, byte data1, byte data2) {
	leftDot();

	if (channel == 16 && lastData1 == 19 && lastData2 == 82 && data1 == 19 && data2 == 82) {
		// Transmit device ID & Firmware version
		sendControlChange(1, 2, 16); // TOOL expects 2 for TherapSID
		sendControlChange(2, version, 16);
		sendControlChange(3, versionDecimal, 16);
		// Transmit all the settings to tool. Tool expects CC messages on CH16
		for (int i = 0; i < (int)(sizeof(globalSettings) / sizeof(*globalSettings)); i++) {
			const globalSetting* setting = &(globalSettings[i]);

			if (setting->ccMessageToolMode < 128) // don't want to send last preset (CC#255)
				sendControlChange(setting->ccMessageToolMode, *(byte*)setting->variable, 16);
		}

		toolMode = true; // therapSid is listening to new settings (CC on CH16)
	}
	// did we receive 1982 twice on channel 16?

	lastData1 = data1;
	lastData2 = data2;
	if (channel == 16 && toolMode) {
		// Go through all global settings and check if the tool requested to change one
		for (int i = 0; i < (int)(sizeof(globalSettings) / sizeof(*globalSettings)); i++) {
			const globalSetting* setting = &(globalSettings[i]);

			if (setting->ccMessageToolMode == data1) {

				if (setting->isBaseOne) {
					// Adjust for internal representation
					data2++;
				}

				// Validate range and update only if valid
				if ((data2 >= setting->minValue) && (data2 <= setting->maxValue)) {
					*(byte*)(setting->variable) = data2;
					EEPROM.update(setting->eepromAddress, data2);
				}
			}
		}

		volumeChanged = true;
	} else if (channel == masterChannel || channel == voice1Channel || channel == voice2Channel ||
	           channel == voice3Channel) {
		if (data1 == 59)
			data1 = 32;

		if (data1 == 1) {
			modWheelLast = data2;
		}

		if (1 <= data1 && data1 <= 36) {
			int mapping[] = {-1, -1, 4,  6,  14, 1,  5, 15, 13, 16, 24, 26, 17, 27, 22, 25, 23, 20, 30,
			                 21, 31, 19, 29, 18, 28, 3, 11, 12, 10, 9,  36, 2,  8,  0,  7,  41, 32};
			movedPot(mapping[data1], data2 << 3, true);
		} else if (37 <= data1 && data1 <= 48) {
			int offset = data1 - 37;
			static const PresetVoice::Shape mapping[] = {PresetVoice::PULSE, PresetVoice::TRI, PresetVoice::SAW,
			                                             PresetVoice::NOISE};
			preset_data.voice[offset / 4].set_shape(mapping[offset % 4], data2);
		} else {
			switch (data1) {
				case 49: // sync1
					bitWrite(preset_data.voice[0].reg_control, 1, data2);
					break;
				case 50: // ring1
					bitWrite(preset_data.voice[0].reg_control, 2, data2);
					break;

				case 51: // sync2
					bitWrite(preset_data.voice[1].reg_control, 1, data2);
					break;
				case 52: // ring2
					bitWrite(preset_data.voice[1].reg_control, 2, data2);
					break;

				case 53: // sync3
					bitWrite(preset_data.voice[2].reg_control, 1, data2);
					break;
				case 54: // ring3
					bitWrite(preset_data.voice[2].reg_control, 2, data2);
					break;

				case 55:
					preset_data.filter_mode = static_cast<FilterMode>(map(data2, 0, 127, 0, 4));
					break;

				case 64:
					if (channel == voice1Channel || channel == voice2Channel || channel == voice3Channel) {
						pedal_adapter.set_pedal(channel, data2 >= 64);
					} else {
						for (int c = 0; c < 16; c++) {
							pedal_adapter.set_pedal(c, data2 >= 64); // it's the master channel - send pedal to all
						}
					}
					break;
			}
		}
	}
}

static void handleBend(byte channel, int value) {
	// 0 to 127 => -1.0 to 1.0
	float value_f = (value - 64.f) / 63.f;
	if (value_f > 1)
		value_f = 1;
	if (value_f < -1)
		value_f = -1;

	if (value_f > 0) {
		value_f *= pitchBendUp;
	} else {
		value_f *= pitchBendDown;
	} // remap to bend ranges

	if (channel == masterChannel) {
		bend = value_f;
	} else if (channel == voice1Channel) {
		bend1 = value_f;
	} else if (channel == voice2Channel) {
		bend2 = value_f;
	} else if (channel == voice3Channel) {
		bend3 = value_f;
	}
}

void sendMidiButt(byte number, int value) {
	rightDot();
	sendControlChange(number, !!value, masterChannelOut);
}

void sendCC(byte number, int value) {
	rightDot();
	sendControlChange(number, value >> 3, masterChannelOut);
}

void sendControlChange(byte number, byte value, byte channel) {
	if (!thru) {
		Serial.write(175 + channel);
		Serial.write(number);
		Serial.write(value);
	}
}

void sendNoteOff(byte note, byte velocity, byte channel) {
	if (!thru) {
		Serial.write(127 + channel);
		Serial.write(note);
		Serial.write(velocity);
	}
}

void sendNoteOn(byte note, byte velocity, byte channel) {
	if (!thru) {
		Serial.write(143 + channel);
		Serial.write(note);
		Serial.write(velocity);
	}
}

// Handles a completed sysex buffer
void processSysex(byte* buffer, int size) {
	if (size > 1) {

		if (buffer[0] == 45) {
			// Manufacturer ID used for ASID (DH's favourite number)
			asidProcessMessage(buffer, size);
		} else {
			// Room for other sysex messages
		}
	}
}

int testCounter;
byte testNote;
const byte cBlues[] = {60, 63, 65, 66, 67, 70, 72, 75, 77, 78, 79, 82, 84};

void midiRead() {

	if (demoMode) {
		testCounter++;
		if (stopdemoMode == true) {
			pedal_adapter.note_off(1, testNote);
				stopdemoMode = demoMode = false;
			}

		if (testCounter == 70) {

			pedal_adapter.note_off(1, testNote);

		}

		else if (testCounter == 37) {

			if (random(10) < 3) {
				pedal_adapter.note_off(1, testNote);
				testNote += 12;
				pedal_adapter.note_on(1, testNote, 64);
			}

		} else if (testCounter == 75) {

			testNote = cBlues[random(13)] + 12 * random(3);
			testNote -= 27;
			pedal_adapter.note_on(1, testNote, 64);
		}

		else if (testCounter == 95) {
			pedal_adapter.note_off(1, testNote);
			
		}

		else if (testCounter == 100) {
			testCounter = 0;

			testNote = cBlues[random(13)] + 12 * random(4);

			testNote -= 27;
			pedal_adapter.note_on(1, testNote, 64);
		}
	}
	while (Serial.available()) {
		byte input = Serial.read();

		if (thru)
			Serial.write(input);

		if (input & 0x80) {

			switch (input) {
				case 0xf0: // sysex start
					sysexMachine.state = RECEIVING;
					sysexMachine.index = 0;
					break;

				case 0xf7: // sysex end
					processSysex(sysexMachine.buffer, sysexMachine.index);
					sysexMachine.index = 0;
					sysexMachine.state = IDLE;
					break;

				case 0xf8: // clock

					sync = 1;
					if ((preset_data.arp_mode) && arping()) {
						clockCount++;
						if (clockCount >= preset_data.arp_rate) {
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
							if (preset_data.lfo[i].looping) {
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
			if (sysexMachine.state == RECEIVING) {
				if (sysexMachine.index < sizeof(sysexMachine.buffer) / sizeof(*sysexMachine.buffer)) {
					// Add sysex data to the buffer
					sysexMachine.buffer[sysexMachine.index++] = input;
				} else {
					// Buffer is full, just wait for this message to be finished
					sysexMachine.state = WAITING_FOR_END;
				}
			} else if (sysexMachine.state == WAITING_FOR_END) {
				// We are getting data even though we are just waiting for sysex end to come
				// So just consume data and do nothing...
			} else if (mData == 255) {
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
						aftertouch = input;
						mData = 255;
						break; // AT
					case 6:
						if (mChannel == masterChannel) {
							preset = input + 1;
							if (preset > PRESET_NUMBER_MAX) {
								preset = PRESET_NUMBER_MIN;
							}
						}
						mData = 0;
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

	digit(0, DIGIT_S);
	digit(1, DIGIT_E);

	byte mem[EEPROM.length()];
	for (size_t i = 0; i < sizeof(mem) / sizeof(*mem); i++) {
		mem[i] = EEPROM.read(i);
	}
	byte nill = 0;

	Serial.write(240);
	delay(1);
	Serial.write(100);
	delay(1);

	for (size_t i = 0; i < sizeof(mem) / sizeof(*mem); i++) {

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
	byte mem[EEPROM.length()];
	digit(0, DIGIT_R);
	digit(1, DIGIT_E);

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
	for (size_t i = 0; i < sizeof(mem) / sizeof(*mem); i++) {

		if ((i != EEPROM_ADDR_MIDI_IN_CH_MASTER) &&
		    (i != EEPROM_ADDR_MIDI_OUT_CH_MASTER)) { // don't overWrite MIDI channels!!
			EEPROM.update(i, mem[i]);
			if (ledLast != i / 40) {
				ledLast = i / 40;
				ledNumber(ledLast);
			}
		}
	}
	digit(0, DIGIT_BLANK);
	digit(1, DIGIT_BLANK);

	Timer1.resume();

	setup();
}
