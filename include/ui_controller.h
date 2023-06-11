#pragma once
#include "preset.h"
#include "ui_vars.h"

class UiDisplayController {
  public:
	void temp_7seg(int digit0, int digit1, int time);
	void update(int preset_number, const Preset& preset, const UiState& ui_state);
	void force_update();

  private:
	void update_leds(const Preset& p, const UiState& ui_state);
	void show_changed(int value);
	void show_arp_mode(int arp_mode);
	void update_7seg(int preset_number, const Preset& preset, const UiState& ui_state);
	void set_led(int index, bool value);

	bool leds[32] = {false};

	Preset old_preset;

	int last_changed_counter = 0;
	int last_changed_digit0 = 99, last_changed_digit1 = 99;
	int old_digit0 = -1, old_digit1 = -1;
	int old_preset_number = -1;
};
