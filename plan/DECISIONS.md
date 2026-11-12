# Decisions (D-###)

Each decision gives the options considered, the choice, the rationale with claim IDs (`research/claims.json`), and the trigger for revisiting it. "Hardware" means the pedal an effect is modelled on (mapping in D-020 to D-026). Numbers that the hardware documentation does not give are *design choices of this project*; they are listed in D-027 and shown to the owner in the Phase 5 report.

## Project-level

### D-001 Licence: AGPL-3.0-only for the whole repository
- Options: (a) AGPL-3.0-only for everything; (b) Apache-2.0 for the project's own sources with AGPLv3 applying only to distributed VST3 binaries; (c) GPL-3.0-or-later.
- Choice: (a). Adopted intake default A-005; one licence for sources, binaries and both plug-in formats.
- Rationale: JUCE 9.0.5 is offered under AGPLv3 or a commercial licence (C-LEGAL claims "JUCE ... dual-licensed", "JUCE.spdx.json"); a plug-in built under the open option is an AGPLv3 work as a whole. Rack plug-ins that are free of charge may use any licence, and GPLv3 section 13 / AGPLv3 section 13 allow the combination (C-LEGAL-014, "GPLv3 section 13"). Option (b) is legally possible (knowledge items K-20261007T160137Z, K-20261009T234234Z) and is reported to the owner as an alternative; see lesson L-20261111T152000Z.
- Consequences: every source file carries `SPDX-License-Identifier: AGPL-3.0-only` (T-906). Rack Component Library SVG files (CC BY-NC 4.0) are never copied into the repository (D-006). The VST3 SDK (MIT, bundled in JUCE) and Catch2 (BSL-1.0) are compatible inputs.
- Revisit if: the owner prefers Apache-2.0 sources (a documentation and header change; no code change).

### D-002 Language level: C++17 for all code
- Options: C++20 (intake wording); C++17.
- Choice: C++17, set by `CMAKE_CXX_STANDARD 17` and by `-std=c++17` in the Rack makefile.
- Rationale: Rack's `plugin.mk` passes `-std=c++11` and lets a later flag win; Rack itself needs C++17, and its supported toolchains are known to accept C++17 (C-PLANNER-015). The core is compiled by both build systems, so it uses the level both are known to accept. Nothing in the design needs C++20. This amends the intake wording "C++20" and is reported in Phase 5.
- Revisit if: a later step needs a C++20 library feature (none is planned).

### D-003 Rack SDK acquisition
- Choice: CI downloads `https://vcvrack.com/downloads/Rack-SDK-2.6.6-<platform>.zip` for `lin-x64`, `win-x64`, `mac-x64`, `mac-arm64`. Step S-030 records each archive's SHA-256 in `deps.lock.json` at the first download (trust on first use, stated as such in the file) and every later run verifies it before unpacking.
- Rationale: the host is unreachable from the planning sandbox (C-PLANNER-014), so no hash could be recorded now; the URL pattern comes from the knowledge store (K-20261009T234202Z). T-905 requires the four hashes to be present.
- Fallback (pre-decided): if an archive cannot be downloaded in CI after three attempts 30 s apart, the Rack job fails; it is never skipped.

### D-004 Naming and trade marks
- Choice: brand "Mufferfluffer"; effect names "Fluff Lowpass", "Fluff Ring", "Fluff Phase", "Fluff Delay", "Fluff Bank", "Fluff Osc", "Fluff Swarm". No file outside the planning records (`research/`, `plan/`, `premortem/`, `reviews/`, `lessons/`, `knowledge/`, `evidence/`, `HANDOFF.md`) names the manufacturer, its product family or its model codes; `README.md` may do so on at most one line. "VST" never appears in a product or company name; documentation that mentions the format carries the sentence "VST is a registered trademark of Steinberg Media Technologies GmbH." Panels and editors are this project's own design: no copy of the hardware's panel layout, colours or lettering.
- Rationale: MURF and FREQBOX are registered marks, the others are claimed marks whose status could not be read (C-LEGAL-027, "UNKNOWN: current USPTO status"); the manufacturer sells its own plug-in versions (C-LEGAL-033); VCV's ethics guidelines ask plug-ins not to copy names, panel design or component layout (C-LEGAL-020); Steinberg's rules on "VST" (C-LEGAL-011, "VST Usage Guidelines").
- Enforced by: T-901, T-902, T-907.
- The Rack manifest carries the tag "Hardware clone", which is the honest description of behaviour-level modelling and is a tag Rack defines (C-PLANNER-004).

### D-005 VST3 build
- Choice: JUCE 9.0.5 fetched by commit with CMake `FetchContent` (override `FETCHCONTENT_SOURCE_DIR_JUCE`). Shell sources live in `vst3/src` and reach every target through the interface library `muff_vst3_sources` (already in `vst3/CMakeLists.txt`). Seven `juce_add_plugin` targets, `FORMATS VST3` only, `COMPANY_NAME "Mufferfluffer"`, `PLUGIN_MANUFACTURER_CODE Muff`, `BUNDLE_ID org.qualitycoding.mufferfluffer.<slug>`, `COPY_PLUGIN_AFTER_BUILD FALSE`, `IS_SYNTH FALSE`, `NEEDS_MIDI_INPUT FALSE`, `VST3_CATEGORIES Fx` plus the category of the table below. Each target defines `MUFF_EFFECT_ID=<n>` and compiles one extra file, `vst3/src/entry/entry.cpp` (not in the glob), whose `createPluginFilter()` returns `muff::createProcessor((muff::EffectId) MUFF_EFFECT_ID).release()`.

| Effect | Target | `PRODUCT_NAME` | `PLUGIN_CODE` | Extra VST3 category |
|---|---|---|---|---|
| Lowpass | `MuffLowpass` | Fluff Lowpass | `Mfl1` | Filter |
| RingMod | `MuffRingMod` | Fluff Ring | `Mfr1` | Modulation |
| Phaser | `MuffPhaser` | Fluff Phase | `Mfp1` | Modulation |
| Delay | `MuffDelay` | Fluff Delay | `Mfd1` | Delay |
| FilterBank | `MuffFilterBank` | Fluff Bank | `Mfb1` | Filter |
| OscBox | `MuffOscBox` | Fluff Osc | `Mfo1` | Generator |
| Chorus | `MuffChorus` | Fluff Swarm | `Mfc1` | Modulation |

