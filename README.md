# mufferfluffer

Seven effects modelled on the documented behaviour of a family of analogue effect pedals, as VST3 plug-ins and VCV Rack 2 modules. This project is not affiliated with, endorsed by or derived from the products of any manufacturer.

The plug-ins and the modules share one DSP core written in C++17. The effects are Fluff Lowpass, Fluff Ring, Fluff Phase, Fluff Delay, Fluff Bank, Fluff Osc and Fluff Swarm. `docs/USER_GUIDE.md` describes every control, input and output.

This is a personal, non-commercial hobby project. VST is a registered trademark of Steinberg Media Technologies GmbH.

## Build

You need CMake 3.20 or later and a C++17 compiler. The reference build image is Ubuntu 24.04 (glibc 2.39).

The core and its tests build on any platform:

```
cmake -S . -B build/core -DCMAKE_BUILD_TYPE=Release
cmake --build build/core --config Release --parallel
ctest --test-dir build/core -C Release --output-on-failure
```

The VST3 plug-ins need JUCE 9.0.5, which CMake fetches by commit. On Ubuntu, first install
`libasound2-dev libxrandr-dev libxinerama-dev libxcursor-dev libglu1-mesa-dev libxcomposite-dev libxi-dev libfreetype-dev libfontconfig1-dev libx11-dev libxext-dev libxrender-dev ninja-build xvfb`.
On macOS, add `-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0` to the first command so that the bundles load on macOS 11 and later.

```
cmake -S . -B build/vst3 -DCMAKE_BUILD_TYPE=Release -DMUFF_BUILD_VST3=ON
cmake --build build/vst3 --config Release --parallel
ctest --test-dir build/vst3 -C Release --output-on-failure -R muff_vst3_tests
```

The VCV Rack plug-in needs the Rack SDK 2.6.6 (and `jq` and `zstd`). The script fetches it for your platform
(`lin-x64`, `win-x64`, `mac-x64` or `mac-arm64`) and checks it against the hashes in `deps.lock.json`. On Windows, build in an MSYS2 MINGW64 shell.

```
python3 tools/fetch_rack_sdk.py lin-x64 .
make -C rack RACK_DIR="$PWD/Rack-SDK" dist
```

The package is `rack/dist/Mufferfluffer-2.0.0-<platform>.vcvplugin`.

## Install

No binaries are published, so build both formats as `## Build` describes, then copy the results.

- VST3 bundles go in `~/.vst3/` (Linux), `~/Library/Audio/Plug-Ins/VST3/` (macOS) or `C:\Program Files\Common Files\VST3\` (Windows), the standard locations of the VST3 format.
- The `.vcvplugin` file goes in the `plugins-<os>-<cpu>` folder of the Rack user folder, which Rack opens from its menu Help > Open user folder. Restart Rack afterwards.

Three notes on compatibility:

- Linux: the binaries are built on Ubuntu 24.04, so they need glibc 2.39 or later.
- Windows: the Microsoft Visual C++ redistributable must be installed, because the runtime is linked dynamically.
- macOS: the bundles are not signed or notarised. On a bundle that you have downloaded rather than built, remove the quarantine attribute with `xattr -dr com.apple.quarantine <bundle>`.

## Tests

```
python3 tools/check_frozen.py
python3 -m unittest discover -s tests/py -v
ctest --test-dir build/core -C Release --output-on-failure
build/core/tests/muff_bench --limit 5
ctest --test-dir build/vst3 -C Release --output-on-failure -R muff_vst3_tests
```

`check_frozen.py` confirms that the frozen test files are unchanged. The Python tests check naming, the Rack manifest and panels, the
dependency pins, workflow hygiene, licence headers and the documentation. The core suite has 98 cases and the VST3 shell suite has 7. The benchmark is a
timing gate for Linux x64; use `--limit 0` elsewhere. Continuous integration runs all of these on Linux, Windows and macOS and
scans the pinned dependencies for known advisories.

## Licence

AGPL-3.0-only. See `LICENSE`.
