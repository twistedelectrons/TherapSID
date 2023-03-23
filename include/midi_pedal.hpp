#pragma once
#include <stdint.h>

typedef void (*note_on_callback_t)(uint8_t, uint8_t, uint8_t);
typedef void (*note_off_callback_t)(uint8_t, uint8_t);

class MidiPedalAdapter {
  public:
	MidiPedalAdapter(note_on_callback_t note_on_callback, note_off_callback_t note_off_callback)
	    : note_held{0}, note_sustained{0}, pedal_down{0}, note_on_callback(note_on_callback),
	      note_off_callback(note_off_callback) {}

	void set_pedal(uint8_t channel, bool pedal_down) {

		this->pedal_down = pedal_down;

		// when pedal goes down, we want to transfer the notes that are held to sustained
		if (pedal_down) {
			for (int i = 0; i < 127; i++) {
				setSustained(i, getHeld(i));
			}
		} else {

			// when pedal goes up, we want to kill the notes that are sustained
			for (int i = 0; i < 127; i++) {
				if ((getSustained(i)) && (!getHeld(i))) {
					//*(only if not held by fingers)

					note_off_callback(channel, i);
				}
			}

			// clear sustained notes
			for (int i = 0; i < 127; i++) {
				setSustained(i, 0);
			}
		}
	}

	void note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
		if (velocity == 0) {
			note_off(channel, note);
			return;
		}
		if (channel != masterChannel) {
			note_on_callback(channel, note, velocity);
			return;
		}

		// is the note already singing? Retrigger
		if (getSustained(note)) {
			note_off_callback(channel, note);
		}

		// trigger the note
		note_on_callback(channel, note, velocity);

		// add note to held array
		setHeld(note, 1);

		if (pedal_down) {
			// pedal is down so we want to sustain the note
			setSustained(note, 1);
		}
	}

	void note_off(uint8_t channel, uint8_t note) {

		if (channel != masterChannel) {
			note_off_callback(channel, note);
			return;
		}

		// remove note from held array
		setHeld(note, 0);

		if (!pedal_down) {
			note_off_callback(channel, note);
		}
	}

	void setSustained(uint8_t note, bool value) { note_sustained[note] = value; }

	bool getSustained(uint8_t note) { return (note_sustained[note]); }

	void setHeld(uint8_t note, bool value) { note_held[note] = value; }

	bool getHeld(uint8_t note) { return (note_held[note]); }

  private:
	bool note_held[127];      // finger is pressing the note
	bool note_sustained[127]; // finger is not pressing the note but pedal is sustaining it

	bool pedal_down;

	note_on_callback_t note_on_callback;
	note_off_callback_t note_off_callback;
};
