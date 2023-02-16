#include "3rdparty/catch_amalgamated.hpp"
#include "voice_allocation.hpp"

template<size_t N> void check_note(PolyVoiceAllocator<N> const& all, int note, int velocity) {
	auto slot = all.find_voice(note);
	REQUIRE(slot.has_value());
	CHECK(all.get_voices()[*slot].note == note);
	CHECK(all.get_voices()[*slot].velocity == velocity);
	CHECK(all.get_voices()[*slot].playing);
}

template<size_t N> int n_playing(PolyVoiceAllocator<N> const& all) {
	int count = 0;
	for (size_t i=0; i<N; i++) {
		if (all.get_voices()[i].playing) {
			count++;
		}
	}
	return count;
}

TEST_CASE("PolyVoiceAllocator") {
	PolyVoiceAllocator<4> all;

	SECTION("empty allocator has no active note") {
		CHECK(n_playing(all) == 0);
	}

	SECTION("cleared allocator has no active note") {
		all.note_on(42, 42);
		CHECK(n_playing(all) == 1);
		all.clear();
		CHECK(n_playing(all) == 0);
	}

	SECTION("note_off on empty allocator has no effect") {
		all.note_off(42);
		CHECK(n_playing(all) == 0);
	}

	SECTION("can play up to N notes") {
		for (int i=0; i<4; i++) {
			all.note_on(i, i);
		}
		for (int i=0; i<4; i++) {
			check_note(all, i, i);
		}

		SECTION("and releasing notes frees slots for new notes") {
			all.note_off(2);
			all.note_off(10); // tolerates spurious off
			CHECK(n_playing(all) == 3);
			all.note_on(4,4);
			check_note(all, 0, 0);
			check_note(all, 1, 1);
			check_note(all, 3, 3);
			check_note(all, 4, 4);
			
			all.note_off(4);
			all.note_off(4); // tolerates double off
			CHECK(n_playing(all) == 3);
			all.note_off(0);
			CHECK(n_playing(all) == 2);
			all.note_on(5,5);
			CHECK(n_playing(all) == 3);
			all.note_on(6,6);
			check_note(all, 1, 1);
			check_note(all, 3, 3);
			check_note(all, 5, 5);
			check_note(all, 6, 6);
		}

		SECTION("and playing more notes will cycle through the old notes to kill") {
			for (int i=0; i<4; i++) {
				all.note_on(4+i,4+i); // kills note i

				for (int j=i+1; j<=i; j++) {
					check_note(all, j, j);
				}
			}
		}
	}

	SECTION("voice-limited PolyVoiceAllocator<16> behaves same as a PolyVoiceAllocator<4>") {
		PolyVoiceAllocator<16> all2;
		all2.set_max_voices(4);

		struct Cmd {
			bool on;
			int note;
		};

		Cmd commands[] = { {true, 1}, {true, 2}, {true, 3}, {false, 2}, {true, 4}, {false, 8}, {true, 5}, {true, 6}, {true, 7}, {true, 8}, {false, 7}, {false, 7}, {true, 9}, {false, 9}, {false, 8}, {true, 10} };

		for (const auto& cmd: commands) {
			if (cmd.on) {
				all.note_on(cmd.note, 64);
				all2.note_on(cmd.note, 64);
			}
			else {
				all.note_off(cmd.note);
				all2.note_off(cmd.note);
			}

			for (int i=0; i<4; i++) {
				CHECK(all.get_voices()[i].note == all2.get_voices()[i].note);
				CHECK(all.get_voices()[i].playing == all2.get_voices()[i].playing);
				CHECK(all.get_voices()[i].velocity == all2.get_voices()[i].velocity);
			}
		}
	}
}
