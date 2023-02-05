#include "Arduino.h"
#include "voice_allocation.hpp"

const long ONE_FP = 1L<<16;

struct Voice {
	int destination_pitch;
	int current_pitch() const { return current_pitch_fp / ONE_FP; }

private:
	int glide_counter = 0;
	long current_pitch_fp = 0;

	void glide_tick(int glide) {
		if (glide) {
			glide_counter++;
			if (glide_counter >= glide) {
				glide_counter = 0;
				current_pitch_fp += (destination_pitch * ONE_FP - current_pitch_fp) / glide;
			}
		}
		else {
			current_pitch_fp = destination_pitch * ONE_FP;
		}
	}
};


template<size_t N_VOICES> struct MachineState {
	Voice voice[N_VOICES];
	MonoNoteTracker<42> mono_tracker[N_VOICES];
	PolyVoiceAllocator<42> voice_allocator;

	void set_n_individual_voices(int n) {
		n_individual_voices = n;

		// FIXME
		// turn off all gates
		// reset all allocators/trackers
	}

	void note_on(int note, int velocity) {
		
	}

	void note_off(int note) {
		note_on(note, 0);
	}

	void note_on_individual(int voice, int note);
	void note_off_individual(int voice, int note);

	private:
		int n_individual_voices = N_VOICES;
};