- Options rejected: one shared static JUCE library for all seven targets (not the build JUCE documents for plug-in targets; untested here; each target compiling its own modules is the documented route and was measured). One multi-effect plug-in (contradicts A-004).
- Build time (measured): one target's JUCE modules build in 3 min 05 s on 2 vCPU (C-PLANNER-002). Seven targets plus the test target are eight such builds: about 25 min on 2 vCPU, estimated 13 min on a 4-vCPU hosted runner. The per-job limit for VST3 jobs is 60 min.
- Compiler cache from the start (adversarial pass R5): the Linux and macOS VST3 jobs install `ccache`, pass `-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache`, and restore and save `~/.cache/ccache` (Linux) or `~/Library/Caches/ccache` (macOS) with `actions/cache`, key `ccache-<runner label>-<JUCE commit>-<hash of vst3/CMakeLists.txt>`; the job prints `ccache -s` at the end. Windows (MSVC) builds without a cache.
- Build time on MSVC and Apple Clang was not measured in planning (R-008).
- Revisit if: a VST3 job exceeds 45 min on any runner (then stop and report in `DEVIATIONS.md` with the per-target times; the pre-decided next step is the shared static JUCE library, which is a build-structure change needing review).

### D-006 Rack build and module shells
- Choice: directory `rack/` holding `plugin.json`, `Makefile` (sets `RACK_DIR ?= ../Rack-SDK`, `FLAGS += -std=c++17 -I../core/include -I../adapters/include`, `SOURCES += $(wildcard src/*.cpp) $(wildcard ../core/src/*.cpp) $(wildcard ../adapters/src/*.cpp)`, then `include $(RACK_DIR)/plugin.mk`), `src/plugin.cpp`, one `src/<Enum>.cpp` per module, and `res/<Enum>.svg`.
- A module is a thin shell over `muff::RackBridge` (D-013): `configParam`/`configSwitch` for every table entry (continuous parameters are Rack parameters over the unit range `[min, max]` for linear tapers and over `[0, 1]` with a `ParamQuantity` that displays `fromNormalised` for log tapers; stepped parameters are `configSwitch` with the step names; `tap` is a momentary button), `configInput`/`configOutput` with the table names, one light per meter. `process()` copies parameters into the effect, fills `RackIO` (audio input: `getVoltageSum()`, so polyphonic cables are summed; other inputs: channel 0), runs the bridge, and writes outputs (monophonic). `onSampleRateChange` calls `prepare`. `configBypass(0, 0)` routes the audio input to output 0 for Rack's own bypass.
- Widgets come from Rack's component library through its API; their graphics are used from the Rack installation and are not copied (D-001). Continuous parameter: `RoundBlackKnob`; stepped parameter with two steps: `CKSS`; with three steps: `CKSSThree`; with more steps (LFO wave, pattern): `RoundBlackSnapKnob`; `tap`: `VCVButton` (momentary: the module writes 1 while pressed, 0 otherwise); filter-bank sliders: `RoundSmallBlackKnob`; ports: `PJ301MPort`; meters: `MediumLight<GreenLight>`.
- Rack's build flags differ from the CMake build (`-O3 -march=nehalem -funsafe-math-optimizations`, read from the SDK's `compile.mk`): the `rack-linux` job therefore also builds and runs `muff_core_tests` with `-DCMAKE_CXX_FLAGS="-O3 -march=nehalem -funsafe-math-optimizations -fno-omit-frame-pointer"` (checked in planning against the oracle: 83 of 83 pass).
- Panels are generated by `tools/gen_panels.py` from the fixture: plain rectangles and lines, labels drawn with a built-in single-stroke letter table (upper-case A-Z, digits, `/ - + .`), every label a `<path>`; no `<text>`, no font file (T-903). Panel width: 12 HP for Lowpass, RingMod, Phaser, OscBox; 16 HP for Delay and Chorus; 20 HP for FilterBank.
- Rationale: C-PLANNER-015; C-LEGAL-017; knowledge items on panel SVG limits (K-20261009T234210Z family).
- Verification: the four `rack-*` CI jobs build `make dist` against the SDK and check the package contents (S-033). Loading in a running Rack is checked by the owner at G-LISTEN (it needs a display and Rack itself; R-012).

