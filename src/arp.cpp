#include "globals.h"
#include "midi.h"
#include "arp.h"

static byte arpNotes[128];
static int arpNote;
static bool arpPendulum;
static byte scrubNote, scrubNoteLast;
static byte trillStep;

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

	int arpCountTarget = map(number, 0, 256, 0, arpCountTotal);

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
		scrubNoteLast = arp_output_note = scrubNote;
		if (sendArp)
			midiOut(arp_output_note);
	}
}

void arpReset() {

	switch (preset_data.arp_mode) {

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
		case 5:
			arpNote = 0;
			arpRound = 0;
			while (!voice_state.held_key(arpNote)) {
				arpNote++;
			}
			break;
	}
	arpCounter = 0;
	arpTick();
}

byte lastArpNote;

void arpTick() {
	if (voice_state.n_held_keys() > 0) {
		if (preset_data.arp_mode) {
			if (preset_data.lfo[0].retrig || voice_state.n_held_keys() == 1) { // always retrigger on the first key
				lfoStep[0] = 0;
			}
			if (preset_data.lfo[1].retrig || voice_state.n_held_keys() == 1) { // always retrigger on the first key
				lfoStep[1] = 0;
			}
			if (preset_data.lfo[2].retrig || voice_state.n_held_keys() == 1) { // always retrigger on the first key
				lfoStep[2] = 0;
			}

			switch (preset_data.arp_mode) {
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
					arp_output_note = arpNote + (arpRound * 12);
					if (sendArp)
						midiOut(arp_output_note);
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
					arp_output_note = arpNote + (arpRound * 12);
					if (sendArp)
						midiOut(arp_output_note);
					break;

				case 3:
					// UP DOWN
					if (arpPendulum) {
						arpNote++;
						if (arpNote > 127) {
							arpNote = 126; // overtook highest note, down one
						}

						while (!voice_state.held_key(arpNote)) {
							arpNote++;
							if (arpNote > 127) {

								arpNote = 126; // overtook highest note, down one
								if (arpRange) {
									arpRound++;
									if (arpRound > arpRange) {
										arpRound = arpRange;
										arpPendulum = 0;

										// get next note down
										while (!voice_state.held_key(arpNote)) {
											arpNote--;
										}
										// get next note down againn so as not to repeat the same note
										while (!voice_state.held_key(arpNote)) {
											arpNote--;
										}

									} else {
										arpNote = 0;
										// get next note up
										while (!voice_state.held_key(arpNote)) {
											arpNote++;
										}
									}
								} else {
									arpPendulum = 0;
									// get next note down
									while (!voice_state.held_key(arpNote)) {
										arpNote--;
									}
								}
							}
						}

					} else {
						arpNote--;
						if (arpNote < 1) {
							arpNote = 0;
						}

						while (!voice_state.held_key(arpNote)) {
							arpNote--;
							if (arpNote < 0) {
								arpNote = 0;
								if (arpRange) {
									arpRound--;
									if (arpRound < 0) {
										arpRound = 0;
										arpPendulum = 1;

										// get next note up
										while (!voice_state.held_key(arpNote)) {
											arpNote++;
										}

									} else {
										arpNote = 127;
										// get next note down
										while (!voice_state.held_key(arpNote)) {
											arpNote--;
										}
									}
								} else {
									arpPendulum = 1;

									// get next note up
									while (!voice_state.held_key(arpNote)) {
										arpNote++;
									}
								}
							}
						}
					}

					if (lastArpNote == arpNote + (arpRound * 12)) {
						if (arpPendulum) {
							arpNote++;
							if (arpNote > 127) {
								arpNote = 126; // overtook highest note, down one
							}

							while (!voice_state.held_key(arpNote)) {
								arpNote++;
								if (arpNote > 127) {

									arpNote = 126; // overtook highest note, down one
									if (arpRange) {
										arpRound++;
										if (arpRound > arpRange) {
											arpRound = arpRange;
											arpPendulum = 0;

											// get next note down
											while (!voice_state.held_key(arpNote)) {
												arpNote--;
											}
											// get next note down againn so as not to repeat the same note
											while (!voice_state.held_key(arpNote)) {
												arpNote--;
											}

										} else {
											arpNote = 0;
											// get next note up
											while (!voice_state.held_key(arpNote)) {
												arpNote++;
											}
										}
									} else {
										arpPendulum = 0;
										// get next note down
										while (!voice_state.held_key(arpNote)) {
											arpNote--;
										}
									}
								}
							}

						} else {
							arpNote--;
							if (arpNote < 1) {
								arpNote = 0;
							}

							while (!voice_state.held_key(arpNote)) {
								arpNote--;
								if (arpNote < 0) {
									arpNote = 0;
									if (arpRange) {
										arpRound--;
										if (arpRound < 0) {
											arpRound = 0;
											arpPendulum = 1;

											// get next note up
											while (!voice_state.held_key(arpNote)) {
												arpNote++;
											}

										} else {
											arpNote = 127;
											// get next note down
											while (!voice_state.held_key(arpNote)) {
												arpNote--;
											}
										}
									} else {
										arpPendulum = 1;

										// get next note up
										while (!voice_state.held_key(arpNote)) {
											arpNote++;
										}
									}
								}
							}
						}
					}
					lastArpNote = arp_output_note = arpNote + (arpRound * 12);
					if (sendArp)
						midiOut(arp_output_note);
					break;

				case 4:
					arpSteptrigger(random(255));
					break;

				case 5:
					arpRound++;
					if (arpRound > arpRange) {
						arpRound = 0;
						arpNote++;
						if (arpNote > 127)
							arpNote = 0;
						while (!voice_state.held_key(arpNote)) {
							arpNote++;
							if (arpNote > 127) {
								arpNote = 0;
							}
						}
					}
					arp_output_note = arpNote + (arpRound * 12);
					if (sendArp)
						midiOut(arp_output_note);
					break;

				case 6:
				case 7:
					trillStep++;
					if (trillStep > 2) {
						trillStep = 0;
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
					}
					arp_output_note = arpNote + (arpRound * 12);
					if (trillStep == 1) {
						arp_output_note += preset_data.arp_mode - 5;
					}
					if (sendArp)
						midiOut(arp_output_note);
					break;
			}
		}
	}
}
