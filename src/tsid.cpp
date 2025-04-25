#include "globals.h"
#include "display.h"
#include "midi.h"
#include "mux.h"
#include "boot.h"
#include "isr.h"
#include "sid.h"
#include "ui_leds.h"
#include "asid.h"
#include "armsid.h"

#include <digitalWriteFast.h>
#include <EEPROM.h>
#include <LedControl.h>

#include <TimerOne.h>

// Traverse through the EEPROM list and set the default value
void setDefaultEEPROMvalue(byte reg) {
	for (int i = 0; i < (int)(sizeof(globalSettings) / sizeof(globalSetting)); i++) {
		const globalSetting* setting = &(globalSettings[i]);
		if (setting->eepromAddress == reg) {
			EEPROM.update(setting->eepromAddress, setting->defaultValue);
		}
	}
}

void setup() {
	Serial.begin(31250);

	preset_data.arp_speed_base = 100;

	// let sid chip wake up
	DDRD |= _BV(2); // SID1
	DDRD |= _BV(6); // SID2
	DDRD |= _BV(3); // LATCH

	DDRB = 255; // data port

	// SID address port and CV switches
#if SIDCHIPS > 2
	DDRC = B11111100;
#else
	DDRC = B11111000;
#endif

	// get the clock going for the SID
	init1MhzClock();

	pinMode(A6, OUTPUT); // sid reset
	sidReset();

	pinMode(A0, INPUT);     // mux inputs
	pinMode(A1, INPUT);     // mux inputs
	pinMode(A2, INPUT);     // mux inputs
	pinMode(A3, INPUT);     // butt inputs
	pinMode(A4, INPUT);     // butt inputs
	digitalWrite(A3, HIGH); // pullup
	digitalWrite(A4, HIGH); // pullup

	pinMode(16, INPUT);
	digitalWrite(16, HIGH); // CV switch1
	pinMode(17, INPUT);
	digitalWrite(17, HIGH); // CV switch2
#if SIDCHIPS > 2
	pinMode(18, OUTPUT); // SID3 CS (ARM2SID)
#else
	pinMode(18, INPUT);
	digitalWrite(18, HIGH); // CV switch3
#endif

	pinMode(A7, INPUT);
	digitalWrite(A7, HIGH); // gate

	// turns on display
	mydisplay.shutdown(0, false);
	mydisplay.setIntensity(0, 1); // 15 = brightest

	// are we holding preset buttons at startup?
	mux(9);
	delay(5);
	if ((PINA & _BV(4)) == 0) {
		// preset up
		sendDump();
	} else {
		mux(3);
		delay(5);
		if ((PINA & _BV(4)) == 0) {
			// preset down
			recieveDump();
		}
	}

	showVersion();

	boot();
	delay(500);

	Timer1.initialize(100);      //
	Timer1.attachInterrupt(isr); // attach the service routine here

	// Validate EEPROM memory structure
	uint16_t value;
	EEPROM.get(EEPROM_ADDR_COOKIE, value);
	bool initialize_memory = (value != EEPROM_COOKIE_VALUE);

	// Initialize EEPROM memory if needed
	if (initialize_memory) {
		EEPROM.put(EEPROM_ADDR_COOKIE, EEPROM_COOKIE_VALUE);
		EEPROM.put(EEPROM_ADDR_VERSION, EEPROM_FORMAT_VERSION_V1);

		// Write default values for all global settings
		for (int i = 0; i < (int)(sizeof(globalSettings) / sizeof(globalSetting)); i++) {
			const globalSetting* setting = &(globalSettings[i]);
			EEPROM.update(setting->eepromAddress, setting->defaultValue);
		}
	}

	// Run EEPROM format upgrades when needed
	EEPROM.get(EEPROM_ADDR_VERSION, value);
	if (value == EEPROM_FORMAT_VERSION_V1) {
		// Convert from V1 to V2:

		// Fill in default values on filter enabled (all three channels on)
		for (byte i = PRESET_NUMBER_MIN; i <= PRESET_NUMBER_MAX; i++) {
			EEPROM.update(EEPROM_ADDR_PRESET_EXTRA_BYTE(i), 0x07);
		}
		value = EEPROM_FORMAT_VERSION_V2;
		EEPROM.put(EEPROM_ADDR_VERSION, value);
	}
	if (value == EEPROM_FORMAT_VERSION_V2) {
		// Convert from V2 to V3:

		// New values
		byte newRegsV3[] = {EEPROM_ADDR_PITCH_BEND_UP,          EEPROM_ADDR_PITCH_BEND_DOWN,
		                    EEPROM_ADDR_NO_ARP_1_KEY,           EEPROM_ADDR_ARMSID_CHIP_EMULATION,
		                    EEPROM_ADDR_ARMSID_ADSR_BUG_FIXED,  EEPROM_ADDR_ARMSID_6581_FILTER_STRENGTH,
		                    EEPROM_ADDR_ARMSID_6581_FILTER_LOW, EEPROM_ADDR_ARMSID_8580_FILTER_CENTRAL,
		                    EEPROM_ADDR_ARMSID_8580_FILTER_LOW};

		for (byte r = 0; r < sizeof(newRegsV3) / sizeof(*newRegsV3); r++) {
			setDefaultEEPROMvalue(newRegsV3[r]);
		}

		value = EEPROM_FORMAT_VERSION_V3;
		EEPROM.put(EEPROM_ADDR_VERSION, value);
	}
	if (value == EEPROM_FORMAT_VERSION_V3) {
		// Convert from V3 to V4:

		// New values
		byte newRegsV4[] = {EEPROM_ADDR_SID1_OFFSET, EEPROM_ADDR_SID1_RANGE,  EEPROM_ADDR_SID2_OFFSET,
		                    EEPROM_ADDR_SID2_RANGE,  EEPROM_ADDR_SID3_OFFSET, EEPROM_ADDR_SID3_RANGE};

		for (byte r = 0; r < sizeof(newRegsV4) / sizeof(*newRegsV4); r++) {
			setDefaultEEPROMvalue(newRegsV4[r]);
		}

		value = EEPROM_FORMAT_VERSION_V4;
		EEPROM.put(EEPROM_ADDR_VERSION, value);
	}

	// are we holding reset (to reset globals)?
	mux(7);
	delay(5);
	bool initialize_globals = ((PINA & _BV(4)) == 0);
	if (initialize_globals) {
		digit(0, DIGIT_D);
		digit(1, DIGIT_F);
		delay(500);
	}

	// Are we holding LOOP (to flash ARMSID settings)?
	mux(1);
	delay(5);
	bool flash_armsid = ((PINA & _BV(4)) == 0);
	if (flash_armsid) {
		digit(0, DIGIT_F);
		digit(1, DIGIT_A);
	}

	// Update all global settings from EEPROM memory
	for (int i = 0; i < (int)(sizeof(globalSettings) / sizeof(globalSetting)); i++) {
		const globalSetting* setting = &(globalSettings[i]);
		byte value = EEPROM.read(setting->eepromAddress);

		// Validate range and set default if needed
		if ((value < setting->minValue) || (value > setting->maxValue) || initialize_globals) {
			value = setting->defaultValue;
			EEPROM.update(setting->eepromAddress, value);
		}

		// Update the actual global setting variable
		*(byte*)setting->variable = value;
	}

	volumeChanged = true;

	setupMux();
	midiInit();

	sidReset(); // prevent sustained Note at startup

	// Sets up ARMSIDs or ARM2SID chips if available
	arm2SidSetSidFmMode(false);
	armsidConfigChip(0, flash_armsid);
	armsidConfigChip(1, flash_armsid);

	// Setup ASID without enabling it
	asidState.enabled = false;
	asidState.isSidFmMode = false;
	asidInit(-1);
}
