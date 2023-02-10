#include "3rdparty/catch_amalgamated.hpp"
#include "voice_allocation.hpp"

TEST_CASE("MonoNoteTracker") {
	MonoNoteTracker<16> tracker;

	SECTION("empty tracker has no active note") {
		CHECK_FALSE(tracker.has_active_note());
		CHECK_FALSE(tracker.active_note().has_value());
	}

	SECTION("cleared tracker has no active note") {
		tracker.note_on(1, 1);
		CHECK(tracker.has_active_note());
		tracker.clear();
		CHECK_FALSE(tracker.has_active_note());
	}

	SECTION("note_off on empty tracker has no effect") {
		tracker.note_off(42);
		CHECK_FALSE(tracker.has_active_note());
		CHECK_FALSE(tracker.active_note().has_value());
	}

	SECTION("notes stack and top-of-stack is active note") {
		tracker.note_on(60, 6);
		CHECK(tracker.has_active_note());
		CHECK(tracker.active_note()->note == 60);
		CHECK(tracker.active_note()->velocity == 6);

		tracker.note_on(80, 8);
		CHECK(tracker.has_active_note());
		CHECK(tracker.active_note()->note == 80);
		CHECK(tracker.active_note()->velocity == 8);

		tracker.note_on(40, 4);
		CHECK(tracker.has_active_note());
		CHECK(tracker.active_note()->note == 40);
		CHECK(tracker.active_note()->velocity == 4);

		tracker.note_on(20, 2);
		CHECK(tracker.has_active_note());
		CHECK(tracker.active_note()->note == 20);
		CHECK(tracker.active_note()->velocity == 2);

		SECTION("when releasing in opposite order") {
			tracker.note_off(20);
			tracker.note_off(30); // withstands spurious release
			CHECK(tracker.has_active_note());
			CHECK(tracker.active_note()->note == 40);
			CHECK(tracker.active_note()->velocity == 4);

			tracker.note_off(40);
			CHECK(tracker.has_active_note());
			CHECK(tracker.active_note()->note == 80);
			CHECK(tracker.active_note()->velocity == 8);

			tracker.note_off(80);
			tracker.note_off(80); // withstands double release
			CHECK(tracker.has_active_note());
			CHECK(tracker.active_note()->note == 60);
			CHECK(tracker.active_note()->velocity == 6);

			tracker.note_off(60);
			CHECK_FALSE(tracker.has_active_note());
		}

		SECTION("when releasing in different order") {
			tracker.note_off(40);
			CHECK(tracker.has_active_note());
			CHECK(tracker.active_note()->note == 20);
			CHECK(tracker.active_note()->velocity == 2);

			tracker.note_off(20);
			CHECK(tracker.has_active_note());
			CHECK(tracker.active_note()->note == 80);
			CHECK(tracker.active_note()->velocity == 8);

			tracker.note_off(60);
			CHECK(tracker.has_active_note());
			CHECK(tracker.active_note()->note == 80);
			CHECK(tracker.active_note()->velocity == 8);

			tracker.note_off(80);
			CHECK_FALSE(tracker.has_active_note());
		}
	}

	SECTION("when exceeding the slots, oldest note is forgotten") {
		// we have 16 slots and are exceeding them by 2
		for (int i=1; i<=18; i++) {
			tracker.note_on(i, 64);
			CHECK(tracker.has_active_note());
			CHECK((int)tracker.active_note()->note == i);
		}
		// we release 18 through 4, i.e. 15 notes.
		for (int i=18; i>=4; i--) {
			tracker.note_off(i);
			CHECK(tracker.has_active_note());
			CHECK(tracker.active_note()->note == i-1);
		}
		// we release the last note it could remember
		tracker.note_off(3);
		CHECK_FALSE(tracker.has_active_note());
	}

}
