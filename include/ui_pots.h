#pragma once
void movedPot(byte number, int value, bool isMidi);

enum class Pot {
	PW1 = 4,
	TUNE1 = 6,
	FINE1 = 14,
	GLIDE1 = 1,
	ATTACK1 = 5,
	DECAY1 = 15,
	SUSTAIN1 = 13,
	RELEASE1 = 16,

	PW2 = 24,
	TUNE2 = 26,
	FINE2 = 17,
	GLIDE2 = 27,
	ATTACK2 = 22,
	DECAY2 = 25,
	SUSTAIN2 = 23,
	RELEASE2 = 20,

	PW3 = 30,
	TUNE3 = 21,
	FINE3 = 31,
	GLIDE3 = 19,
	ATTACK3 = 29,
	DECAY3 = 18,
	SUSTAIN3 = 28,
	RELEASE3 = 3,

	LFO_RATE1 = 11,
	LFO_DEPTH1 = 12,
	LFO_RATE2 = 10,
	LFO_DEPTH2 = 9,
	LFO_RATE3 = 36,
	LFO_DEPTH3 = 2,

	CUTOFF = 8,
	RESONANCE = 0,
	ARP_SCRUB = 7,
	ARP_RATE = 41,
	ARP_RANGE = 32
};
