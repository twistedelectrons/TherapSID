#include "sid.h"
#include "armsid.h"

armsid_config_t armsidConfig;

void sendArmSidCommand(byte chip, byte cmd, byte data) {
	sid_chips[chip].send_update_immediate(31, data);
	sid_chips[chip].send_update_immediate(30, cmd);
	delay(5);
}

void signalConfig(byte chip, bool isStart) {
	for (byte i = 0; i < 3; i++) {
		sid_chips[chip].send_update_immediate(29, 0);
	}
	delay(5);
	if (isStart) {
		sid_chips[chip].send_update_immediate(31, 'D');
		sid_chips[chip].send_update_immediate(30, 'I');
		sid_chips[chip].send_update_immediate(29, 'S');
		delay(5);
	}
}

void saveConfigToFlash(byte chip) {
	sendArmSidCommand(chip, 'E', 0xcf);
	signalConfig(chip, false);
	delay(2000);
}

void saveConfigToRAM(byte chip) {
	sendArmSidCommand(chip, 'E', 0xc0);
	signalConfig(chip, false);
}

void setArm2SidBaseConfig(bool isSid) {
#define ADDRMAP_NONE 0
#define ADDRMAP_SIDL 1
#define ADDRMAP_SIDR 2
#define ADDRMAP_SFX 3
#define ADDRMAP_SIDM 4

#define EMUL_SID 0
#define EMUL_SFX 1
#define EMUL_SFX_SID 2
#define EMUL_NTSC_ADD 4

	if (isSid) {
		// SID mode, 3 SIDs
		sendArmSidCommand(0, 'M', '0' + EMUL_SID); // SID emulation only
		sendArmSidCommand(0, 'M', 'C');            // Wire mode
		sendArmSidCommand(0, 'M', 'S');            // Downmix off

		sendArmSidCommand(0, 'M', 0x80 + ADDRMAP_SIDL); // SIDL at D400
		sendArmSidCommand(0, 'M', 0x90 + ADDRMAP_SIDM); // SIDM at D420
		sendArmSidCommand(0, 'M', 0xa0 + ADDRMAP_SIDM); // SIDM at D500
		sendArmSidCommand(0, 'M', 0xb0 + ADDRMAP_SIDL); // SIDL at D520
		sendArmSidCommand(0, 'M', 0xc0 + ADDRMAP_SIDR); // SIDR at DE00
		sendArmSidCommand(0, 'M', 0xd0 + ADDRMAP_NONE); // DE20 empty
		sendArmSidCommand(0, 'M', 0xe0 + ADDRMAP_NONE); // DF00 empty
		sendArmSidCommand(0, 'M', 0xf0 + ADDRMAP_NONE); // DF20 empty
	} else {
		// SID+FM mode
		sendArmSidCommand(0, 'M', '0' + EMUL_SFX_SID); // SID & SFX emulation
		sendArmSidCommand(0, 'M', 'E');                // Wire mode
		sendArmSidCommand(0, 'M', 'S');                // Downmix off

		sendArmSidCommand(0, 'M', 0x80 + ADDRMAP_SIDL); // SIDL at D400
		sendArmSidCommand(0, 'M', 0x90 + ADDRMAP_NONE); // D420 empty
		sendArmSidCommand(0, 'M', 0xa0 + ADDRMAP_NONE); // D500 empty
		sendArmSidCommand(0, 'M', 0xb0 + ADDRMAP_NONE); // D520 empty
		sendArmSidCommand(0, 'M', 0xc0 + ADDRMAP_NONE); // DE00 empty
		sendArmSidCommand(0, 'M', 0xd0 + ADDRMAP_NONE); // DE20 empty
		sendArmSidCommand(0, 'M', 0xe0 + ADDRMAP_SFX);  // SFX at DF00
		sendArmSidCommand(0, 'M', 0xf0 + ADDRMAP_SFX);  // SFX at DF20
	}

	sendArmSidCommand(0, 'E', 'M' + 32); // Disable "Auto mono to stereo"
	sendArmSidCommand(0, 'E', 'N');      // Disable "Duplicate Left to Right"
	sendArmSidCommand(0, 'A', 0x44);     // Disable Digifix
}

