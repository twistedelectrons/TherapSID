#include "3rdparty/catch_amalgamated.hpp"
#include "voice_state.hpp"

TEST_CASE("Glide") {
	Glide glide;

	SECTION("eventually reaches destination pitch with glide factor 10") {
		for (int destination_pitch : { 6577, 163, 44242, 1742, 3910, 59056, 871 }) {
			glide.destination_pitch = destination_pitch;
			for (int i=0; i<1100; i++) {
				glide.glide_tick(10);
			}
			CHECK(abs(glide.current_pitch() - destination_pitch) <= 1);
		}
	}
	SECTION("eventually reaches destination pitch with glide factor 100") {
		for (int destination_pitch : { 6577, 163, 44242, 1742, 3910, 59056, 871 }) {
			glide.destination_pitch = destination_pitch;
			for (int i=0; i<110000; i++) {
				glide.glide_tick(100);
			}
			CHECK(abs(glide.current_pitch() - destination_pitch) <= 1);
		}
	}
	SECTION("instantly reaches destination pitch with glide factor 0") {
		for (int destination_pitch : { 6577, 163, 44242, 1742, 3910, 59056, 871 }) {
			glide.destination_pitch = destination_pitch;
			glide.glide_tick(0);
			CHECK(abs(glide.current_pitch() - destination_pitch) <= 1);
		}
	}
}
