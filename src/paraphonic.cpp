#include "globals.h"
#include "preset.h"
#include "sid.h"
#include "leds.h"
#include "paraphonic.h"
#include "voice_allocation.hpp"

/*
void paraChange() {

	if (preset_data.paraphonic) {
		updateFilter();

		arpMode = 0;
		arpCount = 0;
		held = 0;
		mono_note_tracker.clear();
		for (int i = 0; i < 3; i++)
			mono_note_trackers[i].clear();
		bitWrite(sid[4], 0, 0);
		bitWrite(sid[11], 0, 0);
		bitWrite(sid[18], 0, 0);
		sidSend(4, sid[4]);
		sidSend(11, sid[11]);
		sidSend(18, sid[18]);

		digit(0, 14);
		digit(1, 17);

		lfoFine1 = lfoFine2 = lfoFine3 = lfoFine4 = lfoFine5 = lfoFine6 = lfoFine7 = lfoFine8 = lfoFine9 = 0;
		lfoTune1 = lfoTune2 = lfoTune3 = lfoTune4 = lfoTune5 = lfoTune6 = lfoTune7 = lfoTune8 = lfoTune9 = 0;

		pw2Base = pw3Base = pw1Base;
		tuneBase2 = tuneBase3 = tuneBase1;
		fineBase2 = fineBase3 = fineBase1;
		glide2 = glide3 = glide1;
		a2 = a3 = a1;
		d2 = d3 = d1;
		r2 = r3 = r1;
		s2 = s3 = s1;
		sid[12] = 255 & a2 << 4;
		sid[12] = sid[12] | d2;
		sid[19] = 255 & a3 << 4;
		sid[19] = sid[19] | d3;
		a2 = a3 = a1;
		d2 = d3 = d1;
		r2 = r3 = r1;
		s2 = s3 = s1;
		sid[13] = 255 & s2 << 4;
		sid[13] = sid[13] | r2;
		sid[20] = 255 & s3 << 4;
		sid[20] = sid[20] | r3;
		paraShape();
	} else {
		bitWrite(sid[4], 0, 0);
		bitWrite(sid[11], 0, 0);
		bitWrite(sid[18], 0, 0);
		sidSend(4, sid[4]);
		sidSend(11, sid[11]);
		sidSend(18, sid[18]);

		voice_allocator.clear();
		digit(0, 99);
		digit(1, 99);
		load(preset);
	}
}

void paraShape() {
	if (preset_data.paraphonic) {
		sid[11] = sid[18] = sid[4];

		sidShape(1, 1, bitRead(sid[4], 6));
		sidShape(1, 2, bitRead(sid[4], 4));
		sidShape(1, 3, bitRead(sid[4], 5));
		sidShape(1, 4, bitRead(sid[4], 7));

		sidShape(2, 1, bitRead(sid[4], 6));
		sidShape(2, 2, bitRead(sid[4], 4));
		sidShape(2, 3, bitRead(sid[4], 5));
		sidShape(2, 4, bitRead(sid[4], 7));
	}
}
*///FIXME Obviously
