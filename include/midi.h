#pragma once
void sendMidiButt(byte number, int value);
void sendCC(byte number, int value);
void sendControlChange(byte number, byte value);
void sendNoteOff(byte note, byte velocity, byte channel);
void sendNoteOn(byte note, byte velocity, byte channel);
void midiRead();
void midiOut(byte note);
void sendDump();
void recieveDump();
