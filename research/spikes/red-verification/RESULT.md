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
