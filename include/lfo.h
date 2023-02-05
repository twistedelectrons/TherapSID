#pragma once
struct ParamsAfterLfo {
	int resonance; // 0..15
	int cutoff; // 0..1023
	int pulsewidth[3]; // 0..2048
};

void setLfo(byte number);
ParamsAfterLfo lfoTick();
void lastMovedPot(byte number);
void chain();
void clearLfo();
