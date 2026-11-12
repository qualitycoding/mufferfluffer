# Traceability

## Success criteria to tests, decisions and steps

| SC | Statement (short) | Tests | Decisions | Steps | Result (filled by S-FINAL) |
|---|---|---|---|---|---|
| SC-1 | Seven effects with exactly the tabled parameters and ports | T-001, T-002, T-003, T-004, T-015, T-851, T-904, T-907 | D-011, D-020..D-026 | S-010..S-018, S-020 | |
| SC-2 | Behaviour within tolerance of the specification | T-010, T-012..T-014, T-017, T-020..T-024, T-101..T-110, T-201..T-206, T-301..T-307, T-401..T-410, T-501..T-507, T-601..T-607, T-701..T-708 | D-012..D-037 | S-010..S-018 | |
| SC-3 | Seven VST3 plug-ins build on four platforms and pass pluginval | T-851, T-854, T-CI-PLUGINVAL, T-905 | D-005, D-007, D-016 | S-020, S-021 | |
| SC-4 | Rack plug-in with seven modules builds on four platforms; manifest and panels valid | T-902, T-903, T-CI-RACKPKG, T-905 | D-003, D-006 | S-030..S-033 | |
| SC-5 | VST3 path and Rack path give the same audio (difference at most 1e-6) | T-016, T-801, T-802, T-811, T-812, T-821, T-852 | D-013, D-016 | S-018, S-020 | |
| SC-6 | Under 5 % of one core per effect on `ubuntu-24.04` x64 | T-PERF-1 | D-050 | S-018 | |
| SC-7 | Finite, bounded output for any parameters, input and sample rate | T-005, T-006, T-007, T-018, T-812, T-853, T-856, T-CI-SAN | D-012 | S-018, S-020 | |
| SC-8 | No allocation or lock on the audio path | T-011, T-855 | D-011, D-016 | S-018, S-020 | |
| SC-9 | No third-party marks in the product; licence headers | T-901, T-902, T-906 | D-001, D-004 | S-021, S-032, S-035 | |
| SC-10 | Listening gate passed | gate G-LISTEN (not a test) | D-040 | S-LISTEN | |

## Platforms declared at intake (A-006) to CI jobs (D-009)

| Platform | Core tests | VST3 build + shell tests | pluginval | Rack package |
|---|---|---|---|---|
| Linux x64 | `core-linux`, `core-sanitizers`, `core-oldest-toolchain`, `core-musl` | `vst3-linux` | `vst3-linux` | `rack-linux` |
| Windows x64 | `core-windows` (MSVC) | `vst3-windows` (MSVC) | `vst3-windows` | `rack-windows` (MinGW) |
| macOS arm64 | `core-macos` | `vst3-macos` | `vst3-macos` | `rack-macos` |
| macOS x64 (Intel) | `core-macos-intel` | `vst3-macos-intel` | not run (D-007: one macOS validation) | `rack-macos-intel` |
| Linux arm64 (not a declared product platform; default extra of protocol 3.9) | `core-linux-arm64` | none (no product) | none | none (no Rack SDK for it) |

## Test catalogue

Categories required by the protocol and where they are met: unit (core behaviour); integration (adaptors, VST3 shell, pluginval, Rack packaging); contract (tables, parameter layout, manifest, panels, licence, naming); golden/regression (T-505 compares every pattern with the frozen fixture; no golden audio files are used because bit-exact audio differs across compilers and the behavioural tests state the required properties directly); operational (indicator meters T-015, real-time safety T-011 and T-855; logging, monitoring and deployment tests do not apply: `software.deploys` is false); security (hostile input T-007, T-812, T-853, T-856; memory safety T-CI-SAN; supply chain T-905; dependency vulnerabilities T-CI-ADVISORIES); performance (T-PERF-1).

### C++ tests (`tests/core`, `tests/vst3`)

