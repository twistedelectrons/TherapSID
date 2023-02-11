#pragma once
#include <stdint.h>

typedef void (*note_on_callback_t)(uint8_t, uint8_t, uint8_t);
typedef void (*note_off_callback_t)(uint8_t, uint8_t);

class MidiPedalAdapter {
	public:
		MidiPedalAdapter(note_on_callback_t note_on_callback, note_off_callback_t note_off_callback)
		    : held_lo{0}, held_hi{0}, pedal_down(false), note_on_callback(note_on_callback),
		      note_off_callback(note_off_callback) {}

		void set_pedal(uint8_t channel, bool pedal_down) {
			if (this->pedal_down && !pedal_down) {
				for (int i = 0; i < 64; i++) {
					if (held_lo[channel] & (1 << i)) {
						note_off_callback(channel, i);
					}
				}
				for (int i = 0; i < 64; i++) {
					if (held_hi[channel] & (1 << i)) {
						note_off_callback(channel, i + 64);
					}
				}
				held_lo[channel] = 0;
				held_hi[channel] = 0;
			}

			this->pedal_down = pedal_down;
		}

		void note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
			if (velocity == 0) {
				note_off(channel, note);
				return;
			}

			if (note < 64)
				held_lo[channel] |= 1 << note;
			else
				held_hi[channel] |= 1 << (note - 64);

			note_on_callback(channel, note, velocity);
		}

		void note_off(uint8_t channel, uint8_t note) {
			if (!pedal_down) {
				if (note < 64)
					held_lo[channel] &= ~(1 << note);
				else
					held_hi[channel] &= ~(1 << (note - 64));

				note_off_callback(channel, note);
			}
		}

	private:
		uint64_t held_lo[16];
		uint64_t held_hi[16];

		bool pedal_down;

		note_on_callback_t note_on_callback;
		note_off_callback_t note_off_callback;
};
