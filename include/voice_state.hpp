#include "voice_allocation.hpp"

const long ONE_FP = 1L<<16;

struct Glide {
	int destination_pitch;
	int current_pitch() const { return current_pitch_fp / ONE_FP; }

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

private:
	int glide_counter = 0;
	long current_pitch_fp = 0;
};


enum class VoiceStateEvent {
	FIRST_NOTE_ON,
	LAST_NOTE_OFF,
	NEUTRAL
};

template<size_t N_OPERATORS> struct VoiceState {
	void set_n_individual_voices(int n) {
		if (n_individual_voices == n)
			return;

		n_individual_voices = n;
		n_usable_operators = (N_OPERATORS / n) * n;

		for (size_t i=0; i<N_OPERATORS; i++) {
			_gate[i] = false;
			mono_tracker[i].clear();
		}
		mono_note_tracker.clear();
		voice_allocator.clear();
	}

	void voice_on(int voice, int note) {
		for (int i=voice; i<n_usable_operators; i+=n_individual_voices) {
			_gate[i] = true;
			_key[i] = note;
		}
	}

	void voice_off(int voice) {
		for (int i=voice; i<n_usable_operators; i+=n_individual_voices) {
			_gate[i] = false;
		}
	}

	VoiceStateEvent note_on(int note, int velocity) {
		if (velocity == 0)
			return note_off(note);

		if (!_held_keys[note]) {
			_n_held_keys++;
			_held_keys[note] = true;
		}

		if (n_individual_voices == 1) { // Monophonic mode
			auto had_active_note = mono_note_tracker.has_active_note();
			mono_note_tracker.note_on(note, velocity);

			voice_on(0, note);

			if (!had_active_note) {
				return VoiceStateEvent::FIRST_NOTE_ON;
			}
		} else {             // paraphonic mode
			auto voice_idx = voice_allocator.note_on(note, velocity);
			voice_on(voice_idx, note);
		}
		return VoiceStateEvent::NEUTRAL;
	}

	VoiceStateEvent note_off(int note) {
		if (_held_keys[note]) {
			_n_held_keys--;
			_held_keys[note] = false;
		}

		if (n_individual_voices == 1) { // Monophonic mode
			mono_note_tracker.note_off(note);
			if (!mono_note_tracker.has_active_note()) {
				voice_off(0);
				return VoiceStateEvent::LAST_NOTE_OFF;
			}
			else {
				voice_on(0, mono_note_tracker.active_note()->note);
				return VoiceStateEvent::NEUTRAL;
			}
		} else {             // paraphonic mode
			auto voice_idx = voice_allocator.note_off(note);

			if (voice_idx.has_value()) {
				voice_off(*voice_idx);
			}
			return VoiceStateEvent::NEUTRAL;
		}
	}

	void note_on_individual(int voice, int note) {
		mono_tracker[voice].note_on(note, 64);
		voice_on(voice, mono_tracker[voice].active_note()->note);
	}

	void note_off_individual(int voice, int note) {
		mono_tracker[voice].note_off(note);

		if (mono_tracker[voice].has_active_note()) {
			voice_on(voice, mono_tracker[voice].active_note()->note);
		} else {
			voice_off(voice);
		}
	}

	int key(int oper) const { return _key[oper]; }
	bool gate(int oper) const { return _gate[oper]; }

	bool held_key(int key) const { return _held_keys[key]; }
	int n_held_keys() const { return _n_held_keys; }

	private:
		int n_individual_voices = N_OPERATORS;
		int n_usable_operators = N_OPERATORS;

		bool _gate[N_OPERATORS] = {false};
		int _key[N_OPERATORS];

		MonoNoteTracker<16> mono_tracker[N_OPERATORS];
		PolyVoiceAllocator<16> voice_allocator;
		MonoNoteTracker<16> mono_note_tracker;

		bool _held_keys[128] = {false};
		int _n_held_keys = 0;
};
