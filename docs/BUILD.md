# Building On macOS

## Prerequisites

- Xcode and its command-line tools
- CMake 3.22 or newer
- Network access on first configure so CMake can fetch JUCE 8.0.4

## Configure And Build

From the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCOMPRESSOR808BYTES_DELUXE=ON
cmake --build build --config Release --parallel
ctest --test-dir build --output-on-failure -C Release
```

Use `-DCOMPRESSOR808BYTES_DELUXE=OFF` to build the Lite development configuration. This only changes exposed tier controls; both builds use the shared DSP and stable parameter IDs.

## Generated Plug-ins

With the commands above, CMake places bundles at:

- `build/Compressor808Bytes_artefacts/Release/AU/808Bytes Compressor.component`
- `build/Compressor808Bytes_artefacts/Release/VST3/808Bytes Compressor.vst3`

The standalone development app is at `build/Compressor808Bytes_artefacts/Release/Standalone/808Bytes Compressor.app`.

## FL Studio macOS Test

1. Install and locally sign the VST3 development build:

```sh
mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3"
ditto "build/Compressor808Bytes_artefacts/Release/VST3/808Bytes Compressor.vst3" "$HOME/Library/Audio/Plug-Ins/VST3/808Bytes Compressor.vst3"
codesign --force --deep --sign - "$HOME/Library/Audio/Plug-Ins/VST3/808Bytes Compressor.vst3"
```

Ad-hoc signing is for local development only. A commercial release requires a Developer ID signature and notarization.

2. Restart FL Studio and open `Options > Manage plugins`.
3. Run `Find installed plugins`, then enable `808Bytes Compressor` in the list.
4. Add it to a mixer insert, lower Threshold, and monitor gain reduction while adjusting Ratio, Attack, Release, Mix, and Makeup.

AU is built for AU-compatible hosts and is installed by copying the component to `~/Library/Audio/Plug-Ins/Components/`. FL Studio on macOS should be tested with the VST3 build.