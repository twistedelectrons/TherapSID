#pragma once
#include "voice_allocation.hpp"

struct Glide {
	int destination_pitch;
	int current_pitch() const { return current_pitch_; }

	void glide_tick(uint8_t glide) {
		if (glide) {
			glide_counter++;
			if (glide_counter >= glide) {
				glide_counter = 0;
				current_pitch_ += (destination_pitch - current_pitch_) / glide;

				// nudge towards destination_pitch because of above imperfect integer division
				if (current_pitch_ < destination_pitch)
					current_pitch_++;
				else if (current_pitch_ > destination_pitch)
					current_pitch_--;
			}
		} else {
			current_pitch_ = destination_pitch;
		}
	}

  private:
	uint8_t glide_counter = 0;
	int current_pitch_ = 0;
};

enum class VoiceStateEvent { FIRST_NOTE_ON, LAST_NOTE_OFF, NEUTRAL };

template <size_t N_OPERATORS> struct VoiceState {
	void set_n_individual_voices(int n) {
		if (n_individual_voices == n)
			return;

		n_individual_voices = n;
		n_usable_operators = (N_OPERATORS / n) * n;

		for (size_t i = 0; i < N_OPERATORS; i++) {
			mono_tracker[i].clear();
		}
		mono_note_tracker.clear();
		voice_allocator.set_max_voices(n);
		voice_allocator.clear();
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
			for (int i = 0; i < 6; i++) {
				myLastNote[i] = mono_note_tracker.active_note()->note;
			}

			if (!had_active_note) {
				return VoiceStateEvent::FIRST_NOTE_ON;
			}
		} else { // paraphonic mode
			voice_allocator.note_on(note, velocity);
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
			if (mono_note_tracker.has_active_note()) {
				for (int i = 0; i < 6; i++) {
					myLastNote[i] = mono_note_tracker.active_note()->note;
				}
				return VoiceStateEvent::NEUTRAL;
			} else {
				return VoiceStateEvent::LAST_NOTE_OFF;
			}
		} else { // paraphonic mode
			voice_allocator.note_off(note);
			return VoiceStateEvent::NEUTRAL;
		}
	}

	void note_on_individual(int voice, int note) {
		mono_tracker[voice].note_on(note, 64);
		myLastNote[voice] = mono_tracker[voice].active_note()->note;
	}

	void note_off_individual(int voice, int note) {
		mono_tracker[voice].note_off(note);
		if (mono_tracker[voice].has_active_note())
			myLastNote[voice] = mono_tracker[voice].active_note()->note;
	}

	int has_individual_override(int oper) {
		if (n_individual_voices == 1) {
			return mono_tracker[oper].has_active_note();
		} else {
			return false;
		}
	}

	int key(int oper) const {
		if (oper >= n_usable_operators) {
			return 0;
		}

		if (n_individual_voices == 1) {
			auto individual = mono_tracker[oper].active_note();
			if (individual.has_value()) {
				return individual->note;
			} else if (mono_note_tracker.has_active_note()) {
				return mono_note_tracker.active_note()->note;
			} else {
				return myLastNote[oper];
			}
		} else if (n_individual_voices == 2) {
			int voice = oper / 3;
			return voice_allocator.get_voices()[voice].note;
		} else {
			int voice = oper % n_individual_voices;
			return voice_allocator.get_voices()[voice].note;
		}
	}
	bool gate(int oper) const {
		if (oper >= n_usable_operators) {
			return false;
		}

		if (n_individual_voices == 1) {
			return mono_tracker[oper].has_active_note() || mono_note_tracker.has_active_note();
		} else if (n_individual_voices == 2) {
			int voice = oper / 3;
			return voice_allocator.get_voices()[voice].playing;
		} else {
			int voice = oper % n_individual_voices;
			return voice_allocator.get_voices()[voice].playing;
		}
	}

	bool held_key(int key) const { return _held_keys[key]; }
	int n_held_keys() const { return _n_held_keys; }

  private:
	uint8_t myLastNote[6]; // track the last note of the individual voices in an effort to fix issue #23
	int n_individual_voices = N_OPERATORS;
	int n_usable_operators = N_OPERATORS;

	MonoNoteTracker<16> mono_tracker[N_OPERATORS];
	PolyVoiceAllocator<6> voice_allocator;
	MonoNoteTracker<16> mono_note_tracker;

	bool _held_keys[128] = {false};
	int _n_held_keys = 0;
};
