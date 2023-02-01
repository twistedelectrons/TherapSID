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

		preset_data.voice[1].pulsewidth_base = preset_data.voice[2].pulsewidth_base = preset_data.voice[0].pulsewidth_base;
		preset_data.voice[1].tune_base = preset_data.voice[2].tune_base = preset_data.voice[0].tune_base;
		preset_data.voice[1].fine_base = preset_data.voice[2].fine_base = preset_data.voice[0].fine_base;
		preset_data.voice[1].glide = preset_data.voice[2].glide = preset_data.voice[0].glide;
		preset_data.voice[1].attack = preset_data.voice[2].attack = preset_data.voice[0].attack;
		preset_data.voice[1].decay = preset_data.voice[2].decay = preset_data.voice[0].decay;
		preset_data.voice[1].release = preset_data.voice[2].release = preset_data.voice[0].release;
		preset_data.voice[1].sustain = preset_data.voice[2].sustain = preset_data.voice[0].sustain;
		sid[12] = 255 & preset_data.voice[1].attack << 4;
		sid[12] = sid[12] | preset_data.voice[1].decay;
		sid[19] = 255 & preset_data.voice[2].attack << 4;
		sid[19] = sid[19] | preset_data.voice[2].decay;
		preset_data.voice[1].attack = preset_data.voice[2].attack = preset_data.voice[0].attack;
		preset_data.voice[1].decay = preset_data.voice[2].decay = preset_data.voice[0].decay;
		preset_data.voice[1].release = preset_data.voice[2].release = preset_data.voice[0].release;
		preset_data.voice[1].sustain = preset_data.voice[2].sustain = preset_data.voice[0].sustain;
		sid[13] = 255 & preset_data.voice[1].sustain << 4;
		sid[13] = sid[13] | preset_data.voice[1].release;
		sid[20] = 255 & preset_data.voice[2].sustain << 4;
		sid[20] = sid[20] | preset_data.voice[2].release;
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
