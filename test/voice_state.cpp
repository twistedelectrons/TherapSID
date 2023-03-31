#include "3rdparty/catch_amalgamated.hpp"
#include "voice_state.hpp"
#include <array>

/// negative numbers mean "off with this key", zero means "off, don't care about the key"
void check_keys(VoiceState<6> const& state, std::array<int, 6> keys) {
	for (int i=0; i<6; i++) {
		if (keys[i] == 0) {
			CHECK_FALSE(state.gate(i));
		}
		else if (keys[i] < 0) {
			CHECK_FALSE(state.gate(i));
			CHECK(state.key(i) == -keys[i]);
		}
		else {
			CHECK(state.gate(i));
			CHECK(state.key(i) == keys[i]);
		}
	}
}

TEST_CASE("VoiceState") {
	VoiceState<6> state;

	SECTION("in mono mode") {
		state.set_n_individual_voices(1); // enable mono mode

		SECTION("keeps stack of notes like MonoNoteTracker") {
			state.note_on(1, 64);
			check_keys(state, {1,1,1,1,1,1});

			state.note_on(2, 64);
			check_keys(state, {2,2,2,2,2,2});

			state.note_off(2);
			check_keys(state, {1,1,1,1,1,1});

			state.note_off(1);
			check_keys(state, {-1,-1,-1,-1,-1,-1});
		}
		
		SECTION("reports FIRST_NOTE_ON, NEUTRAL and LAST_NOTE_OFF") {
			CHECK(state.note_on(1, 64) == VoiceStateEvent::FIRST_NOTE_ON);
			CHECK(state.note_on(2, 64) == VoiceStateEvent::NEUTRAL);
			CHECK(state.note_off(1) == VoiceStateEvent::NEUTRAL);
			CHECK(state.note_off(2) == VoiceStateEvent::LAST_NOTE_OFF);
		}

		SECTION("note_{on,off}_individual override what note_on says") {
			state.note_on(1, 64);
			check_keys(state, {1,1,1,1,1,1});
			state.note_on_individual(2, 42);
			check_keys(state, {1,1,42,1,1,1});
			state.note_off(1);
			check_keys(state, {-1,-1,42,-1,-1,-1});
			state.note_on(1, 64);
			check_keys(state, {1,1,42,1,1,1});
			state.note_off_individual(2, 42);
			check_keys(state, {1,1,1,1,1,1});
		}

		// regression test for https://github.com/twistedelectrons/TherapSID/issues/23
		SECTION("releasing an individual override stays at the released note if no main note is held") {
			state.note_on(1, 64);
			state.note_off(1);
			check_keys(state, {-1,-1,-1,-1,-1,-1});
			state.note_on_individual(2, 42);
			check_keys(state, {-1,-1,42,-1,-1,-1});
			state.note_off_individual(2, 42);
			check_keys(state, {-1,-1,-42,-1,-1,-1});
		}
	}

	SECTION("in poly mode") {
		state.set_n_individual_voices(3); // poly mode

		SECTION("does 2-fold unisono when configured with 3 voices on 6 operators") {
			state.note_on(42, 64);
			check_keys(state, {42, 0, 0, 42, 0, 0});

			state.note_on(23, 64);
			check_keys(state, {42, 23, 0, 42, 23, 0});

			state.note_off(42);
			check_keys(state, {-42, 23, 0, -42, 23, 0});
		}
	}
/*
	SECTION("in any mode: keeps track of held keys") {
		for (int voices : {1, 3, 5, 6}) {
			VoiceState<6> state;
			state.set_n_individual_voices(voices);

			CHECK(state.n_held_keys() == 0);
			CHECK(!state.held_key(1));
			CHECK(!state.held_key(23));
			CHECK(!state.held_key(42));

			state.note_on(42, 64);
			CHECK(state.n_held_keys() == 1);
			CHECK(!state.held_key(1));
			CHECK(!state.held_key(23));
			CHECK(state.held_key(42));
			
			state.note_on(23, 64);
			state.note_on(23, 64);
			CHECK(state.n_held_keys() == 2);
			CHECK(!state.held_key(1));
			CHECK(state.held_key(23));
			CHECK(state.held_key(42));
			
			state.note_off(1);
			CHECK(state.n_held_keys() == 2);
			CHECK(!state.held_key(1));
			CHECK(state.held_key(23));
			CHECK(state.held_key(42));
			
			state.note_off(42);
			CHECK(state.n_held_keys() == 1);
			CHECK(!state.held_key(1));
			CHECK(state.held_key(23));
			CHECK(!state.held_key(42));
			
			state.note_off(23);
			state.note_off(23);
			CHECK(state.n_held_keys() == 0);
			CHECK(!state.held_key(1));
			CHECK(!state.held_key(23));
			CHECK(!state.held_key(42));
		}
	}*/
}
