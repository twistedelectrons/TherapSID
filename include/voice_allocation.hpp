#pragma once

#include <stdint.h>
#include <stddef.h>

#include "util.hpp"

template <size_t N> class MonoNoteTracker {
	public:
		struct NoteVelo {
				uint8_t note;
				uint8_t velocity;
		};

		void note_on(uint8_t note, uint8_t velocity) { notes.push_back(NoteVelo{note, velocity}); }

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

	private:
		StaticVector<NoteVelo, N> notes;
};

template <size_t N> class PolyVoiceAllocator {
	public:
		struct VoiceSlot {
				uint8_t note;
				uint8_t velocity;
				bool playing;
		};

		void set_max_voices(size_t value) {
			if (value > N) {
				value = N;
			}
			max_voices = value;
			next_slot %= max_voices;

			for (int i = max_voices; i < N; i++) {
				voices[i].playing = false;
			}
		}

		optional<size_t> find_voice(uint8_t note) {
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

			ledNumber((!!voices[0].playing) + (!!voices[1].playing) * 2 + (!!voices[2].playing) * 4);

			return slot;
		}

		optional<size_t> note_off(uint8_t note) {
			auto voice_idx = find_voice(note);
			if (voice_idx.has_value()) {
				voices[*voice_idx].playing = false;
			}
			ledNumber((!!voices[0].playing) + (!!voices[1].playing) * 2 + (!!voices[2].playing) * 4);
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

// FIXME separate globals from library code
extern MonoNoteTracker<16> mono_note_tracker;
extern MonoNoteTracker<16> mono_note_trackers[3];
extern PolyVoiceAllocator<3> voice_allocator;
