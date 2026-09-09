# 808Bytes FET Compressor Architecture

## Signal Flow

`Input gain -> dry tap -> one-sample feedback FET gain-reduction loop -> FET/transformer character stage -> parallel mix -> output gain`.

The detector sidechain is high-pass filtered before level detection only. The audible path is never filtered. Stereo channels share one linked envelope and the same gain reduction, preserving stereo imaging. The sidechain listens to the previous post-gain-reduction sample, giving the compressor a causal feedback topology closer to the 1176 than the earlier feed-forward detector.

## DSP

`CompressorEngine` has no GUI or parameter-tree dependency. The current voicing is now 76/FET-inspired rather than a neutral VCA compressor: detection is peak-led, attack timing supports sub-millisecond operation, release is quicker and depth-dependent, the knee is intentionally limited, and the gain law is calibrated for a feedback sidechain instead of a feed-forward threshold computer.

The ratio buttons select 4:1, 8:1, 12:1, 20:1, and `ALL`. Higher ratios apply a small ratio-dependent threshold offset, matching the 1176 behavior where ratio selection changes threshold relationship. `ALL` shifts the internal calibration, slows the attack response into transients, extends release slightly, and increases FET-stage asymmetry/distortion.

The visible 76-style workflow is `Input`, ratio buttons, `Attack`, `Release`, and `Output`. `Input` is the drive control: turning it up increases the level entering the gain-reduction loop and therefore increases compression. The old `threshold` parameter ID remains in the state for compatibility, but it is treated as a hidden FET calibration trim around a fixed internal operating point.

The Deluxe detector mode is interpreted as `Vintage` or `Fast`. Both modes remain peak-led; `Fast` uses a stronger peak blend and quicker transient capture. The character stage provides `Transformer` and `FET Push` drive colors using asymmetric tanh shaping after gain reduction, with a small low-band emphasis and high-band shave before the nonlinear stage. Output gain is applied after the feedback sidechain tap, so it does not increase compression amount. This is still an inspired digital model, not a component-level recreation of a specific hardware unit.

The center meter follows a 1176-style workflow with editor-local `GR`, `+4`, and `+8` modes. `GR` displays gain reduction on a reversed 0 to 20 dB scale. `+4` and `+8` display output level on a VU-style -20 to +3 scale using fixed digital calibration offsets for practical plugin metering.

## UI Layout

The editor uses a fixed 1120x760 hardware canvas and scales it uniformly into the host-provided editor bounds. Resizing preserves the canvas aspect ratio through the JUCE editor constrainer; if a host ignores that ratio, the canvas is centered with unused margin instead of stretching controls independently. This keeps the 76-style meter, knobs, ratio buttons, and bottom rail in stable proportions across window sizes.

The visual direction is now a clean modern hardware panel: light aluminum main surface, dark technical meter displays, flat ratio/meter buttons, restrained accent colors, and minimal wear. The goal is an original interface informed by modern portable instruments and digitally controlled analog hardware, not a copy of any specific product.

## Parameters

Stable IDs: `input`, `threshold`, `ratio`, `attack`, `release`, `makeup`, `mix`, `output`, `knee`, `sidechainHPF`, `detectorMode`, `autoGain`, `character`, `oversampling`, `bypass`.

Current 76-style ranges: input -24 to +24 dB; hidden FET calibration -60 to 0 dB; ratio buttons 4:1, 8:1, 12:1, 20:1, and ALL; attack 0.02 to 0.8 ms; release 50 to 1100 ms; output -24 to +24 dB; mix 0 to 100%; hidden output trim -24 to +12 dB; knee 0 to 8 dB; sidechain HPF 20 to 500 Hz.

## Tiers And Roadmap

One processor uses `PluginTier` and the `COMPRESSOR808BYTES_DELUXE` CMake option. Lite exposes the core 76-style controls and gain-reduction meter. Deluxe additionally exposes knee, detector high-pass, detector mode, character, and oversampling.

Open DSP work: replace the lightweight interpolation-based nonlinear oversampling with a proper anti-aliased oversampling path, implement the already-reserved auto gain parameter, and tune attack/release/character against reference material.
