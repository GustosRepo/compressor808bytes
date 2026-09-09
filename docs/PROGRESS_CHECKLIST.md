# 808Bytes FET Compressor Progress

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

## Phase 2: 76/FET Direction - Started

- [x] Retune attack range for sub-millisecond FET-style response
- [x] Retune release range for faster 76-style recovery
- [x] Make detector modes peak-led with Vintage and Fast behavior
- [x] Limit knee range for harder FET gain reduction
- [x] Add aggressive all-buttons-style gain curve
- [x] Add stepped ratio-button UI for 4:1, 8:1, 12:1, 20:1, and All
- [x] Rename character modes toward Transformer and FET Push
- [x] Add DSP coverage for sparse transient detector behavior
- [x] Add DSP coverage for separate 20:1 and all-buttons limiting behavior

## Deferred Work

- [ ] Add factory preset browser and preset library
- [ ] Implement auto gain
- [ ] Replace lightweight nonlinear oversampling with proper anti-aliased oversampling
- [ ] Add processor-level tests for wet/dry mix, bypass, makeup gain, and meter correctness
- [ ] Tune attack/release/character against reference drum, bass, and vocal material
- [ ] Finalize original weathered-hardware Deluxe visual identity
