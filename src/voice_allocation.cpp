#include "voice_allocation.hpp"

MonoNoteTracker<16> mono_note_tracker;
MonoNoteTracker<16> mono_note_trackers[3];
PolyVoiceAllocator<3> voice_allocator;