### D-007 Plug-in validation
- Choice: pluginval v1.0.4, release archive verified by SHA-256 (hashes in `plan/ENVIRONMENT.md`), run as `pluginval --strictness-level 10 --timeout-ms 600000 --random-seed 0x6d756666 --validate <path-to.vst3>` (pre-mortem round 1 ran pluginval 1.0.4 and found that the option is `--random-seed`, that an unknown option such as `--seed` is ignored without an error, and that the pass-rule strings below do appear in a passing log; the log prints `Random seed:` followed by the seed in use, which the pass rule also checks) (a fixed seed makes a failure repeatable; reports exist of intermittent state-restoration failures at strictness 6 and above with choice parameters, R-011) on `vst3-linux` (under `xvfb-run -a`), `vst3-windows` and `vst3-macos`, once per plug-in. The step fails unless the exit status is 0 **and** the captured log contains `SUCCESS` and at least one line starting `Starting tests in:` (C-PLANNER-008; these strings were read from pluginval's source, not from a run: S-021 pre-decides what to do if a passing log lacks them).
- Negative control: `tools/run_pluginval.py --self-test <pluginval-binary>` runs the same pass rule on an empty directory named `NotAPlugin.vst3` and must report failure; each validating job runs it first.
- `vst3-macos-intel` builds and runs the shell tests but not pluginval (one macOS validation is enough for the format; keeps the job inside its limit).

### D-008 Test frameworks
- C++: Catch2 v3.16.1 by commit (already in `tests/CMakeLists.txt`). Python: `unittest` from the standard library, Python 3.10 or newer.

### D-009 CI matrix (file `.github/workflows/ci.yml`, triggers `push` and `pull_request`)
| Job | Runner | Does |
|---|---|---|
| `repo-checks` | `ubuntu-24.04` | `python3 tools/check_frozen.py`; `python3 -m unittest discover -s tests/py -v`; `python3 tools/gen_expected.py --check ...` |
| `core-linux` | `ubuntu-24.04` | Release build, `muff_core_tests`, `muff_bench --limit 5` (the only enforced performance gate, D-050) |
| `core-sanitizers` | `ubuntu-24.04` | GCC, `-fsanitize=address,undefined`, `-DMUFF_SANITIZER_BUILD=ON`, `muff_core_tests` |
| `core-linux-arm64` | `ubuntu-24.04-arm` | Release build, `muff_core_tests`, `muff_bench --limit 0` |
| `core-musl` | `ubuntu-24.04`, container `alpine:3.22` | `apk add build-base cmake ninja git python3`; Release build, `muff_core_tests`; steps use `shell: sh` (the image has no bash) |
| `core-oldest-toolchain` | `ubuntu-24.04`, container `ubuntu:22.04` | `apt-get install -y g++ cmake ninja-build git python3 ca-certificates` (GCC 11.4, CMake 3.22.1: the job prints both and fails if `cmake --version` is not 3.22.x); Release build, `muff_core_tests` |
| `core-windows` | `windows-2022` | MSVC (Visual Studio 2022 generator, x64), `muff_core_tests`, `muff_bench --limit 0` |
| `core-macos` | `macos-15` | Apple Clang arm64, `muff_core_tests`, `muff_bench --limit 0` |
| `core-macos-intel` | `macos-15-intel` | Apple Clang x86_64, `muff_core_tests` |
| `vst3-linux`, `vst3-windows`, `vst3-macos`, `vst3-macos-intel` | as the `core-*` job of the same platform | `-DMUFF_BUILD_VST3=ON`; build all eight targets (the seven plug-ins and `muff_vst3_tests`); run `muff_vst3_tests`; pluginval per D-007 |
| `rack-linux`, `rack-windows`, `rack-macos`, `rack-macos-intel` | `ubuntu-24.04`, `windows-2022` (MSYS2 MINGW64), `macos-15`, `macos-15-intel` | D-003 download and verify; `make -C rack dist`; package check (S-033) |
| `deps-scan` | `ubuntu-24.04` | `python3 tools/check_advisories.py deps.lock.json` (D-061) |
| `aggregate` | `ubuntu-24.04` | `if: always()`, `needs:` every job above that exists in the file so far (the table is the end state after S-034; steps add their jobs as they create them); fails unless every needed job's result is `success` |

- Runner labels: C-PLANNER-010. `windows-2022` is chosen over `windows-2025` because the latter now carries Visual Studio 2026 (C-PLANNER-011), with which JUCE 9.0.5 is untested here.
- Default extras of protocol 3.9: ARM64 Linux job: present (`core-linux-arm64`), core only, because Rack ships no Linux ARM64 SDK and A-006 lists no Linux ARM64 product. libc-variant job: present (`core-musl`). Oldest-supported-toolchain job: present (`core-oldest-toolchain`); the oldest supported toolchain is declared to be GCC 11 with CMake 3.22 (the Ubuntu 22.04 packages). Items 5 and 6 (Android, iOS): not applicable.
- The two container jobs use no JavaScript actions (their Node runtime inside musl or old-glibc containers was not verified): the source is fetched with `git init . && git remote add origin https://github.com/${GITHUB_REPOSITORY}.git && git fetch --depth 1 origin "$GITHUB_SHA" && git checkout FETCH_HEAD` (the repository is public), and the test log is printed instead of uploaded; their steps use `shell: sh` with `set -eu` and no pipes.
- `ubuntu-22.04` as a runner label is not used: it left active support in September 2026 (R5 finding).
- Every job has `timeout-minutes` (15 for core and repo jobs, 60 for VST3, 40 for Rack). No job uses `continue-on-error`; no command ends in `|| true` (T-905).
- No binary is uploaded as a workflow artefact by `ci.yml`: the repository is public, so an artefact is a publication (C-LEGAL-036, A-009). Test logs (text) are uploaded with `if: always()`. Binaries are produced only by the `packages.yml` workflow of S-040, which runs only on a push whose commit subject starts with the approval marker, behind gate G-002.

### D-010 Repository layout
`core/` (DSP core: `include/muff/core.hpp`, `include/muff/blocks.hpp`, `src/`), `adapters/` (`include/muff/adapters.hpp`, `src/`), `vst3/` (`include/muff/vst3_shell.hpp`, `src/`, `CMakeLists.txt`), `rack/`, `tests/` (frozen), `tools/` (generators and checkers), `docs/`, `.github/workflows/`, planning records as created by this run. Build output goes to `build/` (ignored by git).

## Interfaces

### D-011 Core interface
The public interface of the DSP core is exactly `core/include/muff/core.hpp` and `core/include/muff/blocks.hpp` as committed by this plan (frozen: listed in `tests/FROZEN_MANIFEST.sha256`). Every function's inputs, outputs and error behaviour are stated in the header comments; the stubs in `core/src/stubs.cpp` make the tests importable and are deleted by the implementation steps.
The description tables (`EffectInfo`, `ParamSpec`, `PortSpec`, `MeterSpec`, pattern table) are **generated**: `python3 tools/gen_tables.py tests/fixtures/effects.json core/src/generated_tables.inc`, included once by the core. The fixture is the single source of every user-visible name, range and default (T-001, T-904).

### D-012 Units and control voltages
- Audio: core unit 1.0 = nominal full scale = 5 V in Rack. CV: 1.0 = +5 V.
- A `CvNormalised` input adds `depth x cv` to the target parameter's normalised knob position; the sum is clamped to [0, 1]. Depth is 1.0 (5 V sweeps the whole knob travel, as the hardware manuals state: C-PEDA-011) except the Wave input of OscBox, depth 0.6 (C-PEDC-011).
- A `CvVoltPerOct` input multiplies the target frequency by `2^(5 x cv)`, not limited to the knob range (OscBox Frequency only).
- A `Trigger` input fires on a rising edge through 0.2 CV units (1 V) and re-arms below 0.02 (0.1 V), so both 5 V gates and Rack's 10 V triggers work (T-812).
- The core does not slow down on decaying signals: `process()` sets flush-to-zero and denormals-are-zero for its duration and restores the caller's mode (x86-64: MXCSR bits 0x8040; AArch64: FPCR bit 24), or flushes its states explicitly; the benchmark's silence phase enforces it (D-050).
- Rack users should know that a 0-10 V source reaches the end of a control's travel at 5 V, as on the hardware; the user guide says so.
- Disconnected inputs read 0. Non-finite input samples read 0; finite inputs are limited to +/-16 before use. Every output is finite and within +/-4.0 (`kOutputLimit`); the Rack bridge further limits every output to +/-12 V (VCV's voltage guidance; T-812); if an internal state becomes non-finite the effect resets its audio state and outputs 0 for that sample (T-007).

### D-013 Host adaptors
`adapters/include/muff/adapters.hpp` as committed (frozen): `BlockRunner` for block hosts, `RackBridge` for per-sample voltages; behaviour in the header comments; tests T-801, T-802, T-811, T-812, T-821.

### D-014 Tap tempo
`muff::TapTempo` as specified in `blocks.hpp`: the third tap of a run yields the mean interval; later taps refine the mean; a tap more than 5.0 s after the previous one starts a new run (C-PEDB-017). In an effect, a tap is a rising edge of the `tap_in` trigger input or a 0-to-1 change of the `tap` parameter. A tapped tempo takes control of its target until the target's knob position (including CV) moves by more than 0.01 of its travel from where it was at the tap that made the period available. Targets: Delay: delay time = tapped period, limited to the selected range; Chorus: LFO rate = 1 / period, limited to 0.01-100 Hz; FilterBank: step rate = 2 / period (taps are quarter notes, steps are eighths: C-PEDB-046), limited to 0.05-200 Hz.

### D-015 LFO
`muff::lfoShape` as specified in `blocks.hpp`. Each LFO is a phase accumulator starting at phase 0 after `reset()`. For `RandomStep` a new uniform value in [-1, 1] is drawn each time the phase wraps, from a generator seeded identically at every `reset()` (T-008 determinism); the value before the first wrap is 0.

### D-016 VST3 shell
`vst3/include/muff/vst3_shell.hpp` as committed (frozen): the contract in its header comment; tests T-851 to T-856. The default bus layout is stereo in, stereo out. The host's own bypass is JUCE's default pass-through (stereo, unprocessed); `active` models the pedal's footswitch, including its mono and left-only quirks (D-018), and the user guide explains the difference. `processBlock` uses `juce::ScopedNoDenormals`. Auxiliary inputs and CV inputs of the core are not exposed in the VST3 (they stay disconnected); CV outputs are not exposed. The editor is this project's own generic layout: one rotary slider per continuous parameter, one combo box per stepped parameter, a momentary button for `tap`, three small lamps for the meters, laid out in rows of at most six controls on a plain background with the effect name; fixed size 120 px per control column, 140 px per row plus a 40 px header. `hasEditor()` is true; the editor reads meters with a 30 Hz timer.

- Threads: a `muff::Effect` is used from the audio thread only (`core.hpp`). Parameters reach it from JUCE's atomic parameter values at the top of `processBlock`; meters leave it through `std::atomic<float>` members that `processBlock` writes at the end of each block and the editor's 30 Hz timer reads. The editor holds no pointer to the effect.

### D-017 Parameter smoothing and timing
Continuous parameters are smoothed inside the core with a one-pole of 5 ms time constant applied to the normalised position after CV is added (so CV is smoothed too); stepped parameters switch at once except `active`, which cross-fades over the same 5 ms one-pole. After `reset()` the smoothers start at the parameter targets without CV. Delay times get no extra smoothing: the clocked delay line of D-033 produces the pitch behaviour of a clock change by itself.

### D-018 Bypass (`active` = 0)
- Lowpass and RingMod: output = the driven dry signal (Drive stays in the path: C-PEDA-007).
- Phaser, OscBox, Chorus: every audio output = the input, unity gain. Delay: Mix Out = input; Delay Out and the wet part are silent unless Spillover is on, in which case echoes already in the line continue (also added to Mix Out, scaled by Output and Mix) while no new input is recorded (C-PEDB-002). FilterBank: Left = input, Right = silence (C-PEDB-052).
- CV outputs and oscillator outputs keep running in bypass.

## Effects (D-020 to D-026)
Parameter, port and meter tables: `tests/fixtures/effects.json` (authoritative). Common to all: `drive` in dB into `softClip` (D-037); meter `level` = peak of the driven signal with 100 ms decay; meter `active` = the cross-fade position.

### D-020 Lowpass ("Fluff Lowpass", hardware MF-101)
Signal: in -> Drive -> x. Envelope follower on x (D-030). Raw cutoff `fr = cutoff x 2^(env_amount x env)`; the filter runs at `fc = clamp(fr, 10 Hz, min(20 kHz, 0.45 fs))`. Ladder filter (D-030) with loop gain `k = (resonance / 2) x fade`, `fade = clamp((20 kHz - fr) / 4 kHz, 0, 1)`: above 16 kHz the resonance is faded out, reaching none at 20 kHz, so that self-oscillation leaves the audible range as it does on the hardware (which reaches about 40 kHz, C-PEDA-003) instead of whistling at the top of the band (T-109; R5 finding) (self-oscillation from 8 upward: C-PEDA-002). Output tap: stage 4 (`poles` = 4-pole) or stage 2 (2-pole) with the four-stage loop intact. Out = `(1 - mix) x + mix wet`. `env_out` = env limited to 2.0. Cutoff range 15 Hz-12 kHz log (C-PEDA-001); Amount 0-5 octaves (C-PEDA-006).

### D-021 RingMod ("Fluff Ring", hardware MF-102)
Carrier: sine, frequency `f = base(frequency, range) x 2^(0.5 x lfo_amount x lfo)`, base Lo `0.6 x (80/0.6)^n`, Hi `30 x (4000/30)^n` with n the knob position (C-PEDA-023), limited to 0.45 fs. LFO: sine or square, 0.1-25 Hz; `lfo_amount` is the peak-to-peak sweep in octaves, 0-3 (C-PEDA-028). Wet = x times carrier (ideal multiplier, D-031). A connected `carrier_in` replaces the oscillator in the multiplier: carrier = `clamp(7.0711 x carrier_in, -2, 2)` (a 0.5 V RMS sine gives full modulation: C-PEDA-029). `carrier_out` = 0.25 x the internal oscillator always (C-PEDA-029); `lfo_out` = 0.3 x LFO whatever the Amount (C-PEDA-029).

### D-022 Phaser ("Fluff Phase", hardware MF-103)
x = driven input. Feedback sum `u = x + 0.09 x resonance x tanh(w)` with w the previous sample's chain output. Chain: 6 or 12 first-order all-pass stages (D-032); stage corner `fc = 1100 Hz x 2^(6 x (sweep/10 - 0.5) + 3 x lfo_amount x lfo)`, limited to [20 Hz, 0.45 fs]; LFO triangle, rate `0.01 x 250^n` Hz (Lo) or `1 x 250^n` Hz (Hi) (C-PEDA-039). In 12-stage mode with `sweep_in` connected, stages 7-12 use `fc = 1100 Hz x 2^(6 x (sweep/10 - 0.5) + 6 x clamp(sweep_in, -0.5, 0.5))` instead of the LFO (C-PEDA-046). Out = `output x 0.5 (u + y)`, Aux = `output x 0.5 (u - y)` (C-PEDA-044), `lfo_out` = 0.56 x LFO (C-PEDA-040). No Mix control (C-PEDA-045).

### D-023 Delay ("Fluff Delay", hardware MF-104M, standard edition)
Delay time `d = lo x 10^n`, lo = 40 ms (Short) or 80 ms (Long), so 40-400 ms and 80-800 ms (C-PEDB-006); LFO multiplies d by `2^(lfo x lfo_amount / 10)`; d is limited to the range; d sets the clock of the delay line of D-033, so changing it replays the stored signal faster or slower (T-409). The Super Delay edition is out of scope. Wet = two-pole Butterworth low-pass of the delay-line read at 3.0 kHz (Short) or 1.7 kHz (Long) (C-PEDB-022). Loop: line input = `a x + (feedback / 8) x softClip(f)`, a = the active cross-fade, f = `fb_return` if connected, else wet; loop gain 1 at Feedback 8 (C-PEDB-011). `fb_send` = wet. Mix Out = `output ((1 - mix) x + mix wet)`; Delay Out = `output x wet`. LFO: six shapes (D-015), 0.05-50 Hz. Tap: D-014.

### D-024 FilterBank ("Fluff Bank", hardware MF-105M)
Eight filters in parallel on the driven input. Mids voicing: band-pass at 200, 300, 450, 675, 1000, 1500, 2200, 3400 Hz. Bass voicing: filter 1 a two-pole Butterworth low-pass at 110 Hz, filters 2-8 band-pass at 160, 240, 350, 525, 775, 1200, 1800 Hz (C-PEDB-032, "BASS voicing"). Band-pass: state-variable, Q = 5, unity gain at the centre (D-034). All centres are multiplied by `2^shift`: LFO off: `shift = sweep - 0.5` octaves; LFO on: `shift = 0.5 sin(2 pi phase)`, LFO rate `0.08 x 250^sweep` Hz (0.08-20 Hz: C-PEDB-047).
Each filter output is scaled by its slider (`levelN`, linear) and its envelope. Sequencer: step rate = `rate` (0.5-32 steps per second, log) or the tapped rate (D-014) or, when `step_in` is connected, one step per trigger. The first step happens on the first sample after `reset()`. On step s, channel c triggers if its pattern has `x` at position `s mod length_c`. Envelope: on a trigger, rise linearly from the present value to 1 in `attack` steps, then fall linearly to 0 in `decay` steps, `attack` and `decay` from `filterBankEnvelopeTimes(envelope)` (D-015 block, table in `blocks.hpp`), each at least one sample; a new trigger restarts the rise from the present value. Pattern 1 of each bank is static (all envelopes 1).
**Patterns are this project's own** (24 = 12 per voicing, in the fixture; the Mids bank mirrors the Bass bank's channel order). The hardware's 24 patterns are published as graphics in the manual's appendix, which the research agents could not read (C-PEDB-041, unverified: a negative result). Whether pattern data may be re-expressed is a legal question this plan does not answer (the one manual copyright notice read is all-rights-reserved, C-LEGAL-035); the owner, who can read the grids in their own copy, decides (R-003, gate question L-5).
Left = `output ((1 - mix) x + mix x sum of filters 1,3,5,7)`, Right likewise with filters 2,4,6,8 (C-PEDB-052); mono hosts get Left + Right (`monoSumsRight`).

### D-025 OscBox ("Fluff Osc", hardware MF-107)
x = driven input; env = follower of x (D-035). Oscillator frequency `f = frequency x 2^(5 cv_frequency + env_amount x min(env, 1) + 2 x fm_amount x x)`, limited to [0.01 Hz, min(20 kHz, 0.45 fs)]; knob 25 Hz-1.6 kHz, Env Amount 0-8 octaves upward (C-PEDC-001, "1 V per octave", "up to 8 octaves"). Wave w in [0, 1]: 0 triangle; 1/3 sawtooth (rising); 2/3 square; 1 pulse of 10 % duty; linear cross-fade triangle-saw on [0, 1/3] and saw-square on [1/3, 2/3]; pulse width `0.5 - 0.4 x 3 (w - 2/3)` on [2/3, 1] (order from C-PEDC-003). Sync on: the phase resets to 0 when x rises through +0.02 after having been below -0.02. Out = `output ((1 - mix) x + mix x osc x min(env, 1))` (C-PEDC-007); `env_out` = env limited to 2.0; `osc_out` = 0.5 x osc, always running (C-PEDC-014).

### D-026 Chorus ("Fluff Swarm", hardware MF-108M)
Delay `d = lo x (hi/lo)^(time + 0.5 x lfo_amount x lfo)`, limited to [lo, hi], setting the clock of a delay line of the D-033 kind; Flange lo 0.6 ms, hi 10 ms; Chorus lo 5 ms, hi 50 ms (C-PEDC-024). Wet = two-pole Butterworth low-pass of the delay-line read at `min(16 kHz, 200 / d)` Hz (D-036). Loop: line input = `x + 1.25 x feedback x softClip(f)`, f = `fb_return` if connected else wet; feedback in [-1, 1], loop gain 1 at |feedback| = 0.8 (C-PEDC-025; five marks a side is a design choice, D-027). Left = `output ((1 - mix) x + mix wet)`; Right = `output ((1 - mix) x - mix wet)` (C-PEDC-035); `fb_send` = wet. LFO: six shapes, 0.05-50 Hz; tap: D-014. Meter `rate` = 1 while the LFO phase is below 0.5.

### D-027 Design choices where the hardware is undocumented (owner-visible list)
None of these could be read from any source (C-PEDA-018 claims); each is a reasoned choice, frozen by tests, and changeable only through the amendment route of `HANDOFF.md` section 6.
1. Drive range -7 to +28 dB for every effect. Those end points are documented for the chorus/flanger pedal only; the delay pedal's manual gives a span of about 35 dB without end points (round 2 could not confirm them), and the other five manuals give no figure. The limiter shape `2 tanh(x/2)` is a design choice.
2. Output range -24 to +12 dB.
3. Envelope follower times: Lowpass Fast 2 ms / 50 ms, Smooth 20 ms / 300 ms (attack / release); OscBox 5 ms / 100 ms.
4. Lowpass 2-pole mode keeps the four-stage feedback loop and taps stage 2; no make-up gain as resonance rises; the cutoff is limited to 20 kHz with the resonance faded out between 16 and 20 kHz.
5. RingMod: ideal multiplier, pure sine carrier; the undocumented noise gate reported by one reviewer is not modelled.
6. Phaser: stage corner 1.1 kHz at Sweep 5; resonance feedback 0-0.9, never self-oscillating; Aux = half of (feedback sum minus chain).
7. Delay: logarithmic Time knob; fixed two-pole filters (they do not track the clock); no compander, no clock noise; LFO depth of one octave either way at Amount 10; feedback law linear with unity at 8.
8. FilterBank: Q = 5; Rate 0.5-32 steps per second; envelope time table; sweep depth half an octave either way; dry signal to both outputs when Mix < 1; **own pattern set**.
9. OscBox: exponential FM of 2 octaves per core unit at full Amount; sync threshold +/-0.02; linear VCA; pulse width 10 % at the end of the Wave travel.
10. Chorus: logarithmic Time knob and LFO law; wet bandwidth `min(16 kHz, 200/d)`; unity loop gain at |feedback| 0.8; no compander; right-output modes other than the default are not offered; the Rate CV does not extend the rate beyond the knob's range.
11. Not modelled anywhere: MIDI, host tempo sync, preset storage, noise, component tolerance and drift.
12. Lowpass cutoff knob law: logarithmic from 15 Hz to 12 kHz, which puts the knob's centre at 424 Hz; one manual reading gives the centre as roughly 500 Hz (C-PEDA-001), so the centre is about a quarter-octave lower than that reading.

## Algorithms (D-030 to D-037)

### D-030 Ladder filter and envelope follower
- Options: (a) Huovilainen's non-linear model with 2x oversampling; (b) zero-delay-feedback ladder of four trapezoidal one-poles with one saturator in the feedback path; (c) D'Angelo-Välimäki generalised ladder.
- Choice: (b). Stage: `G = g / (1 + g)`, `g = tan(pi fc / fs)`; `v = (in - s) G; y = v + s; s = y + v`. With `S = (1 - G)(G^3 s1 + G^2 s2 + G s3 + s4)`: `u = (x' - k S) / (1 + k G^4)`, then one refinement `u = x' - k softClip(G^4 u + S)`, then the four stages. `x'` = x plus uniform noise of amplitude 1e-5 from a generator seeded at `reset()` (it starts self-oscillation when there is no input).
- Rationale: k = 4 is the self-oscillation point and the oscillation frequency equals the prewarped cutoff (C-DSP-010, "Zero-delay-feedback solution", "TPT one-pole"); DC gain 1/(1 + k) is left uncompensated (C-DSP-018). Verified by the oracle: T-103 measures the oscillation within 2 % of the cutoff at 100 Hz, 1 kHz and 5 kHz at 44.1, 48 and 96 kHz.
- Contradicting evidence, recorded: the literature recommends oversampling non-linear ladders, and one author calls 44.1 kHz usually insufficient (C-DSP-002, "Zavalishin on oversampling"). This plan ships without oversampling because the single saturator sits in the feedback path only and the tests of tuning and response pass; audible aliasing at high resonance and drive is a named weak spot of the listening gate (question L-3). If the listener reports it, the remedy is 2x oversampling of the ladder loop with a half-band interpolator and decimator; that adds latency and changes the dry-path timing tests, so it goes through the amendment route of `HANDOFF.md` section 6 and is not pre-authorised; R-006. (An earlier draft named a filter-free two-pass scheme; the adversarial pass showed it would not remove aliasing, and it was withdrawn.)
- Envelope follower: `r = |x|`; `e += (r > e ? aAtt : aRel)(r - e)`, `a = 1 - exp(-1 / (tau fs))`; no scaling (C-DSP-062).

### D-031 Ring modulation
Ideal multiplication of the driven input by the carrier (C-DSP-050); no diode-ring model. Aliasing of the sum frequency above fs/2 is accepted (carrier at most 4 kHz).

### D-032 Phaser all-pass chain
First-order all-pass per stage: `a = (t - 1)/(t + 1)`, `t = tan(pi fc / fs)`; `y = a x + x1 - a y1`. N equal stages give N/2 dips where the chain phase is an odd multiple of 180 degrees (C-DSP-045, "First-order digital all-pass"); with equal corners the dips of 6 stages fall where one stage's phase is -30, -90 and -150 degrees, 1.9 octaves apart, matching the manual's "about two octaves" (T-301). Feedback is taken around the whole chain with a one-sample delay and `tanh` on the fed-back sample; the outputs use the feedback sum u, so resonance raises peaks and keeps dips (T-305).

### D-033 Clocked delay line (Delay and Chorus)
- Options: (a) a fixed-rate circular buffer read at a smoothed fractional delay; (b) a line that models the clocked transit of a bucket-brigade device; (c) the full device-and-filter models of the literature (C-DSP-031 to C-DSP-036).
- Choice: (b), after the adversarial pass showed that (a) reproduces a time change as a short chirp, whereas a clocked device replays its whole contents at the new speed for one delay time.
- Definition: a clock phase `p` advances by `1 / (d x fs)` per sample (d = the present delay time in seconds, at least one sample). Each input sample is stored together with the phase at which it entered. The output at any moment is the stored signal at phase `p - 1`, by linear interpolation between the two stored samples whose phases bracket it (interpolating in phase); it is 0 until the phase has advanced by 1 since `reset()`. For that first interpolation the line behaves as if one silent sample had been stored one clock step before the first real one. Per sample: read, then write (so with a constant d the delay is exactly `d x fs` samples), then advance the phase. The buffer holds `0.85 fs + 8` samples for Delay and `0.06 fs + 8` for Chorus; if the read position would fall off the buffer it is moved to the oldest stored sample. To keep precision the phase is rebased (the same constant subtracted from every stored phase) before it exceeds 1e6: a plain loop over the preallocated buffer, no allocation.
- Consequences: a steady d gives the delays of T-401 and T-701; halving d plays the stored signal an octave up for one new delay time, doubling it an octave down (T-409); LFO modulation becomes modulation of the clock, as on the hardware.
- Still not modelled: sampling at the clock rate, companding, clock noise (D-027 item 7). The fixed low-pass filters of D-023 and the tracking low-pass of D-036 stand in for the device's band limit.

### D-034 Filter-bank filters
Zavalishin trapezoidal state-variable filter: `hp = (x - (k + g) s1 - s2)/(1 + k g + g^2)`, `bp = g hp + s1`, `lp = g bp + s2`, state updates `s1 = bp + g hp`, `s2 = lp + g bp`, `g = tan(pi fc / fs)`, `k = 1/Q` (C-DSP-026). Band-pass output `k bp` (unity at the centre); low-pass uses `k = sqrt(2)`. Stable under the half-octave sweep (C-DSP-029).

### D-035 Oscillator
- The oscillator output runs **one sample late** (the value computed for sample n is emitted at n + 1), so that every step in the waveform can be corrected on both sides. `latencySamples()` stays 0: the dry path is not delayed, and one sample on the oscillator is inaudible.
- Per sample: advance the phase by `dt = f / fs`; compute the naive waveform value `cur`; then for each step that occurred since the previous sample, of height h, at t samples before the present sample (0 <= t < 1): add `0.5 h t^2` to the previous sample's value (still pending) and subtract `0.5 h (1 - t)^2` from `cur` (two-sample polynomial band-limited step, C-DSP-053). Steps: phase wrap (`t = phase_after_wrap / dt`; height `-2 x saw weight + 2 x pulse weight`); pulse falling edge at phase = width (`t = (phase - width) / dt`; height `-2 x pulse weight`); hard-sync reset.
- Hard sync: the input crossed +0.02 between the previous input sample and this one; `t = (x - 0.02) / (x - x_previous)`, limited to [0, 1]. The phase restarts from 0 at that instant, so the phase now is `t x dt`. Height = (naive value at phase 0) minus (naive value at the phase reached at the crossing instant). Any wrap or edge correction already applied for this sample is discarded (the pending sample returns to its naive value) before the sync step is applied.
- The triangle part needs no correction. FM and envelope modulation change `dt` per sample.
- Evidence: the oracle built this way measures aliases at -66 dB or better relative to the strongest harmonic with and without sync, and -46 dB for the synced 10 % pulse (edges a few samples apart); a sync reset quantised to the sample grid measures about -30 dB. T-607 freezes -55 dB (-40 dB for the 10 % pulse). Audio-rate FM with these corrections remains an inference (C-DSP-061, R-007).

### D-036 Chorus delay
The clocked line of D-033. Wet bandwidth: the bucket-brigade device and its filters are unknown (C-PEDC-043), so a clock-tracking bandwidth `min(16 kHz, 200 / d)` stands in for the tracking a clocked delay line shows (C-DSP-031).

### D-037 Drive stage
`softClip(g x)` with `g = 10^(drive/20)`, memoryless, with no anti-aliasing, for both the dry and the wet path.
- Why not anti-aliased: first-order antiderivative anti-aliasing (C-DSP-064) costs half a sample of delay and a treble droop of about 0.6 dB at 10 kHz and 2.6 dB at 20 kHz (48 kHz rate) on every signal, including a clean one at unity drive; oversampling adds latency. The frozen tests check the dry path against `2 tanh(x/2)` to within 1e-3 (T-010, T-107, T-206), which a delayed dry path would fail; the wet path is not constrained by them, but one drive computation feeds both paths to keep them phase-aligned in the Mix control.
- Measured cost (adversarial pass, oracle): with +28 dB of drive on a loud high-frequency input, alias components reach about -21 dB relative to full scale. This is accepted for this version and put to the listener: demo 23 and question L-3. R-006.

## Quality and gates

### D-040 Listening references and the form of the listening gate
- Search result: no redistributable recording (CC0, CC BY, CC BY-SA) of any of the seven pedals was found, and no dry/wet pair with stated settings under any licence (C-REF-028 to C-REF-035; search log in `research/SOURCES.md`). Several hosts could not be searched from this environment (Freesound content, Wikimedia Commons, SoundCloud); that gap is recorded as R-004. The restrictive recordings found (music under CC BY-NC-SA / BY-NC-ND on the Internet Archive) state no settings and have no dry counterpart, so they cannot support a blind comparison and are not used.
- Decision (protocol 3.10.1): the blind A/B test is **replaced by a demo bundle** with fixed questions. The goal "sounds like the hardware" stays, but it is judged by the owner's ear against whatever they know of the hardware, not by a scored comparison. This weakens SC-10 from "indistinguishable in a blind test" to "the owner accepts the demos"; reported in Phase 5.
- The owner may, privately, compare the demos with any recording or official demonstration of the hardware they have access to, or supply their own dry/wet recordings of hardware (R-004's response); the gate text says so.
- Dry material for the demos is synthesised by a committed script (`tools/make_demo_inputs.py`: plucked-string tones, a sawtooth bass line, a drum-like noise pattern, a held chord), so every file in the bundle is this project's own work and may sit on a public branch.

### D-041 Publication
The repository is public. Allowed on public branches: source, plans, test logs, demo audio made entirely by this project. Not allowed without G-002: built binaries (VST3 bundles, `.vcvplugin` packages). Never allowed: third-party recordings, manual pages, manufacturer artwork.

### D-050 Performance gate
`muff_bench --limit 5` on `core-linux` (`ubuntu-24.04`, x64) is the gate for SC-6. The benchmark times each effect on a steady tone and on silence after a tone and reports the slower (the adversarial pass found the first oracle rising to 9.3 % in silence through denormal numbers; with D-012's protection it measures 0.53 to 1.9 %). Measured planning value: the oracle uses 0.53-1.9 % per effect on a 2-vCPU sandbox (C-PLANNER-017), so 5 % leaves a factor of about 2.6 for the slowest effect and more for the others, for interpolation quality, band-limiting and runner variance (C-PLANNER-018). Other platforms run `--limit 0` and record the numbers in the job log; they do not gate. If `core-linux` fails the gate, the pre-decided response is profiling and optimisation of the failing effect (S-018), never raising the limit.

### D-060 Mutation testing
See step S-MUTATE in `plan/PLAN.md` for the full record required by the skill. Tool: Mull for LLVM 19, with the floor kept in `mutation-baseline.json` (`project_floor`), installed from the `.deb` attached to a GitHub release of `mull-project/mull` (not from the Cloudsmith repository, which Mull's changelog says is exhausted: R5 finding), with clang-19; scope `core/src/*.cpp` and `adapters/src/*.cpp` (not the generated tables, not `vst3/`, not `rack/`); floor 60 % project score.

### D-061 Dependency vulnerability scan
`tools/check_advisories.py` posts each pinned commit of `deps.lock.json` to `https://api.osv.dev/v1/query` as `{"commit": "<sha>"}` and fails if any response contains a `vulns` entry whose id is not in the lock file's optional `advisory_allow` list, or if a request fails after three attempts 20 s apart (C-PLANNER-019). The URL can be replaced through the environment variable `MUFF_OSV_URL`, which exists only so that the failure path can be demonstrated (S-034). `advisory_allow` holds the ids of advisories that were read and judged not to affect this project, each with an entry in `plan/RISKS.md`; the tool prints the list on every run. An empty answer means "none known", not "proven safe", and the query does not see libraries bundled inside JUCE (zlib, libpng, FLAC, HarfBuzz and others); both limits are stated in the script's output.

### D-062 Code review
Protocol 3.8 as written: one review of the whole implementation diff (step S-REVIEW) by three fresh-context reviewers with the briefs listed in that step (correctness and anti-patterns; security and real-time safety; untested code paths and dependency currency), findings merged in `reviews/CODE_REVIEW.md`, zero unresolved Critical or High before any later step; then the adversarial review of S-ADVERSARIAL in `reviews/ADVERSARIAL_REVIEW.md`. There is no separate review per milestone: the three milestones agreed at intake only order the effect steps, and the effects are reviewed together.

### D-063 Tests added and amended after the first pre-mortem round
A fresh reviewer built variants of the reference implementation and found that (a) a Delay or Chorus whose feedback loop bypasses the wet filter passed every test, (b) the 2-pole Lowpass mode was never tested with resonance, and (c) T-304's 250 Hz case failed for a rate only 0.02 % off, far inside its stated 2 % tolerance. Added: T-410 and T-708 (each repeat is darker than the last), T-110 (2-pole mode self-oscillates at the cutoff). Amended: T-304 counts cycles for the 250 Hz case. Evidence of red and green, and of the unfiltered-loop variant failing exactly the two new tests: `research/spikes/red-verification/RESULT.md`.

### D-071 Binary compatibility floor
- macOS: the VST3 configure step on macOS passes `-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0` (both architectures), so bundles built on the macOS 15 runner load on macOS 11 and later; Rack's own makefile sets its floor for the Rack plug-in. `packages.yml` prints each macOS library's minimum OS (`otool -l <library> | grep -A3 LC_BUILD_VERSION`) and fails if `minos` is above 11.0.
- Linux: the floor is the build image, Ubuntu 24.04 (glibc 2.39); stated in `README.md`. Windows: the MSVC runtime is linked dynamically (JUCE's default); `README.md` says the Microsoft Visual C++ redistributable is required.
- `README.md` `## Install` also says that on macOS a downloaded bundle must have its quarantine attribute removed (`xattr -dr com.apple.quarantine <bundle>`), because the bundles are not signed or notarised (A-009: no release).
- Not verified in planning (no macOS machine): if JUCE 9.0.5 refuses a deployment target of 11.0 at configure time, use the lowest value its error message names and record it in `DEVIATIONS.md`.

### D-070 Versions
VST3 plug-ins `0.1.0` (CMake project version). Rack plug-in `2.0.0` (Rack requires major version 2).
