#pragma once
#include "preset.h"
#include "ui_vars.h"

static const int POT_NONE = 20;
static const int DONTCARE = 123;

class UiDisplayController {
  public:
	void temp_7seg(int digit0, int digit1, int time);
	void update(int preset_number, const Preset& preset, const UiState& ui_state, byte turboMidiXrate);
	void force_update();

  private:
	void update_leds(const Preset& p, const UiState& ui_state);
	void show_changed(int value);
	void show_byte(byte value, bool center);
	void show_filterSetupOffset(byte chip);
	void show_filterSetupRange(byte chip);
	void show_arp_mode(int arp_mode);
	void update_7seg(int preset_number, const Preset& preset, const UiState& ui_state, byte turboMidiXrate);
	void set_led(int index, bool value);

	bool leds[32] = {false};

	Preset old_preset;

	int last_changed_counter = 0;
	int last_changed_digit0 = 99, last_changed_digit1 = 99;
	int old_digit0 = -1, old_digit1 = -1;
	int old_preset_number = -1;
	byte old_turbomidi_x_rate = 1;
};