| ID | Name | Category | SC | Decisions | Passes from step |
|---|---|---|---|---|---|
| T-705 | Chorus LFO sweeps the delay inside the range; six wave settings differ | unit (behaviour) | SC-2 | D-026, D-036, D-014 | S-015 |
| T-003 | normalisation maps follow the three tapers | unit | SC-1 | D-011 | S-010 |
| T-005 | prepare validates the sample rate | unit | SC-1, SC-7 | D-011, D-012 | S-018 |
| T-603 | OscBox Wave morphs triangle, sawtooth, square, narrow pulse | unit (behaviour) | SC-2 | D-025, D-035 | S-017 |
| T-015 | meters report level and state | operational (indicators) | SC-1 | D-020..D-026 | S-018 |
| T-103 | Lowpass self-oscillates at the cutoff frequency when resonance is 10 | unit (behaviour) | SC-2 | D-020, D-030 | S-011 |
| T-306 | Phaser Sweep In moves the second six stages in 12-stage mode only | unit (behaviour) | SC-2 | D-022, D-032 | S-013 |
| T-108 | Drive spans -7 dB to +28 dB into a soft limiter | unit (behaviour) | SC-2 | D-020, D-030 | S-011 |
| T-109 | Lowpass: self-oscillation fades out above 16 kHz instead of whistling at the limit | unit (behaviour) | SC-2 | D-020, D-030 | S-011 |
| T-110 | Lowpass 2-pole mode keeps the four-stage resonance loop (self-oscillates at the cutoff) | unit (behaviour) | SC-2 | D-020, D-027 | S-011 |
| T-101 | Lowpass 4-pole response without resonance | unit (behaviour) | SC-2 | D-020, D-030 | S-011 |
| T-502 | FilterBank stereo split: odd filters left, even filters right; sliders | unit (behaviour) | SC-2 | D-024, D-034, D-014 | S-016 |
| T-507 | FilterBank LFO/Sweep shifts all centre frequencies by up to half an | unit (behaviour) | SC-2 | D-024, D-034, D-014 | S-016 |
| T-021 | lfoShape produces the six documented shapes | unit | SC-2 | D-014, D-015, D-024, D-037 | S-010 |
| T-506 | FilterBank Rate, tap tempo (taps are quarter notes, steps are eighths) | unit (behaviour) | SC-2 | D-024, D-034, D-014 | S-016 |
| T-405 | Delay LFO modulates the delay time; Amount 0 leaves the pitch alone | unit (behaviour) | SC-2 | D-023, D-033, D-014 | S-014 |
| T-406 | Delay tap tempo: three taps set the delay time until the Time knob | unit (behaviour) | SC-2 | D-023, D-033, D-014 | S-014 |
| T-301 | Phaser: three dips with 6 stages, six with 12, at the all-pass chain's | unit (behaviour) | SC-2 | D-022, D-032 | S-013 |
| T-009 | silence in gives silence at the main output with default settings | unit | SC-1, SC-7 | D-011, D-012 | S-018 |
| T-020 | softClip is 2*tanh(x/2) | unit | SC-2 | D-014, D-015, D-024, D-037 | S-010 |
| T-604 | OscBox: the oscillator's loudness follows the input envelope; Envelope | unit (behaviour) | SC-2 | D-025, D-035 | S-017 |
| T-307 | Phaser LFO Amount sweeps the dips by up to +/-3 octaves; Output scales | unit (behaviour) | SC-2 | D-022, D-032 | S-013 |
| T-002 | table lookups reject unknown ids | contract | SC-1 | D-011, D-020..D-026 | S-010 |
| T-105 | Lowpass resonance: loop gain k = resonance / 2 | unit (behaviour) | SC-2 | D-020, D-030 | S-011 |
| T-008 | processing is deterministic and reset restores the initial audio state | unit | SC-1, SC-7 | D-011, D-012 | S-018 |
| T-107 | Lowpass Mix crossfades from the driven dry signal to the filter | unit (behaviour) | SC-2 | D-020, D-030 | S-011 |
| T-201 | RingMod produces only sum and difference frequencies | unit (behaviour) | SC-2 | D-021, D-031 | S-012 |
| T-205 | RingMod external carrier replaces the internal oscillator in the | unit (behaviour) | SC-2 | D-021, D-031 | S-012 |
| T-701 | Chorus delay ranges: Flange 0.6-10 ms, Chorus 5-50 ms | unit (behaviour) | SC-2 | D-026, D-036, D-014 | S-015 |
| T-302 | Phaser Aux Out has peaks where Audio Out has dips | unit (behaviour) | SC-2 | D-022, D-032 | S-013 |
| T-707 | Chorus feedback insert and wet bandwidth | unit (behaviour) | SC-2 | D-026, D-036, D-014 | S-015 |
| T-708 | Chorus repeats darken: the loop passes through the wet filter | unit (behaviour) | SC-2 | D-026, D-036 | S-015 |
| T-602 | OscBox Frequency CV is 1 V per octave and reaches up to 20 kHz | unit (behaviour) | SC-2 | D-025, D-035 | S-017 |
| T-402 | Delay Feedback: loop gain is feedback / 8 | unit (behaviour) | SC-2 | D-023, D-033, D-014 | S-014 |
| T-812 | RackBridge connection rules and bad voltages | integration (host adaptor) | SC-5, SC-7 | D-012, D-013 | S-018 |
| T-503 | FilterBank sequencer: the first step falls on the first sample after | unit (behaviour) | SC-2 | D-024, D-034, D-014 | S-016 |
| T-204 | RingMod LFO Amount is the peak-to-peak carrier sweep in octaves | unit (behaviour) | SC-2 | D-021, D-031 | S-012 |
| T-102 | Lowpass 2-pole response without resonance | unit (behaviour) | SC-2 | D-020, D-030 | S-011 |
| T-305 | Phaser Resonance raises the peaks and leaves the dips in place | unit (behaviour) | SC-2 | D-022, D-032 | S-013 |
| T-001 | effect tables equal the frozen fixture | contract | SC-1 | D-011, D-020..D-026 | S-010 |
| T-407 | Delay bypass: echoes stop, or carry on in Spillover mode | unit (behaviour) | SC-2 | D-023, D-033, D-014 | S-014 |
| T-505 | FilterBank plays every frozen pattern (external Step clock) | golden (pattern table) | SC-2 | D-024, D-034, D-014 | S-016 |
| T-821 | the VST3 path and the Rack path give the same audio | integration (parity) | SC-5 | D-013 | S-018 |
| T-605 | OscBox hard sync locks the oscillator's period to the input | unit (behaviour) | SC-2 | D-025, D-035 | S-017 |
| T-706 | Chorus tap tempo sets one LFO cycle per tapped beat; the Rate meter | unit (behaviour) | SC-2 | D-026, D-036, D-014 | S-015 |
| T-017 | tailSeconds is an upper bound and reports self-oscillation | unit | SC-2 | D-011, D-023, D-026 | S-018 |
| T-004 | parameters start at defaults, clamp, round and ignore bad values | unit | SC-1, SC-7 | D-011, D-012 | S-018 |
| T-408 | Delay feedback insert, Delay Out and Mix | unit (behaviour) | SC-2 | D-023, D-033, D-014 | S-014 |
| T-409 | Delay: shortening the time replays the stored signal faster, for one new delay time | unit (behaviour) | SC-2 | D-023, D-033 | S-014 |
| T-410 | Delay repeats darken: the loop passes through the wet filter | unit (behaviour) | SC-2 | D-023 | S-014 |
| T-704 | Chorus self-oscillates from |Feedback| 0.8 and decays below it | unit (behaviour) | SC-2 | D-026, D-036, D-014 | S-015 |
| T-018 | behaviour is the same at 44.1 kHz and 192 kHz | integration (sample rates) | SC-7 | D-012, D-030, D-033 | S-018 |
| T-023 | filterBankEnvelopeTimes interpolates the frozen table | unit | SC-2 | D-014, D-015, D-024, D-037 | S-010 |
| T-012 | a normalised CV input sweeps depth x CV of the knob range and is | unit | SC-2 | D-012 | S-018 |
| T-702 | Chorus stereo: the right output carries the delayed signal inverted | unit (behaviour) | SC-2 | D-026, D-036, D-014 | S-015 |
| T-501 | FilterBank centre frequencies, unity centre gain and Q = 5 | unit (behaviour) | SC-2 | D-024, D-034, D-014 | S-016 |
| T-024 | the frozen pattern set is well formed | contract (fixture) | SC-2 | D-014, D-015, D-024, D-037 | S-010 |
| T-016 | reported latency matches the dry path | unit | SC-5 | D-011 | S-018 |
| T-013 | a disconnected input reads as zero | unit | SC-2 | D-012 | S-018 |
| T-010 | bypass passes the input: drive stays in the path of Lowpass and RingMod | unit | SC-2 | D-018 | S-018 |
| T-203 | RingMod LFO: 0.1-25 Hz, sine or square, LFO Out is +/-1.5 V whatever | unit (behaviour) | SC-2 | D-021, D-031 | S-012 |
| T-304 | Phaser LFO: triangle, Lo 0.01-2.5 Hz, Hi 1-250 Hz, LFO Out +/-2.8 V | unit (behaviour) | SC-2 | D-022, D-032 | S-013 |
| T-206 | RingMod Mix at zero is the driven dry signal | unit (behaviour) | SC-2 | D-021, D-031 | S-012 |
| T-703 | Chorus feedback is bipolar: all harmonics of 1/delay, or odd harmonics | unit (behaviour) | SC-2 | D-026, D-036, D-014 | S-015 |
| T-404 | Delay bandwidth: 3.0 kHz in Short, 1.7 kHz in Long, two poles | unit (behaviour) | SC-2 | D-023, D-033, D-014 | S-014 |
| T-504 | FilterBank envelope shapes follow the Envelope control | unit (behaviour) | SC-2 | D-024, D-034, D-014 | S-016 |
| T-104 | Lowpass does not self-oscillate at resonance 7 | unit (behaviour) | SC-2 | D-020, D-030 | S-011 |
| T-802 | BlockRunner channel rules: stereo in is averaged, mono out sums a split | integration (host adaptor) | SC-5 | D-013 | S-018 |
| T-606 | OscBox FM Amount and Env Amount move the oscillator frequency | unit (behaviour) | SC-2 | D-025, D-035 | S-017 |
| T-607 | OscBox: wraps, pulse edges and hard-sync resets are band-limited | unit (behaviour) | SC-2 | D-025, D-035 | S-017 |
| T-022 | TapTempo needs three taps, averages, and restarts after five idle | unit | SC-2 | D-014, D-015, D-024, D-037 | S-010 |
| T-601 | OscBox frequency range 25 Hz-1.6 kHz; Oscillator Out is +/-2.5 V with | unit (behaviour) | SC-2 | D-025, D-035 | S-017 |
| T-303 | Phaser Sweep spans six octaves around 1.1 kHz | unit (behaviour) | SC-2 | D-022, D-032 | S-013 |
| T-014 | a parameter step is smoothed: no click | unit | SC-2 | D-017 | S-018 |
| T-011 | the audio path performs no heap allocation | operational (real-time safety) | SC-8 | D-011 | S-018 |
| T-106 | Lowpass envelope follower: level, speeds and cutoff modulation | unit (behaviour) | SC-2 | D-020, D-030 | S-011 |
| T-403 | Delay self-oscillates above Feedback 8 and stays bounded | unit (behaviour) | SC-2 | D-023, D-033, D-014 | S-014 |
| T-007 | outputs stay finite and bounded for any parameters, inputs and sample | security / robustness | SC-7 | D-012 | S-018 |
| T-006 | process before prepare writes zeros | unit | SC-1, SC-7 | D-011, D-012 | S-018 |
| T-401 | Delay time: Short 40-400 ms, Long 80-800 ms, logarithmic knob | unit (behaviour) | SC-2 | D-023, D-033, D-014 | S-014 |
| T-811 | RackBridge scales volts: 5 V is one core unit on every port | integration (host adaptor) | SC-5, SC-7 | D-012, D-013 | S-018 |
| T-202 | RingMod carrier ranges: Lo 0.6-80 Hz, Hi 30 Hz-4 kHz; Carrier Out is +/ | unit (behaviour) | SC-2 | D-021, D-031 | S-012 |
| T-801 | BlockRunner equals per-sample processing and does not depend on block | integration (host adaptor) | SC-5 | D-013 | S-018 |
| T-853 | state round-trips and hostile state is ignored | security | SC-7 | D-016 | S-020 |
| T-855 | processBlock performs no heap allocation | operational (real-time safety) | SC-8 | D-016 | S-020 |
| T-852 | processBlock equals the core driven with the host's parameter values | integration | SC-5 | D-016 | S-020 |
| T-856 | output stays finite for hostile audio input | security | SC-7 | D-016 | S-020 |
| T-854 | bus layouts: mono or stereo in, never fewer outputs than inputs | integration | SC-3 | D-016 | S-020 |
| T-851 | the host parameters mirror the core tables | contract | SC-1, SC-3 | D-016 | S-020 |

