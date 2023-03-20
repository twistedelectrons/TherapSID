#pragma once
#include <stdint.h>

typedef void (*note_on_callback_t)(uint8_t, uint8_t, uint8_t);
typedef void (*note_off_callback_t)(uint8_t, uint8_t);

class MidiPedalAdapter {
  public:
	MidiPedalAdapter(note_on_callback_t note_on_callback, note_off_callback_t note_off_callback)
	    : held_lo{0}, held_hi{0}, pedal_down{0}, note_on_callback(note_on_callback),
	      note_off_callback(note_off_callback) {}

	void set_pedal(uint8_t channel, bool pedal_down) {

		// when pedal goes down, we want to add the held fingers to the held_lo/hi arrays
		// otherwise stuck notes if pedal goes up/down twice
		if (pedal_down) {
			held_lo[channel] = held_Finger_lo[channel];
			held_hi[channel] = held_Finger_hi[channel];
		}
		if (this->pedal_down[channel] && !pedal_down) {
			for (int i = 0; i < 64; i++) {
				if (held_lo[channel] & (1ULL << i)) {
					if (held_Finger_lo[channel] & (1ULL << i)) {
					} else {
						note_off_callback(channel, i);
					}
				}
			}
			for (int i = 0; i < 64; i++) {
				if (held_hi[channel] & (1ULL << i)) {
					if (held_Finger_hi[channel] & (1ULL << i)) {
					} else {
						note_off_callback(channel, i + 64);
					}
				}
			}
			held_lo[channel] = 0;
			held_hi[channel] = 0;
		}

		this->pedal_down[channel] = pedal_down;
	}

	void note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
		if (velocity == 0) {
			note_off(channel, note);
			return;
		}

		if (note < 64) {
			held_Finger_lo[channel] |= 1ULL << note;
		} else {
			held_Finger_hi[channel] |= 1ULL << (note - 64);
		}

		if (pedal_down[channel]) {
			if (note < 64) {
				held_lo[channel] |= 1ULL << note;
			} else {
				held_hi[channel] |= 1ULL << (note - 64);
			}
		}

		if (note < 64) {
			if ((held_Finger_hi[channel] & (1ULL << note)) && ((held_hi[channel] & (1ULL << note)))) {
			} else {
				note_on_callback(channel, note, velocity);
			}
		} else { // do not retrigger an already singing voice or it gets stuck (not sure why)
			if (((held_Finger_hi[channel] & (1ULL << (note - 64)))) && ((held_hi[channel] & (1ULL << (note - 64))))) {
			} else {
				note_on_callback(channel, note, velocity);
			} // do not retrigger an already singing voice or it gets stuck (not sure why)
		}
	}

	void note_off(uint8_t channel, uint8_t note) {

		if (note < 64) {
			held_Finger_lo[channel] &= ~(1ULL << note);
		} else {
			held_Finger_hi[channel] &= ~(1ULL << (note - 64));
		}

		if (!pedal_down[channel]) {
			if (note < 64) {
				held_lo[channel] &= ~(1ULL << note);
			} else {
				held_hi[channel] &= ~(1ULL << (note - 64));
			}

			note_off_callback(channel, note);
		}
	}

  private:
	uint64_t held_lo[16];
	uint64_t held_hi[16];

	uint64_t held_Finger_lo[16];
	uint64_t held_Finger_hi[16];

	bool pedal_down[16];

	note_on_callback_t note_on_callback;
	note_off_callback_t note_off_callback;
};
