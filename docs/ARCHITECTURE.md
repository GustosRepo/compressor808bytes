# 808Bytes Compressor Architecture

## Signal Flow

`Input gain -> dry tap -> linked RMS detector -> dB gain computer -> makeup gain -> parallel mix -> output gain`.

The detector sidechain is high-pass filtered before level detection only. The audible path is never filtered. Stereo channels share one averaged-power RMS envelope and the same gain reduction, preserving stereo imaging.

## DSP

`CompressorEngine` has no GUI or parameter-tree dependency. `EnvelopeDetector` smooths the squared linked detector signal with attack/release coefficients derived from the active sample rate, then converts it to RMS amplitude. The gain computer works in dB and supports hard or continuously soft knees. All audio parameters are smoothed in the processor before reaching the engine.

## Parameters

Stable IDs: `input`, `threshold`, `ratio`, `attack`, `release`, `makeup`, `mix`, `output`, `knee`, `sidechainHPF`, `detectorMode`, `autoGain`, `character`, `oversampling`, `bypass`.

Phase 1 ranges: input -24 to +24 dB; threshold -60 to 0 dB; ratio 1:1 to 20:1; attack 0.1 to 100 ms; release 10 to 2000 ms; makeup -12 to +24 dB; mix 0 to 100%; output -24 to +12 dB; knee 0 to 24 dB; sidechain HPF 20 to 500 Hz.

## Tiers And Roadmap

One processor uses `PluginTier` and the `COMPRESSOR808BYTES_DELUXE` CMake option. Lite exposes the professional core controls and gain-reduction meter. Deluxe additionally exposes knee, detector high-pass, detector mode, character, and oversampling. Future Deluxe work may implement the already-reserved auto gain parameter, advanced meters, and expanded presets without changing preset IDs. Licensing and product activation are intentionally out of scope.
