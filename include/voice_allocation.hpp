#pragma once

#include <stdint.h>
#include <stddef.h>

#include "util.hpp"

/** Notes are arranged in a stack. Note-on puts the desired note onto the top
    of the stack, note-off removes the desired notes from within the stack.
    The active note is always the one on the top. */
template <size_t N> class MonoNoteTracker {
  public:
	struct NoteVelo {
		uint8_t note;
		uint8_t velocity;
	};

	void note_on(uint8_t note, uint8_t velocity) {
		if (!notes.push_back(NoteVelo{note, velocity})) {
			notes.erase(0);
			notes.push_back(NoteVelo{note, velocity});
		}
	}

	void note_off(uint8_t note) {
		for (size_t i = notes.size(); i-- > 0;) {
			if (notes[i].note == note) {
				notes.erase(i);
			}
		}
	}

	bool has_active_note() const { return !notes.empty(); }

	/** Returns the MIDI note number of the currently active note, if any, or
	 * a negative number if no note is active. */
	optional<NoteVelo> active_note() const {
		if (notes.empty()) {
			return nullopt;
		} else {
			return notes.back();
		}
	}

	void clear() { notes.clear(); }

	/** Number of notes on the stack */
	size_t n_notes() const { return notes.size(); }

  private:
	StaticVector<NoteVelo, N> notes;
};

template <size_t N> class PolyVoiceAllocator {
  public:
	struct VoiceSlot {
		uint8_t note = 0;
		uint8_t velocity = 0;
		bool playing = false;
	};

	void set_max_voices(size_t value) {
		if (value > N) {
			value = N;
		}
		max_voices = value;
		next_slot %= max_voices;

		for (auto i = max_voices; i < N; i++) {
			voices[i].playing = false;
		}
	}

	optional<size_t> find_voice(uint8_t note) const {
		for (size_t i = 0; i < max_voices; i++) {
			if (voices[i].note == note && voices[i].playing) {
				return i;
			}
		}
		return nullopt;
	}

	size_t note_on(uint8_t note, uint8_t velocity) {
		for (size_t i = 0; i < max_voices; i++) {
			if (!voices[next_slot].playing) {
				break;
			}
			next_slot = (next_slot + 1) % max_voices;
		}

		auto slot = next_slot;
		voices[slot] = VoiceSlot{note, velocity, true};
		next_slot = (next_slot + 1) % max_voices;

		return slot;
	}

	optional<size_t> note_off(uint8_t note) {
		auto voice_idx = find_voice(note);
		if (voice_idx.has_value()) {
			voices[*voice_idx].playing = false;
		}
		return voice_idx;
	}

	VoiceSlot const* get_voices() const { return voices; }

	void clear() {
		for (auto& voice : voices) {
			voice.playing = false;
		}
	}

  private:
	VoiceSlot voices[N];
	size_t next_slot = 0;
	size_t max_voices = N;
};
