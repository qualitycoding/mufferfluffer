# Spike: the frozen tests are satisfiable (green against an oracle) and fail against the stubs (red)

Date 2026-11-11, planning sandbox (Ubuntu 24.04, 2 vCPU, GCC 13.3.0, CMake 3.28.3, Ninja, Python 3.13).
Dependencies from local checkouts: Catch2 v3.16.1 (`0809213`), JUCE 9.0.5 (`d60b7a7`).

## What was run
| # | Build | Command | Result |
|---|---|---|---|
| 1 | Core + adaptor tests against the **oracle** (`research/spikes/oracle/`) | `cmake -S . -B build/oracle -G Ninja -DCMAKE_BUILD_TYPE=Release -DMUFF_IMPL_DIR=$PWD/research/spikes/oracle -DFETCHCONTENT_SOURCE_DIR_CATCH2=<Catch2>`; `cmake --build build/oracle -j2`; `build/oracle/tests/muff_core_tests` | exit 0, "All tests passed (299243 assertions in 77 test cases)", 14 s |
| 2 | Same tests against the **stubs** (`core/src/stubs.cpp`, `adapters/src/stubs.cpp`) | same without `-DMUFF_IMPL_DIR` | exit 42, "test cases: 77 \| 77 failed"; JUnit report: 73 cases fail by the stub's `NotImplemented` exception, 4 by an assertion on a stub return value; no crash, no hang, no skipped case |
| 3 | Oracle under AddressSanitizer + UndefinedBehaviourSanitizer | as 1 with `-DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" -DMUFF_SANITIZER_BUILD=ON` | exit 0, "All tests passed (299236 assertions in 76 test cases)" (T-011 is compiled out in sanitizer builds), 26 s |
| 4 | VST3 shell tests against the **oracle shell** (`research/spikes/oracle_vst3/`) + oracle core | `cmake -S . -B build/vst-oracle -G Ninja -DMUFF_BUILD_VST3=ON -DMUFF_IMPL_DIR=... -DMUFF_VST3_IMPL_DIR=$PWD/research/spikes/oracle_vst3 -DFETCHCONTENT_SOURCE_DIR_JUCE=<JUCE> ...`; `cmake --build build/vst-oracle --target muff_vst3_tests -j2` (2 min 47 s); run the binary | exit 0, "All tests passed (1563 assertions in 6 test cases)", with and without `xvfb-run` |
| 5 | VST3 shell tests against the **stubs** | same without the two `IMPL_DIR` options | exit 42, "test cases: 6 \| 6 failed" |
| 6 | Repository tests against a synthetic satisfying tree | `python3 research/spikes/oracle_repo/make_tree.py . /tmp/oracle_tree`; `MUFF_REPO_ROOT=/tmp/oracle_tree python3 -m unittest discover -s tests/py` | "Ran 9 tests ... OK" |
| 7 | Repository tests against this repository | `python3 -m unittest discover -s tests/py` | "FAILED (failures=9)", each with a "missing deliverable" or equivalent message |
| 8 | Benchmark against the stubs | `build/stubs/tests/muff_bench --seconds 1` | exit 3, "error: BlockRunner" (clean failure) |

## Benchmark (oracle, `muff_bench --seconds 10`, best of three, per cent of one core)
lowpass 0.655, ringmod 0.575, phaser 0.631, delay 0.889, filterbank 0.937, oscbox 0.408, chorus 0.874. Exit 0 against the 5 % limit.
The oracle is unoptimised and has no oversampling; the 5 % limit of A-011 leaves a factor of five for the deliverable and for slower runners.

## Defects this spike found before the freeze
First oracle run: 67 of 77 passed. Of the 10 failures, 8 were defects in the draft tests (a comparison made while a smoothed control was still moving; three clock-relative sources given absolute sample indices; an exact float equality after smoothing; a tap-tempo sequence that did not respect the five-second idle rule it tests elsewhere; a burst detector whose window was shorter than one cycle of the test tone; an aux-output threshold placed above the analytic value) and 2 were defects in the oracle that exposed an under-specified decision (the envelope follower's scaling, now fixed in D-030 as "no scaling"; the phaser's output mix taken from the dry input instead of the feedback sum, which filled in the dips at high resonance, now fixed in D-032).
The sanitizer run then found one more test defect: T-503 read past the end of its output buffer (rendered 17 steps, read up to step 22.7). Fixed by rendering 24 steps.

## What this does not show
The oracle is not a quality bar: it has no oversampling, linear delay interpolation and a generic editor. The deliverable is written under the plan's steps, review and mutation gates. Rack module shells and the seven plug-in targets are not built here.

## Update after the adversarial pass (2026-11-12)
The adversarial review (`research/rounds/R5-adversarial.md`) led to changes in decisions D-012, D-016, D-020, D-033, D-035 and D-050, three new tests (T-109, T-409, T-607), stricter assertions in T-017, T-812, T-854 and T-856, a second (silence) phase in the benchmark, and matching changes to the oracle (clocked delay line, resonance fade, flush-to-zero during `process`, one-sample-late oscillator with sub-sample sync, +/-12 V limit in the Rack bridge). Re-run on the final test files:

