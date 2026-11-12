# Environment and pins

Every version below was read from its source on 2026-11-11 or 2026-11-12 (claims C-PLANNER-001 to C-PLANNER-019). "Verified here" means it was exercised in the planning sandbox.

## Dependencies
| Item | Pin | How pinned | Verified here |
|---|---|---|---|
| JUCE | 9.0.5, commit `d60b7a78030661574a615b586d9325ba7a7fa7dd` | `FetchContent` `GIT_TAG` in `vst3/CMakeLists.txt` | configure, build and tests (spikes `juce-vst3-build`, `red-verification`) |
| Catch2 | v3.16.1, commit `08092139210881f01a34e60c5f2fd6afa0ea4024` | `FetchContent` `GIT_TAG` in `tests/CMakeLists.txt` | yes |
| VCV Rack SDK | 2.6.6 (Rack tag `v2.6.6` = `061ccf63c1758599396ac1bb10d47345d9d34076`) | archive URL of D-003 + SHA-256 in `deps.lock.json` (recorded by S-030) | headers only (cloned tag); archive not reachable from the sandbox |
| pluginval | v1.0.4 (tag commit `ed19c2c16b57a6d94db391bea3ef4a80b769d5bf`) | release archive + SHA-256: Linux `c01c49d8063965c4c2dea8324468336768f5c9139e0b1caebde14c2400b55352`, macOS `3c4c533bda0c5059eea3ddaea752d757ee2025041f0f47e6bcb0e87f6082b29f`, Windows `c08e61ce3b96db41636f8ec7e76f4c7e2c13ebdac7fa1b5a1f52b4f32ec715ab` | no (hashes from the knowledge store; the download step verifies them, so a wrong hash stops the job) |
| Mull | package `mull-19` from the `mull-stable` Cloudsmith repository, with `clang-19` | version recorded by S-MUTATE.1 in `deps.lock.json` | no |
| Python | 3.10 or newer, standard library only | runner default | 3.13 here |
| CMake | 3.22 or newer | `cmake_minimum_required` | 3.28.3 here |

Release archive URLs: pluginval `https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_<Linux|macOS|Windows>.zip`; Rack SDK `https://vcvrack.com/downloads/Rack-SDK-2.6.6-<lin-x64|win-x64|mac-x64|mac-arm64>.zip`.

## GitHub Actions
| Action | Version | Commit to use in `uses:` |
|---|---|---|
| `actions/checkout` | v7.0.1 | `3d3c42e5aac5ba805825da76410c181273ba90b1` |
| `actions/upload-artifact` | v7.0.2 | `cf430e030ddbb5b0abf93d22962f4752f3646cd9` |
| `actions/download-artifact` | v8.0.2 | `9000827ccba6bdab643e8b6fd33ac0654aef8333` |
| `actions/cache` | v6.1.0 | `55cc8345863c7cc4c66a329aec7e433d2d1c52a9` |
| `msys2/setup-msys2` | v2.33.0 | `2ff06a04f6edec2a4d4bad38c3bfb2df71f4b9f2` |

Runner labels (C-PLANNER-010, C-PLANNER-011): `ubuntu-24.04`, `ubuntu-22.04`, `ubuntu-24.04-arm`, `windows-2022`, `macos-15`, `macos-15-intel`. Never a `-latest` label.

## Linux packages
- JUCE VST3 build and tests (Ubuntu 24.04; run `sudo apt-get update` first): `libasound2-dev libxrandr-dev libxinerama-dev libxcursor-dev libglu1-mesa-dev libxcomposite-dev libxi-dev libfreetype-dev libfontconfig1-dev libx11-dev libxext-dev libxrender-dev ninja-build xvfb`.
- Rack plug-in build (Ubuntu): `build-essential git jq zip unzip curl`. Windows (MSYS2 MINGW64, via `msys2/setup-msys2` with `msystem: MINGW64`): `git make tar unzip zip mingw-w64-x86_64-gcc mingw-w64-x86_64-jq python zstd`. macOS: Xcode command-line tools of the runner image plus `brew install jq` if `jq` is absent.

## Commands (exact)
```
# core + adaptor tests (any platform)
cmake -S . -B build/core -DCMAKE_BUILD_TYPE=Release
cmake --build build/core --config Release --parallel
ctest --test-dir build/core -C Release --output-on-failure        # runs every T-0xx..T-8xx case
build/core/tests/muff_bench --limit 5                             # Linux x64 gate; --limit 0 elsewhere (Windows: build/core/tests/Release/muff_bench.exe)

# sanitizers (Linux, GCC)
cmake -S . -B build/asan -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMUFF_SANITIZER_BUILD=ON \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build/asan --parallel && build/asan/tests/muff_core_tests

# VST3 plug-ins + shell tests
cmake -S . -B build/vst3 -DCMAKE_BUILD_TYPE=Release -DMUFF_BUILD_VST3=ON
cmake --build build/vst3 --config Release --parallel
ctest --test-dir build/vst3 -C Release --output-on-failure -R muff_vst3_tests

# repository tests and frozen-file check
python3 tools/check_frozen.py
python3 -m unittest discover -s tests/py -v

# Rack plug-in (SDK unpacked to ./Rack-SDK)
make -C rack RACK_DIR="$PWD/Rack-SDK" dist
```
Offline or cached dependencies: add `-DFETCHCONTENT_SOURCE_DIR_CATCH2=<dir>` and `-DFETCHCONTENT_SOURCE_DIR_JUCE=<dir>`.

## Planning sandbox (for the record)
Ubuntu 24.04, 2 vCPU, 7 GiB, GCC 13.3.0, CMake 3.28.3, Python 3.13, `sudo apt-get` available. Reachable: `github.com` (git), `raw.githubusercontent.com`, Ubuntu mirrors. Not reachable: `vcvrack.com`, `api.osv.dev`, `www.gnu.org`, GitHub release assets, non-repository GitHub REST paths. `clang++` 18 is installed but cannot compile C++ there (no matching standard library headers); sanitizer runs used GCC.
