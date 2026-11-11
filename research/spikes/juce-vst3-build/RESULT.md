# Spike: JUCE 9.0.5 VST3-only plug-in builds in the planning sandbox (R6)

- **Tests claim**: C-ENV-JUCE (JUCE 9.0.5 `juce_add_plugin(... FORMATS VST3)` configures and links on Ubuntu 24.04 with GCC 13.3 and CMake 3.28.3).
- **Date**: 2026-11-11. **Machine**: planning sandbox, 2 vCPU, 7 GiB, Ubuntu 24.04.
- **JUCE**: tag `9.0.5`, commit `d60b7a78030661574a615b586d9325ba7a7fa7dd`, cloned to `/home/claude/deps/JUCE`.
- **Packages installed first** (`sudo apt-get update` was needed before install): `libasound2-dev libxrandr-dev libxinerama-dev libxcursor-dev libglu1-mesa-dev libxcomposite-dev libxi-dev libfreetype-dev libfontconfig1-dev libx11-dev libxext-dev libxrender-dev`.
- **Commands**:
  - `cmake -S research/spikes/juce-vst3-build -B /home/claude/build/juce-spike -G Ninja -DCMAKE_BUILD_TYPE=Release -DJUCE_SOURCE_DIR_OVERRIDE=/home/claude/deps/JUCE` -> exit 0, 38.6 s wall.
  - `cmake --build /home/claude/build/juce-spike --target MuffSpike_VST3 -j2` -> exit 0, 3 min 05 s wall (5 min 42 s CPU), 54 build edges.
- **Result**: `MuffSpike_artefacts/Release/VST3/MuffSpike.vst3/Contents/x86_64-linux/MuffSpike.so` (12 894 504 bytes) and `Contents/Resources/moduleinfo.json` produced.
- **Observations**: JUCE 9.0.5 prints a compile-time note that `JUCE_DISPLAY_SPLASH_SCREEN` is ignored (no splash screen exists in this version). The first build of the JUCE modules dominates; seven plug-ins sharing one static JUCE library target avoid paying it seven times (D-005).
- **Not covered**: loading the plug-in in a host; pluginval (runs in CI, S-021).
