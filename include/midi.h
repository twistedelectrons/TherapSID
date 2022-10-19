#pragma once
void readMidi();
void sendCC(byte number, int value);
void sendMidiButt(byte number, int value);
void HandleNoteOn(byte channel, byte note, byte velocity);
void HandleControlChange(byte channel, byte data1, byte data2);
void handleBend(byte channel, int value);
void midiOut(byte note);
void sendDump();
void recieveDump();
void pedalUp();
void pedalDown();
