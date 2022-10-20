#include "globals.h"
#include "leds.h"
#include "preset.h"
#include "sid.h"
#include "paraphonic.h"
#include "midi.h"
#include "lfo.h"
#include "arp.h"

void buttChanged(byte number, bool value) {
	if (!value) {
		// ledNumber(number);
		//  PRESSED
		switch (number) {

			case 2:
				if (!filterModeHeld) {
					sidShape(0, 1, !bitRead(sid[4], 6));
					shape1Pressed = true;
					paraShape();
					sendMidiButt(37, bitRead(sid[4], 6));
				} else {
					filterEnabled[0] = !filterEnabled[0];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(0);
				break; // RECT 1
			case 12:
				if (!filterModeHeld) {
					sidShape(0, 2, !bitRead(sid[4], 4));
					shape1Pressed = true;
					paraShape();
					sendMidiButt(38, bitRead(sid[4], 4));
				} else {
					filterEnabled[0] = !filterEnabled[0];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(0);
				break; // TRI 1
			case 4:
				if (!filterModeHeld) {
					sidShape(0, 3, !bitRead(sid[4], 5));
					shape1Pressed = true;
					paraShape();
					sendMidiButt(39, bitRead(sid[4], 5));
				} else {
					filterEnabled[0] = !filterEnabled[0];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(0);
				break; // SAW 1
			case 8:
				if (!filterModeHeld) {
					sidShape(0, 4, !bitRead(sid[4], 7));
					shape1Pressed = true;
					paraShape();
					sendMidiButt(40, bitRead(sid[4], 7));
				} else {
					filterEnabled[0] = !filterEnabled[0];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(0);
				break; // NOISE 1

			case 0:
				if (!filterModeHeld) {
					sidShape(1, 1, !bitRead(sid[11], 6));
					sendMidiButt(41, bitRead(sid[11], 6));
				} else {
					filterEnabled[1] = !filterEnabled[1];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(1);
				break; // RECT 2
			case 10:
				if (!filterModeHeld) {
					sidShape(1, 2, !bitRead(sid[11], 4));
					sendMidiButt(42, bitRead(sid[11], 4));
				} else {
					filterEnabled[1] = !filterEnabled[1];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(1);
				break; // TRI 2
			case 6:
				if (!filterModeHeld) {
					sidShape(1, 3, !bitRead(sid[11], 5));
					sendMidiButt(43, bitRead(sid[11], 5));
				} else {
					filterEnabled[1] = !filterEnabled[1];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(1);
				break; // SAW 2
			case 14:
				if (!filterModeHeld) {
					sidShape(1, 4, !bitRead(sid[11], 7));
					sendMidiButt(44, bitRead(sid[11], 7));
				} else {
					filterEnabled[1] = !filterEnabled[1];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(1);
				break; // NOISE 2

			case 1:
				if (!filterModeHeld) {
					sidShape(2, 1, !bitRead(sid[18], 6));
					sendMidiButt(45, bitRead(sid[18], 6));
				} else {
					filterEnabled[2] = !filterEnabled[2];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(2);
				break; // RECT 3
			case 9:
				if (!filterModeHeld) {
					sidShape(2, 2, !bitRead(sid[18], 4));
					sendMidiButt(46, bitRead(sid[18], 4));
				} else {
					filterEnabled[2] = !filterEnabled[2];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(2);
				break; // TRI 3
			case 5:
				if (!filterModeHeld) {
					sidShape(2, 3, !bitRead(sid[18], 5));
					sendMidiButt(47, bitRead(sid[18], 5));
				} else {
					filterEnabled[2] = !filterEnabled[2];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(2);
				break; // SAW 3
			case 13:
				if (!filterModeHeld) {
					sidShape(2, 4, !bitRead(sid[18], 7));
					sendMidiButt(48, bitRead(sid[18], 7));
				} else {
					filterEnabled[2] = !filterEnabled[2];
					showFilterAssigns();
					assignmentChanged = true;
				}
				setFilterBit(2);
				break; // NOISE 3

			case 11:
				bitWrite(sid[4], 1, !bitRead(sid[4], 1));
				ledSet(16, bitRead(sid[4], 1));
				sendMidiButt(49, bitRead(sid[4], 1));
				break; // SYNC 1
			case 3:
				bitWrite(sid[4], 2, !bitRead(sid[4], 2));
				ledSet(17, bitRead(sid[4], 2));
				sendMidiButt(50, bitRead(sid[4], 2));
				break; // RING 1

			case 16:
				bitWrite(sid[11], 1, !bitRead(sid[11], 1));
				ledSet(18, bitRead(sid[11], 1));
				sendMidiButt(51, bitRead(sid[11], 1));
				break; // SYNC 2
			case 20:
				bitWrite(sid[11], 2, !bitRead(sid[11], 2));
				ledSet(19, bitRead(sid[11], 2));
				sendMidiButt(52, bitRead(sid[11], 2));
				break; // RING 2

			case 28:
				bitWrite(sid[18], 1, !bitRead(sid[18], 1));
				ledSet(20, bitRead(sid[18], 1));
				sendMidiButt(53, bitRead(sid[18], 1));
				break; // SYNC 3
			case 24:
				bitWrite(sid[18], 2, !bitRead(sid[18], 2));
				ledSet(21, bitRead(sid[18], 2));
				sendMidiButt(54, bitRead(sid[18], 2));
				break; // RING 3

			case 15:
				lfoButtPressed = true;
				selectedLfo = 0;
				if ((lastPot != 9) && (lastPot != 10)) {
					chain();
				}
				break; // LFO CHAIN 1
			case 27:
				lfoButtPressed = true;
				selectedLfo = 1;
				if ((lastPot != 11) && (lastPot != 12)) {
					chain();
				}
				break; // LFO CHAIN 2
			case 21:
				lfoButtPressed = true;
				selectedLfo = 2;
				if ((lastPot != 13) && (lastPot != 14)) {
					chain();
				}
				break; // LFO CHAIN 3

			case 30:
				if (lfoShape[selectedLfo] != 1) {
					lfoShape[selectedLfo] = 1;
				} else {
					lfoShape[selectedLfo] = 0;
				}
				showLfo();
				break; // LFO RECT
			case 18:
				if (lfoShape[selectedLfo] != 2) {
					lfoShape[selectedLfo] = 2;
				} else {
					lfoShape[selectedLfo] = 0;
				}
				showLfo();
				break; // LFO SAW
			case 26:
				if (lfoShape[selectedLfo] != 3) {
					lfoShape[selectedLfo] = 3;
				} else {
					lfoShape[selectedLfo] = 0;
				}
				showLfo();
				break; // LFO TRI
			case 22:
				if (lfoShape[selectedLfo] != 4) {
					lfoShape[selectedLfo] = 4;
				} else {
					lfoShape[selectedLfo] = 0;
				}
				showLfo();
				break; // LFO NOISE
			case 31:
				if (lfoShape[selectedLfo] != 5) {
					lfoShape[selectedLfo] = 5;
				} else {
					lfoShape[selectedLfo] = 0;
				}
				showLfo();

				break; // LFO ENV3

			case 32:
				arpModeHeld = true;
				if (midiSetup) {
					midiSetup = 3;
					digit(99, 0);
					digit(99, 1);
				}
				break; // ARP MODE

			case 29:
				retrig[selectedLfo] = !retrig[selectedLfo];
				showLfo();
				break; //  RETRIG
			case 17:
				looping[selectedLfo] = !looping[selectedLfo];
				showLfo();
				break; //  LOOP

			case 19:

				if (midiSetup > 0) {
					if ((midiSetup == 1) && (masterChannel < 16)) {
						masterChannel++;
						saveChannels();
						ledNumber(masterChannel);
					}
					if ((midiSetup == 2) && (masterChannelOut < 16)) {
						masterChannelOut++;
						saveChannels();
						ledNumber(masterChannelOut);
					}

				} else {
					if (arpModeHeld) {
						midiSetup = 1;
						ledNumber(masterChannel);
					} else {

						saveModeTimer = 0;
						presetUp = true;
						if (presetDown) {
							if (!saveMode) {
								saveMode = true;
								saveBounce = 1600;
							} else {
								saveBounce = 1600;
								save();
							}
						}
					}
				}
				break; // PRESET UP

			case 25:

				if (midiSetup > 0) {
					if ((midiSetup == 1) && (masterChannel > 1)) {
						masterChannel--;
						saveChannels();
						ledNumber(masterChannel);
					}
					if ((midiSetup == 2) && (masterChannelOut > 1)) {
						masterChannelOut--;
						saveChannels();
						ledNumber(masterChannelOut);
					}
				} else {

					if (arpModeHeld) {
						midiSetup = 2;
						ledNumber(masterChannelOut);
					} else {

						saveModeTimer = 0;
						presetDown = true;
						if (presetUp) {
							if (!saveMode) {
								saveMode = true;
								saveBounce = 1600;
							} else {
								saveBounce = 1600;
								save();
							}
						}
					}
					break; // PRESET DOWN

					case 23:
						resetDown = true;
						load(1);
						resetDownTimer = 0;
						break; // PRESET RESET

					case 7:
						assignmentChanged = false;
						filterModeHeld = true;
						showFilterAssigns();

						break; //  FILTER MODE
				}
		}
	}

	else {

		switch (number) {

			case 2:
				shape1Pressed = false;
				shape1PressedTimer = 0;
				break; // RECT 1
			case 12:
				shape1Pressed = false;
				shape1PressedTimer = 0;
				break; // TRI 1
			case 4:
				shape1Pressed = false;
				shape1PressedTimer = 0;
				break; // SAW 1
			case 8:
				shape1Pressed = false;
				shape1PressedTimer = 0;
				break; // NOISE 1

			case 0:
				break; // RECT 2
			case 10:
				break; // TRI 2
			case 6:
				break; // SAW 2
			case 14:
				break; // NOISE 2

			case 1:
				break; // RECT 3
			case 9:
				break; // TRI 3
			case 5:
				break; // SAW 3
			case 13:
				break; // NOISE 3

			case 11:
				break; // SYNC 1
			case 3:
				break; // RING 1

			case 16:
				break; // SYNC 2
			case 20:
				break; // RING 2

			case 28:
				break; // SYNC 3
			case 24:
				break; // RING 3

			case 15:
				lfoButtPressed = false;
				lfoButtTimer = 0;
				break; // LFO CHAIN 1
			case 27:
				lfoButtPressed = false;
				lfoButtTimer = 0;
				break; // LFO CHAIN 2
			case 21:
				lfoButtPressed = false;
				lfoButtTimer = 0;
				break; // LFO CHAIN 3

			case 30:
				break; // LFO RECT
			case 18:
				break; // LFO SAW
			case 26:
				break; // LFO TRI
			case 22:
				break; // LFO NOISE
			case 31:
				break; // LFO ENV3

			case 32:
				arpModeHeld = false;
				arpModeCounter = 0;
				if (!midiSetup) {
					if (!pa) {
						arpMode++;
						if (arpMode > 4) {
							arpMode = 0;
							sendNoteOff(lastNote, 127, masterChannelOut);
						}
						showArp();
					}
				} else if (midiSetup == 3) {
					midiSetup = 0;
					digit(0, 99);
					digit(1, 99);
				}

				break; // ARP MODE
			case 29:
				break; //  RETRIG
			case 17:
				break; //  LOOP

			case 19:

				if (!midiSetup) {
					presetUp = false;
					presetScrollSpeed = 10000;
					if ((!saveBounce) && (!loadTimer) && (!scrolled)) {
						preset++;
						if (preset > 99) {
							preset = 1;
						}
						ledNumber(preset);
					} else {
						saveEngaged = false;
					}
					saveModeTimer = 0;
				}
				scrolled = false;
				break; // PRESET UP
			case 25:

				if (!midiSetup) {
					presetDown = false;
					presetScrollSpeed = 10000;
					if ((!saveBounce) && (!loadTimer) && (!scrolled)) {
						preset--;
						if (preset < 1) {
							preset = 99;
						}
						ledNumber(preset);
					} else {
						saveEngaged = false;
					}
					saveModeTimer = 0;
				}
				scrolled = false;
				break; // PRESET DOWN

			case 23:
				resetDown = false;
				break; // PRESET RESET

			case 7:
				if (!fatChanged) {
					unShowFilterAssigns();

					if (!assignmentChanged) {
						filterMode++;
						if (filterMode > 4)
							filterMode = 0;
						updateFilter();
						sendCC(55, map(filterMode, 0, 4, 0, 1023));
					}
				} else {
					fatChanged = false;
				}
				filterModeHeld = false;
				break; //  FILTER MODE
		}
	}
}
