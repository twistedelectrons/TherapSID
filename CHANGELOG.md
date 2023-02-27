# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased 2.1] - TODO

### Added
- 2 and 6 voice polyphonic modes (via F5 fat mode).
- 3-voice 2-operator polyphonic mode (via F6 fat mode in paraphonic mode).

### Fixed

### Changed

### Removed

## [Unreleased 2.0] - 2023-02-14

### Fixed

- Fixed audio crackling when changing filter mode.
- Fixed audio crackling when changing presets.
- Arp scrub and random mode now use all notes (instead of leaving out the last).
- UI LED bugs.
- Releasing a channel2/3/4 override note will fall back to the "main" channel1 note.
- Applying aftertouch does no longer set the UI's current lfo to 1.

### Changed

- Replace MIDI library by own implementation.
- Constrain pulse width range so it cannot be set to 0 any more.
- Paraphonic mode no longer overrides voice2 and voice3's settings in the preset.
- Paraphonic voice allocation: Round-robin allocator which prefers released voices
  over active ones.
- Monophonic voice allocation: Always play the most recently pressed note. (And
  update this also when releasing notes.)
- Always switch back to displaying the current preset's number after some time.
- Probably reduced the SID parameter update rate.

### Removed

- MIDI CCs 60-67 no longer control the active LFO's settings.

## [1.9 (platformio edition)] - 2022-10-21

### Changed

- Ported code from Arduino IDE to platformio
- Probably changed versions of used libraries
- `clang-format` treatment.