### Repository tests (`tests/py/test_repo.py`)

| ID | Methods | Category | SC | Decisions | Passes from step |
|---|---|---|---|---|---|
| T-901 | test_no_third_party_marks_in_the_product; test_product_names_come_from_the_fixture | contract (naming) | SC-9 | D-004, D-005 | S-021, S-032, S-035 |
| T-902 | test_manifest (Rack plugin.json) | contract | SC-4 | D-006, D-004 | S-032 |
| T-903 | test_panels (Rack panel SVG rules) | contract | SC-4 | D-006 | S-031 |
| T-904 | test_core_tables_are_generated_from_the_fixture | contract | SC-1 | D-011 | S-010, S-018, S-020 |
| T-905 | test_dependency_lock; test_workflows | security (supply chain) / operational | SC-3, SC-4 | D-003, D-007, D-009, D-060 | S-001, S-030, S-MUTATE |
| T-906 | test_spdx_headers_and_licence_text | contract (licence) | SC-9 | D-001 | S-035 |
| T-907 | test_readme_and_user_guide | contract (documentation) | SC-1 | D-004, D-027 | S-035 |

### Checks that run only in CI or as a benchmark

| ID | Check | Category | SC | Decisions | Step |
|---|---|---|---|---|---|
| T-PERF-1 | `muff_bench --limit 5` on `core-linux`: each effect under 5 % of one core at 48 kHz, 64-sample blocks | performance | SC-6 | D-050 | S-018 |
| T-CI-SAN | `muff_core_tests` under AddressSanitizer and UndefinedBehaviourSanitizer (`core-sanitizers`) | security (memory safety) | SC-7 | D-009 | S-018 |
| T-CI-PLUGINVAL | pluginval v1.0.4 at strictness 10 on each of the seven VST3 plug-ins (Linux, Windows, macOS arm64) with the log rule of D-007 | integration (host validation) | SC-3 | D-007 | S-021 |
| T-CI-RACKPKG | `tools/check_rack_package.py` on the package built by each `rack-*` job | integration (packaging) | SC-4 | D-006, D-003 | S-033 |
| T-CI-ADVISORIES | `tools/check_advisories.py deps.lock.json` (`deps-scan`) | security (dependency vulnerabilities) | SC-3 | D-061 | S-034 |

## Test-quality self-audit (protocol 2.3)

- Every test asserts on behaviour visible through a public header, the fixture or a repository file; none asserts on private structure.
- Expected values come from the decisions' formulas or from analytic results (for example the gain of four cascaded one-poles, the phase of an all-pass chain, the product of two sines), computed inside the test, not from the oracle's output.
- Tolerances are stated next to each assertion with the analytic value they surround.
- Each test was shown to fail (against the stubs) and to pass (against an independent oracle), the C++ core tests ran clean under AddressSanitizer and UndefinedBehaviourSanitizer and also pass when built with Rack's compiler flags: `research/spikes/red-verification/RESULT.md`.
- Known limits of the suite, accepted: aliasing of the drive stage and of the ladder's saturator is not measured (D-030, D-037; question L-3); interpolation quality of the delay lines is not measured; the oscillator's band-limiting is measured without FM only (T-607); the Rack module shells have no automated behavioural test (R-012); the VST3 editor is exercised only by pluginval.