| Build | Result |
|---|---|
| Core + adaptor tests against the oracle | exit 0, "All tests passed (299407 assertions in 80 test cases)" |
| Same against the stubs | "test cases: 80 \| 80 failed" |
| Oracle under AddressSanitizer + UndefinedBehaviourSanitizer | "All tests passed (299400 assertions in 79 test cases)" |
| Oracle built with Rack's flags (`-O3 -march=nehalem -funsafe-math-optimizations -fno-omit-frame-pointer`) | "All tests passed (299407 assertions in 80 test cases)" |
| VST3 shell tests against the oracle shell | "All tests passed (1591 assertions in 6 test cases)" |
| VST3 shell tests against the stubs | "test cases: 6 \| 6 failed" |
| Benchmark against the oracle, `--seconds 5`, slower of tone and silence phases | lowpass 1.018, ringmod 0.711, phaser 0.734, delay 0.903, filterbank 1.879, oscbox 0.649, chorus 1.024 (per cent of one core); exit 0 |
| Benchmark against the stubs | exit 3 |

Oscillator aliasing measured on the oracle (strongest alias between 200 Hz and 6 kHz relative to the strongest of the first 20 harmonics, 48 kHz): without sync -61 to -76 dB; with sub-sample-timed sync -66 to -74 dB, except the synced 10 % pulse at 1333 Hz: -46.5 dB; with sync quantised to the sample grid (the first oracle): -30 to -37 dB.
The numbers in the earlier sections of this file describe the first freeze candidate (77 tests) and are kept as the record of what was run then.

## Re-freeze after pre-mortem round 1 (2026-11-12)
Tests added: T-110 (Lowpass 2-pole self-oscillation), T-410 (Delay repeats darken), T-708 (Chorus repeats darken). Test amended: T-304 (the 250 Hz case now counts cycles; its slope measure failed for a rate 0.02 % off although the stated tolerance is 2 %).

| Build | Result |
|---|---|
| Oracle, Release | "All tests passed (299451 assertions in 83 test cases)" |
| Oracle, AddressSanitizer + UndefinedBehaviourSanitizer | "All tests passed (299444 assertions in 82 test cases)" |
| Oracle, Rack compiler flags | "All tests passed (299451 assertions in 83 test cases)" |
| Stubs | "test cases: 83 | 83 failed" |
| Mutant of the oracle whose Delay and Chorus loops feed back the unfiltered line read | "test cases: 83 | 81 passed | 2 failed": exactly T-410 and T-708 fail (measured low-minus-high step 0.03 dB and 0.00 dB against the required 1.8 dB) |

## Re-freeze after pre-mortem round 2 (2026-11-13)
Tests added: T-308, T-411, T-508, T-509 (core), T-857 (VST3 shell). Test amended: T-905 `test_workflows` (the `aggregate` job needs a job-level `if: always()`).

| Build | Result |
|---|---|
| Oracle, Release | "All tests passed (299647 assertions in 87 test cases)" |
| Oracle, AddressSanitizer + UndefinedBehaviourSanitizer | "All tests passed (299640 assertions in 86 test cases)" |
| Oracle, Rack compiler flags | "All tests passed (299647 assertions in 87 test cases)" |
| Stubs | "test cases: 87 | 87 failed" |
| Oracle VST3 shell | "All tests passed (1612 assertions in 7 test cases)" |
| Stub VST3 shell | "test cases: 7 | 7 failed" |
| Repository tests on the synthetic tree | "OK" (9 tests); with the `aggregate` job's `if: always()` moved to a step: `test_workflows` fails |

Variants of the oracle, each run against the whole core suite (87 cases):

| Variant | Result |
|---|---|
| Phaser LFO depth one octave instead of three | 86 passed, 1 failed: T-308 |
| Delay LFO depth a quarter of the specified | 86 passed, 1 failed: T-411 |
| FilterBank LFO depth 0.2 octave | 86 passed, 1 failed: T-508 |
| FilterBank LFO depth 0.8 octave | 86 passed, 1 failed: T-508 |
| FilterBank envelope restarts from zero on a trigger | 86 passed, 1 failed: T-509 |
| VST3 shell that always asks the runner for two outputs | 6 passed, 1 failed: T-857 (filter bank) |

## Re-freeze after pre-mortem round 3 (2026-11-13)
Test amended: T-308 finds the LFO's top by its signed maximum (the helper it used before takes the largest absolute value, which made the test fail for a correct implementation whose LFO phase is held in single precision). Re-run: oracle 87 of 87 (86 under the sanitizers, 87 with Rack flags); the one-octave-depth variant still fails exactly T-308; stubs 87 failed. The round 3 reviewer ran the same change against a single-precision-phase variant and a variant with the rate 3e-7 low: both pass.

## Re-freeze after the assumption audit (2026-11-13)
Test amended: T-902 `test_manifest` now fails if any module carries the tag "Hardware clone" (it required the tag before; D-072 item 1). Repository tests on the synthetic tree without the tag: "OK" (9 tests); with the tag added to one module: `test_manifest` fails; on the real branch: 9 fail (nothing is built yet). No C++ test changed.