/*
 * 6581 Filter Strength
 *
 * -7 to +7, Follin to Extreme
 */
void set6581FilterStrength(byte chip, signed char strength) {
	if (abs(strength) <= 7) {
		sendArmSidCommand(chip, 'E', 0x80 + (strength & 0xf));
	}
}

/*
 * 6581 Filter Low Frequency
 *
 * -1 to +1, 150 to 310 Hz
 */
void set6581FilterLow(byte chip, signed char freq) {
	if (abs(freq) <= 1) {
		sendArmSidCommand(chip, 'E', 0x90 + (freq & 0xf));
	}
}

/*
 * 8580 Filter Center Frequency
 *
 * -3 to +3, 12000 to 3000 Hz
 */
void set8580FilterCentral(byte chip, signed char freq) {
	if (abs(freq) <= 3) {
		sendArmSidCommand(chip, 'E', 0xa0 + (freq & 0xf));
	}
}

/*
 * 8580 Filter Low Frequency
 *
 * -3 to +3, 30 to 330 Hz
 */
void set8580FilterLow(byte chip, signed char freq) {
	if (abs(freq) <= 3) {
		sendArmSidCommand(chip, 'E', 0xb0 + (freq & 0xf));
	}
}

/*
 * SID Chip type
 *
 * Auto, 6581, 8580
 */
void setChipEmulation(byte chip, byte emulation) {
	byte value;
	switch (emulation) {
		case ARMSID_EMULATION_AUTO:
			value = '7';
			break;

		case ARMSID_EMULATION_6581:
			value = '6';
			break;

		case ARMSID_EMULATION_8580:
			value = '8';
			break;

		default:
			value = '7';
			break;
	}
	sendArmSidCommand(chip, 'E', value);
}

/*
 * ADSR Envelope bug fixed or not
 */
void setADSRbugFixed(byte chip, bool isFixed) { sendArmSidCommand(chip, 'E', isFixed ? 'O' + 32 : 'O'); }

/*
 * Sets the ARM2SID into FM(OPL)-mode or SID3 mode
 */
void arm2SidSetSidFmMode(bool isFM) {
	// Depending on state, this seems to be needed to get comms up
	sid_chips[0].send_update_immediate(0, 0);
	sid_chips[1].send_update_immediate(0, 0);

	signalConfig(0, true);
	setArm2SidBaseConfig(!isFM);
	saveConfigToRAM(0);
}

/*
 * Set up the selected ARMSID chip with settings from EEPROM
 *
 * The settings can be stored to RAM or flash. Storing in RAM
 * makes it possible to execute it very fast, so it can be
 * done on the fly, for example in the middle of ASID playback
 */
void armsidConfigChip(byte chip, bool save_to_flash) {
	byte chipShift = 0;
	signalConfig(chip, true);

	if (chip != 0) {
		// Second chip is in the top nibble
		chipShift = 4;
	}
	setChipEmulation(chip, (armsidConfig.emulation.raw >> chipShift) & 0x0f);
	setADSRbugFixed(chip, (armsidConfig.adsrBugFixed.raw >> chipShift) & 0x0f);
	set6581FilterStrength(chip, ((armsidConfig.filter6581strength.raw >> chipShift) & 0x0f) - 7);
	set6581FilterLow(chip, ((armsidConfig.filter6581low.raw >> chipShift) & 0x0f) - 1);
	set8580FilterCentral(chip, ((armsidConfig.filter8580central.raw >> chipShift) & 0x0f) - 3);
	set8580FilterLow(chip, ((armsidConfig.filter8580low.raw >> chipShift) & 0x0f) - 3);

	// Perform the actual config store
	if (save_to_flash) {
		// Save the settings to ARMSID flash memory
		// This will take several seconds, so should only be done
		// during startup
		saveConfigToFlash(chip);
	} else {
		// Save the settings to ARMSID RAM memory
		saveConfigToRAM(chip);
	}
}