#include "globals.h"
#include "midi.h"
#include "arp.h"
#include "leds.h"

static byte arpNotes[128];
static int arpNote;
static bool arpPendulum;
static byte scrubNote, scrubNoteLast;

static int arp_note; // FIXME FIXME FIXME this is never used. send note_on/off to the voice manager instead!

void showArp() {
	arpRound = 0;
	arpCounter = 0;
	arpNote = 0;

	// don't get in the way of startup preset display
	if (millis() > 2000)
		frozen = 500;

	switch (arpMode) {

		case 0: // OFF
			digit(0, 0);
			digit(1, 12);
			break;

		case 1: // UP
			digit(0, 13);
			digit(1, 14);
			break;

		case 2: // DOWN
			digit(0, 15);
			digit(1, 0);
			break;

		case 3: // UP/DOWN
			digit(0, 13);
			digit(1, 15);
			break;

		case 4: // UP/DOWN
			digit(0, 16);
			digit(1, 15);
			break;
	}
}

void arpSteptrigger(int number) {

	if (number > 255) {
		number = 255;
	} else if (number < 1) {
		number = 1;
	}

	// count held notes
	arpCount = 0;
	for (int i = 0; i < 127; i++) {
		if (voice_state.held_key(i)) {
			arpNotes[arpCount] = i;
			arpCount++;
		}
	}

	// multiply by range
	int arpCountTotal = arpCount + (arpRange * arpCount);

	int arpCountTarget = map(number, 0, 255, 0, arpCountTotal - 1);

	scrubNote = 0;
	while (arpCountTarget >= arpCount) {
		arpCountTarget -= arpCount;
		if (arpCountTarget < 0) {
			arpCountTarget = 0;
		}
		scrubNote += 12;
	}
	scrubNote += arpNotes[arpCountTarget];

	if ((scrubNote) && (scrubNote != scrubNoteLast)) {
		scrubNoteLast = arp_note = scrubNote;
		if (sendArp)
			midiOut(arp_note);
	}
}

void arpReset(byte note) {

	switch (arpMode) {

		case 1:
			arpNote = 0;
			arpRound = 0;
			break;
		case 2:
			arpNote = 127;
			arpRound = arpRange;
			break;
		case 3:
			arpNote = 0;
			arpRound = 0;
			arpPendulum = 0;
			break;
	}
	arpCounter = 0;
	arpTick();
}

void arpTick() {
	if (voice_state.n_held_keys() > 0) {
		if (arpMode) {
			if (retrig[0]) {
				lfoStep[0] = 0;
			}
			if (retrig[1]) {
				lfoStep[1] = 0;
			}
			if (retrig[2]) {
				lfoStep[2] = 0;
			}

			switch (arpMode) {
				case 1:
					arpNote++;
					if (arpNote > 127)
						arpNote = 0;
					while (!voice_state.held_key(arpNote)) {
						arpNote++;
						if (arpNote > 127) {
							arpNote = 0;
							arpRound++;
							if (arpRound > arpRange) {
								arpRound = 0;
							}
						}
					}
					arp_note = arpNote + (arpRound * 12);
					if (sendArp)
						midiOut(arp_note);
					break;

				case 2:
					arpNote--;
					if (arpNote < 1)
						arpNote = 127;
					while (!voice_state.held_key(arpNote)) {
						arpNote--;
						if (arpNote == 0) {
							arpNote = 127;
							arpRound--;
							if (arpRound < 0) {
								arpRound = arpRange;
							}
						}
					}
					arp_note = arpNote + (arpRound * 12);
					if (sendArp)
						midiOut(arp_note);
					break;

				case 3:
					switch (arpPendulum) {
						case 0:
							arpNote++;
							while (!voice_state.held_key(arpNote)) {
								arpNote++;
								if (arpNote > 127) {
									arpNote = 0;
									arpRound++;
									if (arpRound > arpRange) {
										arpRound--;
										arpPendulum = 1;
									}
								}
							}
							arp_note = arpNote + (arpRound * 12);
							if (sendArp)
								midiOut(arp_note);
							break;

						case 1:
							arpNote--;
							while (!voice_state.held_key(arpNote)) {
								arpNote--;
								if (arpNote == 0) {
									arpNote = 127;
									arpRound--;
									if (arpRound < 0) {
										arpRound++;
										arpPendulum = 0;
									}
								}
							}
							arp_note = arpNote + (arpRound * 12);
							if (sendArp)
								midiOut(arp_note);
							break;
					}
					break;

				case 4:
					arpSteptrigger(random(255));
					break;
			}
		}
	}
}
