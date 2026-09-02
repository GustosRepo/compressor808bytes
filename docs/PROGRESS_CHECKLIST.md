# 808Bytes Compressor Progress

## Phase 1: Core Compressor - Complete

- [x] JUCE/CMake macOS AU and VST3 scaffold
- [x] Shared Phase 1 stereo compressor DSP architecture
- [x] APVTS stable parameter IDs and state serialization
- [x] Development UI controls and atomically published meters
- [x] Deluxe UI controls for knee, sidechain HPF, detector mode, character, and oversampling
- [x] DSP tests: ratio, threshold, knee, linked stereo, HPF stability, and finite output
- [x] DSP tests for hybrid detector modes, program-dependent release, character, and oversampling
- [x] Configure, build, and verify generated AU/VST3 bundles
- [x] Run DSP tests at 44.1, 48, and 96 kHz across common buffer sizes
- [x] Launch standalone development build
- [x] Test VST3 in FL Studio on macOS

## Deferred Work

- [ ] Add factory preset browser and preset library
- [ ] Implement auto gain
- [ ] Finalize original weathered-hardware Deluxe visual identity
