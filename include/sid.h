#pragma once
void sidReset();
void init1MhzClock();
void sidDelay();
void sidSend(byte address, byte data);
void sidSend1Only(byte address, byte data);
void sidSend2(byte address, byte data);
void sidUpdate();
void sidPitch(byte voice, int pitch);
void sidShape(byte voice, byte shape, bool value);
void updateFilter();
void calculatePitch();
void updateFatMode();
void setFilterBit(byte channel);
