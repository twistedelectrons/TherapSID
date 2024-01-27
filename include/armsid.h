#pragma once
extern void armsidConfigChip(byte chip, bool save_to_flash);
extern void arm2SidSetSidFmMode(bool isFM);

// Raw representation, one SID in each nibble
#define ARMSID_SETTING_RAW(value) (value + (value << 4))

#define ARMSID_EMULATION_AUTO 0
#define ARMSID_EMULATION_6581 1
#define ARMSID_EMULATION_8580 2

#define ARMSID_6581_FILTER_STRENGTH_DEFAULT 7
#define ARMSID_6581_FILTER_LOW_DEFAULT 1
#define ARMSID_8580_FILTER_CENTRAL_DEFAULT 3
#define ARMSID_8580_FILTER_LOW_DEFAULT 3

typedef union {
	struct {
		byte sid2 : 4;
		byte sid1 : 4;
	};
	byte raw;
} armsid_setting_t;

typedef struct {
	armsid_setting_t emulation;
	armsid_setting_t adsrBugFixed;
	armsid_setting_t filter6581strength;
	armsid_setting_t filter6581low;
	armsid_setting_t filter8580central;
	armsid_setting_t filter8580low;
} armsid_config_t;

extern armsid_config_t armsidConfig;
