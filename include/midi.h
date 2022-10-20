#pragma once
void sendMidiButt(byte number, int value);
void sendCC(byte number, int value);
void sendControlChange(byte number, byte value);
void sendNoteOff(byte note, byte velocity, byte channel);
void sendNoteOn(byte note, byte velocity, byte channel);
void midiRead();
void HandleNoteOn(byte channel, byte note, byte velocity);
void HandleControlChange(byte channel, byte data1, byte data2);
void handleBend(byte channel, int value);
void midiOut(byte note);
void sendDump();
void recieveDump();
void pedalUp();
void pedalDown();
