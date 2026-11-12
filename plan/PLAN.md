# Execution plan

Read `HANDOFF.md` first. Step template: protocol 3.1. All paths are relative to the repository root. "The suite" means `muff_core_tests` (77 cases), `muff_vst3_tests` (6 cases) and `tests/py` (9 cases). Commands are for Linux unless a step says otherwise; exact build commands are in `plan/ENVIRONMENT.md`. A filter such as `"[lowpass]"` is passed to the test binary: `build/core/tests/muff_core_tests "[lowpass]"`.

Rules that apply to every step (not repeated below):
- Never edit a file listed in `tests/FROZEN_MANIFEST.sha256`; `python3 tools/check_frozen.py` must exit 0 before every commit.
- After every step: append a line to `EXECUTION_LOG.md`, commit, push, and check that `git ls-remote origin refs/heads/<branch>` equals `git rev-parse HEAD` (Rule 6).
- A step is done only when its "Done when" commands were run on the pushed commit and gave the stated result, and every file it lists under Outputs exists and is non-empty (Rule 14).
- Unanticipated situation: the default rule in `HANDOFF.md` section 7.
- Tiers name the least capable model allowed; a more capable one may always be used.
- Workflows are triggered by `push` (optionally selected by a marker in the head commit's message), because `workflow_dispatch` and `schedule` only work for workflow files that exist on the default branch (`main`), and the implementation branch is not the default branch. Where a step adds `workflow_dispatch` or `schedule` as well, those triggers become live only after the owner merges the pull request of S-FINAL.
- In workflow files, quote every `${{ ... }}` expression that appears inside a YAML flow sequence or mapping, and give shell steps `shell: bash` with `set -euo pipefail` as their first line.
- A gate's `GATE-<id>.md` and its `GATE-<id>.RESPONSE.md` are committed in the same commit as the first change that applies the response.

Step order: S-000, S-001, S-010 to S-018, S-020, S-021, S-030 to S-035, S-REVIEW, S-ADVERSARIAL, S-MUTATE, S-040, S-LISTEN, S-FINAL, S-RETRO, S-KNOW.
Independent groups that may run concurrently once their dependencies pass: {S-011, S-012, S-013, S-014, S-015, S-016, S-017} (each owns one source file); {S-020} and {S-030, S-031} after S-018.

---

### S-000 Environment and access verification
- Tier: Haiku
- Profile: software
- Depends on: none
- Inputs: the generation branch named in `HANDOFF.md`; `plan/ENVIRONMENT.md`; `tests/FROZEN_MANIFEST.sha256`.
- Actions:
  1. `git fetch origin && git checkout -b impl-<run_id> origin/<generation branch>` where `<run_id>` is the generation branch name without its `gen-` prefix.
  2. Install the Linux packages of `plan/ENVIRONMENT.md` (JUCE list). Record `cmake --version`, `g++ --version`, `python3 --version`, `nproc`.
  3. `python3 tools/check_frozen.py` (expect exit 0 and the line `frozen files intact: <n> files`).
  4. Red baseline: run the three "core + adaptor tests" commands of `plan/ENVIRONMENT.md`; expect `ctest` to report 77 of 77 tests failed. Run `python3 -m unittest discover -s tests/py`; expect `FAILED (failures=9)`.
  5. Access checks: `git push -u origin impl-<run_id>` (push permission); `gh api repos/qualitycoding/mufferfluffer/actions/runs --jq '.total_count'` or the equivalent GitHub tool (permission to read Actions runs; needed from S-001 on); `git ls-remote https://github.com/juce-framework/JUCE 9.0.5` and `git ls-remote https://github.com/catchorg/Catch2 v3.16.1` (reachability; expect the commits of `plan/ENVIRONMENT.md`).
  6. Create `EXECUTION_LOG.md` with the header `| UTC time | Step | Attempt | Outcome | Commit | Note |` and this step's line. Create empty `DEVIATIONS.md` with the header `| UTC time | Step | Deviation | Reason | Reversible? |`.
- Outputs: branch `impl-<run_id>`; `EXECUTION_LOG.md`; `DEVIATIONS.md`.
- Evidence produced: none
- Done when: actions 3, 4 and 5 gave the expected results; both files exist and are pushed.
- Checkpoint: `.checkpoints/state.json`: `phase: "impl"`, `active_step_id: "S-000"`, implementer environment facts.
- On failure: any check fails: write `BLOCKED.md` listing exactly what is missing and stop before any delivery work. If the red baseline shows any passing test, stop and write `BLOCKED.md` (the stubs or the tests were altered).
- Gate: none
- Relevant decisions/claims: D-008, D-010; C-PLANNER-001, C-PLANNER-005
- Lessons applied: L-20261111T151500Z-download-output-committed-unchecked (assert before commit)
- Exclusive resources: none

### S-001 Build skeleton and first CI jobs
- Tier: Sonnet
- Profile: software
- Depends on: S-000
- Inputs: `CMakeLists.txt`, `tests/CMakeLists.txt`, `plan/ENVIRONMENT.md` (pins, labels), `plan/DECISIONS.md` D-009.
- Actions:
  1. Create `.gitignore` with the lines `build/`, `Rack-SDK/`, `rack/build/`, `rack/dist/`, `rack/plugin.*` (not `plugin.json`: add the negation line `!rack/plugin.json`), `__pycache__/`, `*.pyc`.
  2. Create `deps.lock.json` with the keys `juce`, `catch2`, `pluginval`, `rack-sdk` holding the versions, commits and the three pluginval hashes of `plan/ENVIRONMENT.md`; give `rack-sdk` `"version": "2.6.6"` and `"sha256": {}` (filled by S-030).
  3. Create `.github/workflows/ci.yml`: `on: [push, pull_request]`; top-level `permissions:` with `contents: read`; `concurrency` group `ci-${{ github.ref }}` with `cancel-in-progress: true`; a top-level `env:` entry `MUFF_TEST_FILTER: "T-00[1-3]*,T-02*"` (the temporary filter, widened by later steps and removed by S-018); the jobs `repo-checks`, `core-linux`, `core-sanitizers`, `core-linux-arm64`, `core-musl`, `core-oldest-toolchain`, `core-windows`, `core-macos`, `core-macos-intel` and `aggregate` exactly as tabled in D-009, except that until S-018 each core job runs `muff_core_tests "$MUFF_TEST_FILTER"` instead of `ctest` and does not run `muff_bench`, and `repo-checks` runs only `python3 tools/check_frozen.py` and `python3 tools/gen_expected.py --check tests/fixtures/effects.json tests/core/expected_effects.inc`. Every `uses:` is pinned to the commit in `plan/ENVIRONMENT.md`. Every job sets `timeout-minutes`. Each job's first step prints the tool versions it relies on. Test logs are written with `2>&1 | tee <job>.log` under `shell: bash` with `set -o pipefail`, and uploaded with `actions/upload-artifact` under `if: always()` and `if-no-files-found: error`.
  4. `aggregate`: `if: always()`, `needs:` all other jobs, one step that fails unless every entry of `${{ toJSON(needs) }}` has `"result": "success"` (use `python3 -c` reading the JSON from an environment variable).
  5. Push and read the run. The core is still stubs at this step, so the test step of each core job is expected to fail; what this step proves is that every job builds and reaches its test step on every platform. Record each job's conclusion and the name of its failing step in `EXECUTION_LOG.md`.
- Outputs: `.gitignore`, `deps.lock.json`, `.github/workflows/ci.yml`.
- Evidence produced: none
- Done when: `python3 tools/check_frozen.py` exits 0; the pushed commit's workflow run shows, for each of the eight core jobs, the build step succeeded and the test step ran (its log contains `test cases:`); `repo-checks` is green.
- Checkpoint: `active_step_id: "S-001"`, run URL.
- On failure: a job fails before its test step: fix the workflow or the build wiring (not the frozen `tests/CMakeLists.txt`); three attempts per job, then `BLOCKED.md`. A runner label that GitHub rejects: stop and write `BLOCKED.md` (labels are frozen by T-905's job list and D-009).
- Gate: none
- Relevant decisions/claims: D-009, D-005; C-PLANNER-009, C-PLANNER-010, C-PLANNER-011
- Lessons applied: lessons on swallowed failures and unpinned actions listed in `research/PRIOR_KNOWLEDGE.md` rows "CI honesty" and "pins"
- Exclusive resources: `.github/workflows/ci.yml`

### S-010 Core foundation: tables, lookups, normalisation, building blocks
- Tier: Sonnet
- Profile: software
- Depends on: S-001
- Inputs: `core/include/muff/core.hpp`, `core/include/muff/blocks.hpp`, `tests/fixtures/effects.json`, `tools/gen_tables.py`, D-011, D-014, D-015, D-017.
- Actions:
  1. `python3 tools/gen_tables.py tests/fixtures/effects.json core/src/generated_tables.inc`; commit the output.
  2. Delete `core/src/stubs.cpp`. Create `core/src/tables.cpp` (includes `generated_tables.inc` once; implements `effectInfo`, `findParam`, `findInput`, `findOutput`, `findMeter`, `toNormalised`, `fromNormalised` exactly as the header comments specify), `core/src/blocks.cpp` (`softClip`, `lfoShape`, `TapTempo`, `filterBankEnvelopeTimes`), `core/src/effect_base.hpp` and `.cpp` (a private base class providing: parameter storage with clamping and rounding; connection flags; input sanitising and output limiting of D-012; 5 ms smoothing of D-017; the `active` cross-fade; the level meter; `prepare` validation), and `core/src/create.cpp` (`createEffect`: throws `std::out_of_range` for an invalid id; for a valid id whose effect is not yet written, throws `muff::NotImplemented`).
  3. Every new file starts with `// SPDX-License-Identifier: AGPL-3.0-only`.
- Outputs: the files named above.
- Evidence produced: T-001, T-002, T-003, T-020, T-021, T-022, T-023, T-024
- Done when: `build/core/tests/muff_core_tests "T-001*,T-002*,T-003*,T-020*,T-021*,T-022*,T-023*,T-024*"` reports `All tests passed` with 8 test cases; `python3 tools/check_frozen.py` exits 0; the CI run of the pushed commit is green in all core jobs with the filter of S-001.
- Checkpoint: `active_step_id: "S-010"`.
- On failure: fix and re-run; a test that seems wrong: `HANDOFF.md` section 6 (test challenge), never edit the test.
- Gate: none
- Relevant decisions/claims: D-011, D-012, D-014, D-015, D-017
- Lessons applied: see `research/PRIOR_KNOWLEDGE.md` row "single source of truth for tables"
- Exclusive resources: none

### S-011 Lowpass
- Tier: Opus
- Profile: software
- Depends on: S-010
- Inputs: D-018, D-020, D-030, D-037; `research/spikes/oracle/oracle_core.cpp` may be read as a satisfiability reference (it is not a quality bar and must not be copied wholesale: it lacks the structure of S-010).
- Actions: create `core/src/lowpass.cpp` implementing the effect per D-020 and D-030 on the base class of S-010; register it in `core/src/create.cpp`. `tailSeconds()`: `1.0e9f` when `resonance >= 8`, else `1.0f`. `latencySamples()`: 0. In `.github/workflows/ci.yml` append `,[lowpass]` to `MUFF_TEST_FILTER`.
- Outputs: `core/src/lowpass.cpp`; edits to `create.cpp`, `ci.yml`.
- Evidence produced: T-101 to T-108
- Done when: `build/core/tests/muff_core_tests "[lowpass]"` reports `All tests passed` with 8 test cases; CI green in all core jobs.
- Checkpoint: `active_step_id: "S-011"`.
- On failure: three implementation attempts; if T-103 (self-oscillation frequency) cannot be met within 2 % with the D-030 equations, stop and write `BLOCKED.md` quoting the measured frequencies (the oracle meets it, so this indicates an implementation defect; do not change the algorithm family without re-opening Phase 2).
- Gate: none
- Relevant decisions/claims: D-020, D-030; C-DSP-008, C-DSP-010, C-DSP-011, C-PEDA-002
- Lessons applied: none specific
- Exclusive resources: none (the edits to `create.cpp` and `ci.yml` are one line each; if S-011 to S-017 run concurrently, each rebases before pushing)

### S-012 RingMod
- Tier: Sonnet
- Profile: software
- Depends on: S-010
- Inputs: D-018, D-021, D-031, D-015.
- Actions: create `core/src/ringmod.cpp` per D-021; register it; `tailSeconds()` returns `0.1f`; append `,[ringmod]` to `MUFF_TEST_FILTER`.
- Outputs: `core/src/ringmod.cpp`; edits to `create.cpp`, `ci.yml`.
- Evidence produced: T-201 to T-206
- Done when: `muff_core_tests "[ringmod]"`: `All tests passed`, 6 test cases; CI green in all core jobs.
- Checkpoint: `active_step_id: "S-012"`.
- On failure: as S-011 (three attempts, then `BLOCKED.md`).
- Gate: none
- Relevant decisions/claims: D-021, D-031
- Lessons applied: none specific
- Exclusive resources: none

### S-013 Phaser
- Tier: Sonnet
- Profile: software
- Depends on: S-010
- Inputs: D-018, D-022, D-032.
- Actions: create `core/src/phaser.cpp` per D-022 and D-032; register it; `tailSeconds()` returns `0.5f`; append `,[phaser]` to `MUFF_TEST_FILTER`.
- Outputs: `core/src/phaser.cpp`; edits to `create.cpp`, `ci.yml`.
- Evidence produced: T-301 to T-307
- Done when: `muff_core_tests "[phaser]"`: `All tests passed`, 7 test cases; CI green in all core jobs.
- Checkpoint: `active_step_id: "S-013"`.
- On failure: as S-011.
- Gate: none
- Relevant decisions/claims: D-022, D-032
- Lessons applied: none specific
- Exclusive resources: none

### S-014 Delay
- Tier: Opus
- Profile: software
- Depends on: S-010
- Inputs: D-014, D-015, D-017, D-018, D-023, D-033.
- Actions: create `core/src/delay.cpp` per D-023 and D-033 (delay-line buffer sized in `prepare`, never in `process`); register it; `tailSeconds()`: with `g = feedback / 8`: `1.0e9f` if `g >= 1`; `0.9f` if `g <= 0.001`; else `0.8f * (1 + 6.908f / -ln(g)) + 0.1f`; append `,[delay]` to `MUFF_TEST_FILTER`.
- Outputs: `core/src/delay.cpp`; edits to `create.cpp`, `ci.yml`.
- Evidence produced: T-401 to T-408
- Done when: `muff_core_tests "[delay]"`: `All tests passed`, 8 test cases; CI green in all core jobs.
- Checkpoint: `active_step_id: "S-014"`.
- On failure: as S-011.
- Gate: none
- Relevant decisions/claims: D-023, D-033; C-PEDB-003, C-PEDB-012
- Lessons applied: none specific
- Exclusive resources: none

### S-015 Chorus
- Tier: Opus
- Profile: software
- Depends on: S-010
- Inputs: D-014, D-015, D-018, D-026, D-036.
- Actions: create `core/src/chorus.cpp` per D-026 and D-036; register it; `tailSeconds()`: with `g = 1.25 * |feedback|`: `1.0e9f` if `g >= 1`; `0.15f` if `g <= 0.001`; else `0.05f * (1 + 6.908f / -ln(g)) + 0.1f`; append `,[chorus]` to `MUFF_TEST_FILTER`.
- Outputs: `core/src/chorus.cpp`; edits to `create.cpp`, `ci.yml`.
- Evidence produced: T-701 to T-707
- Done when: `muff_core_tests "[chorus]"`: `All tests passed`, 7 test cases; CI green in all core jobs.
- Checkpoint: `active_step_id: "S-015"`.
- On failure: as S-011.
- Gate: none
- Relevant decisions/claims: D-026, D-036
- Lessons applied: none specific
- Exclusive resources: none

### S-016 FilterBank
- Tier: Opus
- Profile: software
- Depends on: S-010
- Inputs: D-014, D-018, D-024, D-034; the pattern table generated into `core/src/generated_tables.inc` (`kPatterns`).
- Actions: create `core/src/filterbank.cpp` per D-024 and D-034; register it; `tailSeconds()` returns `4.5f`; append `,[filterbank]` to `MUFF_TEST_FILTER`.
- Outputs: `core/src/filterbank.cpp`; edits to `create.cpp`, `ci.yml`.
- Evidence produced: T-501 to T-507
- Done when: `muff_core_tests "[filterbank]"`: `All tests passed`, 7 test cases; CI green in all core jobs.
- Checkpoint: `active_step_id: "S-016"`.
- On failure: as S-011.
- Gate: none
- Relevant decisions/claims: D-024, D-034; C-PEDB-020, C-PEDB-021
- Lessons applied: none specific
- Exclusive resources: none

### S-017 OscBox
- Tier: Opus
- Profile: software
- Depends on: S-010
- Inputs: D-018, D-025, D-035.
- Actions: create `core/src/oscbox.cpp` per D-025 and D-035 (band-limited steps at wraps, pulse edges and sync resets); register it; `tailSeconds()` returns `1.0f`; append `,[oscbox]` to `MUFF_TEST_FILTER`.
- Outputs: `core/src/oscbox.cpp`; edits to `create.cpp`, `ci.yml`.
- Evidence produced: T-601 to T-606
- Done when: `muff_core_tests "[oscbox]"`: `All tests passed`, 6 test cases; CI green in all core jobs.
- Checkpoint: `active_step_id: "S-017"`.
- On failure: as S-011.
- Gate: none
- Relevant decisions/claims: D-025, D-035; C-DSP-055, C-DSP-061
- Lessons applied: none specific
- Exclusive resources: none

### S-018 Adaptors, whole core suite, sanitizers, performance
- Tier: Sonnet
- Profile: software
- Depends on: S-011, S-012, S-013, S-014, S-015, S-016, S-017
- Inputs: `adapters/include/muff/adapters.hpp`, D-013, D-050.
- Actions:
  1. Delete `adapters/src/stubs.cpp`; create `adapters/src/adapters.cpp` implementing `BlockRunner` and `RackBridge` per the header.
  2. In `ci.yml`: delete the `MUFF_TEST_FILTER` entry; every core job now runs `ctest --test-dir build/core -C Release --output-on-failure`; add `muff_bench --limit 5` to `core-linux` and `muff_bench --limit 0` to `core-linux-arm64`, `core-windows`, `core-macos` (D-009).
  3. Run the sanitizer commands of `plan/ENVIRONMENT.md` locally.
- Outputs: `adapters/src/adapters.cpp`; `ci.yml`.
- Evidence produced: T-004 to T-018, T-801, T-802, T-811, T-812, T-821, T-PERF-1, T-CI-SAN
- Done when: `ctest --test-dir build/core --output-on-failure` reports `100% tests passed, 0 tests failed out of 77`; the sanitizer binary reports `All tests passed` (76 test cases); `build/core/tests/muff_bench --limit 5` exits 0 and prints seven lines; the CI run is green in `repo-checks`, all eight core jobs and `aggregate`; `grep -c MUFF_TEST_FILTER .github/workflows/ci.yml` prints 0.
- Checkpoint: `active_step_id: "S-018"`, the seven benchmark numbers from the `core-linux` log.
- On failure: a common test fails for one effect: return to that effect's step. `muff_bench` at or above 5 % for an effect on `core-linux`: profile that effect (`perf record` or manual timers), optimise (hoist `tan`/`pow` calls to control rate with per-sample linear interpolation of coefficients, at most every 16 samples; never change audible behaviour covered by a test), three attempts, then `BLOCKED.md`; never raise the limit (D-050). The gate failing once and passing on re-run of the same commit: R-009.
- Gate: none
- Relevant decisions/claims: D-013, D-050; C-PLANNER-017, C-PLANNER-018
- Lessons applied: `research/PRIOR_KNOWLEDGE.md` rows "performance targets per runner", "sanitizers"
- Exclusive resources: `.github/workflows/ci.yml`

### S-020 VST3 processor shell
- Tier: Opus
- Profile: software
- Depends on: S-018
- Inputs: `vst3/include/muff/vst3_shell.hpp` (the contract), `vst3/CMakeLists.txt`, D-016; `research/spikes/oracle_vst3/processor.cpp` as a satisfiability reference.
- Actions:
  1. Delete `vst3/src/stubs.cpp`; create `vst3/src/processor.cpp` (the `juce::AudioProcessor` of the contract; parameter values are read from the parameter objects, which are atomics, at the top of `processBlock`; `juce::ScopedNoDenormals` in `processBlock`).
  2. Add job `vst3-linux` to `ci.yml` (D-009): install the JUCE package list, configure with `-DMUFF_BUILD_VST3=ON`, build target `muff_vst3_tests`, run it with `xvfb-run -a`. Add it to `aggregate`'s `needs`.
- Outputs: `vst3/src/processor.cpp`; `ci.yml`.
- Evidence produced: T-851 to T-856
- Done when: `ctest --test-dir build/vst3 -R muff_vst3_tests --output-on-failure` reports `100% tests passed, 0 tests failed out of 1` and the binary's own summary says `All tests passed` with 6 test cases; CI green including `vst3-linux`.
- Checkpoint: `active_step_id: "S-020"`.
- On failure: three attempts; a JUCE API difference from the contract (a method the contract names does not exist in 9.0.5): stop and write `BLOCKED.md` (the oracle shell compiled against 9.0.5, so first diff against it).
- Gate: none
- Relevant decisions/claims: D-016, D-005; C-PLANNER-002, C-PLANNER-016
- Lessons applied: `research/PRIOR_KNOWLEDGE.md` rows "JUCE parameter IDs and version hints", "state restore is hostile input"
- Exclusive resources: `.github/workflows/ci.yml`

### S-021 Editor, seven plug-in targets, validation on all platforms
- Tier: Sonnet
- Profile: software
- Depends on: S-020
- Inputs: D-005 (table of targets), D-007, D-016 (editor description), `plan/ENVIRONMENT.md` (pluginval hashes and URLs).
- Actions:
  1. Create `vst3/src/editor.hpp`, `vst3/src/editor.cpp` (the editor of D-016; `createEditor()` in `processor.cpp` returns it) and `vst3/src/entry/entry.cpp` (D-005).
  2. Below the marked line of `vst3/CMakeLists.txt` add the seven `juce_add_plugin` targets with the properties of D-005; each links `muff_vst3_sources` and `juce::juce_recommended_warning_flags`, adds `src/entry/entry.cpp`, and defines `MUFF_EFFECT_ID`.
  3. Create `tools/fetch_pluginval.py <platform> <dest>`: downloads the archive for `Linux`, `macOS` or `Windows`, verifies its SHA-256 against `deps.lock.json` before unpacking, exits non-zero on mismatch or after three failed attempts 30 s apart.
  4. Create `tools/run_pluginval.py <pluginval-binary> <vst3-bundle>...`: for each bundle runs the command of D-007, captures the log to `pluginval-<name>.log`, and applies the pass rule of D-007; exits non-zero if any bundle fails.
  5. Add jobs `vst3-windows`, `vst3-macos`, `vst3-macos-intel` and extend `vst3-linux`: build all targets; run `muff_vst3_tests`; on Linux, Windows and macOS arm64 run steps 3 and 4 over the seven bundles (Linux under `xvfb-run -a`). Upload the pluginval logs (text) with `if: always()`. No bundle is uploaded.
- Outputs: the files named; `ci.yml`.
- Evidence produced: T-CI-PLUGINVAL; first part of T-901 (plug-in targets exist)
- Done when: on Linux, `cmake --build build/vst3` produces seven directories `build/vst3/vst3/Muff*_artefacts/Release/VST3/*.vst3`, each containing a shared library larger than 1 MB (print sizes); `python3 tools/run_pluginval.py` over the seven exits 0 locally or in CI; CI green in all four `vst3-*` jobs and `aggregate`.
- Checkpoint: `active_step_id: "S-021"`, per-platform build minutes from the job summaries.
- On failure: pluginval reports a failure: fix the shell or editor (the contract is fixed); if the failing check contradicts the frozen contract, stop and write `BLOCKED.md` (R-011). A VST3 job over 45 min: apply D-005's revisit rule (add `actions/cache` for a `ccache`/`sccache` directory keyed on the JUCE commit and runner label) and record it in `DEVIATIONS.md`. pluginval download hash mismatch: stop and write `BLOCKED.md` (do not update the hash).
- Gate: none
- Relevant decisions/claims: D-005, D-007, D-016; C-PLANNER-006, C-PLANNER-007, C-PLANNER-008
- Lessons applied: `research/PRIOR_KNOWLEDGE.md` rows "pluginval exit status is not proof", "headless plug-in validation"
- Exclusive resources: `.github/workflows/ci.yml`

### S-030 Rack SDK acquisition
- Tier: Sonnet
- Profile: software
- Depends on: S-018
- Inputs: D-003, `deps.lock.json`.
- Actions:
  1. Create `tools/fetch_rack_sdk.py <platform> <dest> [--record]`: downloads the archive of D-003 for `lin-x64`, `win-x64`, `mac-x64` or `mac-arm64` (three attempts, 30 s apart); without `--record` it verifies the SHA-256 against `deps.lock.json` and fails on mismatch or missing entry; with `--record` it prints the hash and the byte size; then unpacks to `<dest>/Rack-SDK` and asserts `<dest>/Rack-SDK/plugin.mk` and `<dest>/Rack-SDK/include/rack.hpp` exist.
  2. Add workflow `.github/workflows/record-rack-sdk.yml` (same permissions and pin rules; `on: push`, with the single job guarded by `if: contains(github.event.head_commit.message, '[record-rack-sdk]')`) whose `ubuntu-24.04` job runs the tool with `--record` for the four platforms and prints the four hashes and sizes; push a commit whose message contains `[record-rack-sdk]` and read the job log.
  3. Write the four hashes into `deps.lock.json` under `rack-sdk.sha256`, with a sibling key `"sha256_note": "trust on first use: recorded from vcvrack.com on <UTC date> by run <run URL>"`.
- Outputs: `tools/fetch_rack_sdk.py`, `.github/workflows/record-rack-sdk.yml`, `deps.lock.json`.
- Evidence produced: first part of T-905 (lock file complete)
- Done when: `python3 -m unittest tests.py.test_repo.T905PinsAndWorkflows.test_dependency_lock` run from the repository root with `PYTHONPATH=.` passes, or equivalently `python3 -m unittest discover -s tests/py -k test_dependency_lock` reports `OK`; each recorded archive size is above 5 MB.
- Checkpoint: `active_step_id: "S-030"`, the four hashes.
- On failure: download fails three times in the workflow: wait 30 min, push another marker commit (an empty commit is enough: `git commit --allow-empty`), at most three runs, then `BLOCKED.md` (D-003).
- Gate: none
- Relevant decisions/claims: D-003; C-PLANNER-014
- Lessons applied: L-20261111T151500Z-download-output-committed-unchecked
- Exclusive resources: `deps.lock.json`

### S-031 Rack panels
- Tier: Sonnet
- Profile: software
- Depends on: S-018
- Inputs: D-006 (panel rules and widths), `tests/fixtures/effects.json`.
- Actions: create `tools/gen_panels.py` which, for each effect, writes `rack/res/<Enum>.svg` (width `HP * 5.08` mm, height 128.5 mm, matching `viewBox`, background rectangle, the effect name at the top, a label under every control and port position, all as `<path>` elements from the built-in stroke-letter table) and `rack/src/generated_layout.hpp` (for each effect, arrays of millimetre positions for parameters, inputs, outputs and lights in table order; controls on a grid 15.24 mm wide by 18 mm high starting 22 mm from the top, ports in rows at the bottom starting 98 mm from the top). `python3 tools/gen_panels.py --check` exits 1 if committed files differ from regenerated ones. Run it and commit the outputs.
- Outputs: `tools/gen_panels.py`, `rack/res/*.svg` (7), `rack/src/generated_layout.hpp`.
- Evidence produced: T-903
- Done when: `python3 -m unittest discover -s tests/py -k test_panels` reports `OK`; `python3 tools/gen_panels.py --check` exits 0; each SVG is between 2 kB and 400 kB.
- Checkpoint: `active_step_id: "S-031"`.
- On failure: three attempts, then `BLOCKED.md`.
- Gate: none
- Relevant decisions/claims: D-006; C-LEGAL-014, C-LEGAL-015
- Lessons applied: `research/PRIOR_KNOWLEDGE.md` row "Rack panel SVG limits"
- Exclusive resources: none

### S-032 Rack module shells and manifest
- Tier: Opus
- Profile: software
- Depends on: S-030, S-031
- Inputs: D-006, D-012, D-013, `rack/src/generated_layout.hpp`, the Rack SDK headers (unpacked by `tools/fetch_rack_sdk.py lin-x64 .`).
- Actions:
  1. Create `rack/plugin.json` with the fields T-902 checks: slug, name and brand `Mufferfluffer`; version `2.0.0`; license `AGPL-3.0-only`; author `qualitycoding`; sourceUrl `https://github.com/qualitycoding/mufferfluffer`; seven modules with `slug` = the fixture's `enum`, `name` = the fixture's `name`, a description of at most 120 characters, and tags: Lowpass `["Filter", "Effect", "Hardware clone"]`; RingMod `["Ring modulator", "Effect", "Hardware clone"]`; Phaser `["Phaser", "Effect", "Hardware clone"]`; Delay `["Delay", "Effect", "Hardware clone"]`; FilterBank `["Filter", "Sequencer", "Effect", "Hardware clone"]`; OscBox `["Oscillator", "Effect", "Hardware clone"]`; Chorus `["Chorus", "Flanger", "Effect", "Hardware clone"]`.
  2. Create `rack/Makefile` (D-006), `rack/src/plugin.hpp`, `rack/src/plugin.cpp` (registers seven models) and `rack/src/module_shell.hpp` (one class template or one generic `Module`/`ModuleWidget` pair parameterised by `muff::EffectId`, built from `muff::effectInfo` and the generated layout, following D-006), plus `rack/src/<Enum>.cpp` for each effect creating its `Model`.
  3. Build on Linux: `python3 tools/fetch_rack_sdk.py lin-x64 .` then `make -C rack RACK_DIR="$PWD/Rack-SDK" dist`.
- Outputs: `rack/plugin.json`, `rack/Makefile`, `rack/src/*`.
- Evidence produced: T-902; T-906 precondition (`rack/src` exists)
- Done when: `python3 -m unittest discover -s tests/py -k test_manifest` reports `OK`; `make -C rack ... dist` exits 0 and `rack/dist/Mufferfluffer-2.0.0-lin-x64.vcvplugin` exists and is larger than 100 kB; `nm -D --defined-only rack/plugin.so | grep -c ' init$'` prints 1.
- Checkpoint: `active_step_id: "S-032"`.
- On failure: a Rack API named in D-006 does not exist in SDK 2.6.6 (`configSwitch`, `configBypass`, `getVoltageSum`, `ParamQuantity`): read `Rack-SDK/include/engine/Module.hpp` and `Port.hpp`, use the present equivalent, and record it in `DEVIATIONS.md`; anything that changes the user-visible behaviour of D-006: `BLOCKED.md`.
- Gate: none
- Relevant decisions/claims: D-006, D-004; C-PLANNER-004, C-PLANNER-015, C-LEGAL-011
- Lessons applied: `research/PRIOR_KNOWLEDGE.md` rows "Rack module lifecycle", "Rack voltage standards"
- Exclusive resources: none

### S-033 Rack builds on four platforms
- Tier: Sonnet
- Profile: software
- Depends on: S-032
- Inputs: D-009, `plan/ENVIRONMENT.md` (packages), `tools/fetch_rack_sdk.py`.
- Actions: add jobs `rack-linux`, `rack-windows` (steps under `msys2/setup-msys2` with `msystem: MINGW64` and `shell: msys2 {0}`), `rack-macos`, `rack-macos-intel`: fetch and verify the SDK for the job's platform, `make -C rack RACK_DIR=... dist`, then `python3 tools/check_rack_package.py rack/dist` (new tool: asserts exactly one `*.vcvplugin` file larger than 100 kB whose name contains the platform token; unpacks it (it is a zstd-compressed tar: `zstd -d` then `tar -t`) and asserts it contains `Mufferfluffer/plugin.json`, the platform's plug-in library (`plugin.so`, `plugin.dll` or `plugin.dylib`), `Mufferfluffer/LICENSE*` and the seven `res/*.svg`). Add the four jobs to `aggregate`. No package is uploaded.
- Outputs: `tools/check_rack_package.py`; `ci.yml`.
- Evidence produced: T-CI-RACKPKG
- Done when: CI green in the four `rack-*` jobs and `aggregate`; each job's log shows the package name and size.
- Checkpoint: `active_step_id: "S-033"`.
- On failure: a platform fails to build: fix forward (three attempts per platform), then `BLOCKED.md`. `make dist` does not include `LICENSE`: add `DISTRIBUTABLES += $(wildcard ../LICENSE)` to `rack/Makefile`.
- Gate: none
- Relevant decisions/claims: D-003, D-006, D-009
- Lessons applied: `research/PRIOR_KNOWLEDGE.md` rows "Windows CI", "artefact post-conditions"
- Exclusive resources: `.github/workflows/ci.yml`

### S-034 Dependency vulnerability scan
- Tier: Sonnet
- Profile: software
- Depends on: S-030
- Inputs: D-061, `deps.lock.json`.
- Actions: create `tools/check_advisories.py <lockfile>` per D-061 (standard library `urllib`; prints one line per dependency: name, commit, number of advisories; for entries without a `commit` key (Rack SDK, pluginval archives) prints `not queried: no commit pin`); add job `deps-scan` and add it to `aggregate`.
- Outputs: `tools/check_advisories.py`; `ci.yml`.
- Evidence produced: T-CI-ADVISORIES
- Done when: CI green in `deps-scan`; its log shows two queried commits (JUCE, Catch2) each with a count; running the tool locally with a lock file whose JUCE commit is replaced by `0000000000000000000000000000000000000000` still exits 0 with count 0 (documents the "none known" limit), and running it with `MUFF_OSV_URL=https://api.osv.dev/v1/nonexistent` exits non-zero (shows the gate can fail).
- Checkpoint: `active_step_id: "S-034"`.
- On failure: the API reports an advisory for a pinned commit: read it; if it affects code this project uses, stop and write `BLOCKED.md` (a pin change re-opens `plan/ENVIRONMENT.md`); if it does not, record the advisory ID and the reason in `plan/RISKS.md` and add the ID to an `allow` list in `deps.lock.json` that the tool honours and prints.
- Gate: none
- Relevant decisions/claims: D-061; C-PLANNER-019
- Lessons applied: `research/PRIOR_KNOWLEDGE.md` row "a check that cannot fail"
- Exclusive resources: `.github/workflows/ci.yml`

### S-035 Documentation
- Tier: Sonnet
- Profile: software
- Depends on: S-021, S-033
- Inputs: T-907's requirements; D-004; the fixture.
- Actions: rewrite `README.md` with sections `## Build`, `## Install` (where a VST3 bundle and a Rack package go on each operating system), `## Tests`, `## Licence`, keeping the sentence that the project is not affiliated with any manufacturer, adding the Steinberg sentence of D-004, and claiming nothing about likeness to any hardware beyond "modelled on the documented behaviour of" (no claim of sounding identical: no test or comparison supports one); create `docs/USER_GUIDE.md` with one `## <effect name>` section per effect that names every parameter, input and output by its table name and describes it in one or two sentences from D-020 to D-026, including the D-027 design choices that a user would notice (own pattern set; no MIDI); create `CHANGELOG.md` with a `0.1.0` entry. British English.
- Outputs: `README.md`, `docs/USER_GUIDE.md`, `CHANGELOG.md`.
- Evidence produced: T-907; T-901 (naming), T-906 (licence headers) become checkable
- Done when: `python3 -m unittest discover -s tests/py -k "T907 or T901 or T906 or T902 or T903 or T904"` reports `OK` (T-905's workflow test stays red until S-MUTATE adds `mutation.yml`); in `ci.yml`, `repo-checks` now also runs `python3 -m unittest discover -s tests/py -v -k "not test_workflows"`; CI green.
- Checkpoint: `active_step_id: "S-035"`.
- On failure: T-901 reports a forbidden mark in a product file: remove it. Three attempts, then `BLOCKED.md`.
- Gate: none
- Relevant decisions/claims: D-004, D-027
- Lessons applied: user preference for British English (intake); `research/PRIOR_KNOWLEDGE.md` row "user-visible names in one place"
- Exclusive resources: `.github/workflows/ci.yml`

### S-REVIEW Code review
- Tier: Opus
- Profile: software
- Depends on: S-018, S-021, S-033, S-034, S-035
- Inputs: `git diff origin/<generation branch>...HEAD`; the frozen tests; `plan/DECISIONS.md`; D-062.
- Actions: three fresh-context reviewers that wrote none of the code, each given the diff and one brief: (1) correctness against D-011 to D-037 and anti-patterns (dead code, duplication, hidden mutation, naming, needless complexity); (2) security and real-time safety (memory handling in delay lines and state parsing, allocation or locking on the audio thread, denormals, integer overflow in sample counters, division by zero at parameter extremes); (3) untested code paths (every new function mapped to a T- id or named as a gap) and dependency currency (each pin compared with the newest tag by `git ls-remote --tags`, differences listed). Merge into `reviews/CODE_REVIEW.md` with one row per finding: id, severity (protocol Rule 4), file and line, description, resolution (fix commit or R- entry). Fix every Critical and High finding; record each fix of an implementer mistake as a lesson (Rule 11).
- Outputs: `reviews/CODE_REVIEW.md`; fix commits; lessons.
- Evidence produced: none
- Done when: `reviews/CODE_REVIEW.md` has no Critical or High row without a fix commit; the suite status is unchanged or better (`ctest` 77 of 77; `muff_vst3_tests` 6 of 6); CI green.
- Checkpoint: `active_step_id: "S-REVIEW"`, counts by severity.
- On failure: a finding needs a new test: `HANDOFF.md` section 6 (Phase 2 re-opening); a finding disputes a decision: record it as an R- entry at its severity and continue unless Critical or High, in which case `BLOCKED.md`.
- Gate: none
- Relevant decisions/claims: D-062
- Lessons applied: none specific
- Exclusive resources: none

### S-ADVERSARIAL Adversarial review
- Tier: Opus
- Profile: software
- Depends on: S-REVIEW
- Inputs: the built core library, test binaries and plug-ins; `plan/ASSUMPTIONS.md`; D-012, D-016.
- Actions: one fresh-context agent tries to break the product, writing throw-away programs against the public headers in `build/adversarial/` (not committed): parameter changes on every sample; sample rates at both limits and prepare/reset storms; block sizes 1 and 8192 in the VST3 shell; state blobs of 0 bytes, 64 MB of zeros, deeply nested XML, XML with 100 000 `param` children, values `nan`, `inf`, `1e400`; all inputs at +/-1e30 and alternating NaN; tap triggers every sample; 24 hours of simulated audio at 8 kHz for one effect chosen at random (counter overflow); `RackIO` with every connection flag toggled each sample; denormal inputs (1e-38) with timing compared against normal inputs (slow-down above 3x is a finding); a host-style automation stress on the VST3 shell: one thread calls `setValueNotifyingHost` on every parameter in a tight loop and calls `getStateInformation`/`setStateInformation` while another runs `processBlock` for 60 s, built with `-fsanitize=thread` (any reported race in project code is a finding). Each confirmed break becomes a new frozen test through `HANDOFF.md` section 6 or an R- entry with severity. Output `reviews/ADVERSARIAL_REVIEW.md` (what was tried, what broke, resolution).
- Outputs: `reviews/ADVERSARIAL_REVIEW.md`; fix commits.
- Evidence produced: none
- Done when: no Critical or High break is unresolved; the suite is green as in S-REVIEW; CI green.
- Checkpoint: `active_step_id: "S-ADVERSARIAL"`.
- On failure: as S-REVIEW.
- Gate: none
- Relevant decisions/claims: D-012, D-016; A-007, A-011
- Lessons applied: none specific
- Exclusive resources: none

### S-MUTATE Mutation testing of the DSP core
- Tier: Opus (design and survivor work), Sonnet (workflow)
- Profile: software
- Depends on: S-ADVERSARIAL
- Inputs: skills `mutation-testing-in-ci` and `mutation-sharding` of `qualitycoding/skills` (read both `SKILL.md` files and copy their scripts `mull_score.py`, `mull_include_paths.py` and the example workflow as the starting point; record the skills-repository commit used); D-060.
- Pre-decided record (protocol 3.8):
  - Tool and pin: Mull, package `mull-19` from `https://dl.cloudsmith.io/public/mull-project/mull-stable/` with `clang-19` and `libclang-rt-19-dev` on `ubuntu-24.04`; the installed package version is printed by the job and written to `deps.lock.json` under `mull` by sub-step 1; the configuration file `mull.yml` (mutators: the tool's default set; `includePaths: ["core/src/.*", "adapters/src/.*"]`; `excludePaths: [".*generated_tables.*", ".*tests/.*", ".*_deps/.*"]`) is part of the pin.
  - Scope: `core/src/*.cpp`, `adapters/src/*.cpp`. Out of scope: generated tables, `vst3/`, `rack/` (shells; covered by T-851 to T-856, pluginval and package checks).
  - Floor: project score 60 %, chosen before the first run; after the first real full run it is raised to five points below the achieved score, never lowered without a D- entry.
  - Test list: the whole of `muff_core_tests` is too slow per mutant, so each shard runs only the tests that exercise its file: `lowpass.cpp` -> `"[lowpass]"`; `ringmod.cpp` -> `"[ringmod]"`; `phaser.cpp` -> `"[phaser]"`; `delay.cpp` -> `"[delay]"`; `chorus.cpp` -> `"[chorus]"`; `filterbank.cpp` -> `"[filterbank]"`; `oscbox.cpp` -> `"[oscbox]"`; `tables.cpp`, `blocks.cpp`, `effect_base.cpp`, `create.cpp` -> `"[tables],[params],[blocks],[lifecycle],[cv],[bypass],[smoothing],[meters]"`; `adapters.cpp` -> `"[adapters]"`. Omitted for every shard: T-007 (robustness sweep), T-011 (allocation hook), T-017, T-018 and, for the effect shards, the common tests; mutants only those tests would kill count as survivors, so the score is conservative (this omission is decision D-060 and is restated in `reviews/MUTATION.md`).
  - Estimates (not measured: Mull is not installable in the planning sandbox, so these are estimates to be replaced by sub-step 1): mutant count 600 to 1200 over 12 files; per-shard test time 1 to 4 s in a Release build, assumed 5 to 20 s instrumented and unoptimised; per-shard job time `mutants x time / 4 workers` = about 10 to 60 min; instrumented build about 5 min per shard. Runtime budget: 90 min per shard job, 6 h for the full run across shards.
  - Shards: a generated matrix, one job per in-scope file (12), `fail-fast: false`, `ubuntu-24.04` (4 vCPU, 16 GB), `--workers 4`, `timeout-minutes: 120`, per-mutant timeout 10x the shard's measured baseline (minimum 30 s), warm-up timeout 120 s.
  - Triggers (three tiers, one required check), all in `.github/workflows/mutation.yml`, never path-filtered: (a) `pull_request` and `push`: job `scope` computes `incremental` (only changed in-scope files), `full` (when anything under `tests/`, `mull.yml`, `tools/mull_*`, or `mutation.yml` itself changed) or `not-applicable`; (b) `schedule` weekly (Monday 03:17 UTC) full run on the default branch (live after the merge of S-FINAL's pull request), whose failure opens an issue titled `Scheduled mutation run failed` assigned to the repository owner (this job alone has `permissions: issues: write`); (c) on demand: `workflow_dispatch` with a `mode` input (live after that merge) and, on any branch, a push whose head commit message contains `[mutation full]`; (d) job `mutation-gate` with `if: always()`: the only status meant to be required; it fails unless every expected shard report is present and the score computed **from the SQLite reports** by `tools/mull_score.py` meets the project floor (full) or the per-file baseline `mutation-baseline.json` with tolerance 2 points (incremental), with `--min-mutants 5` per shard; `not-applicable` is a success that prints `mode: not-applicable`.
- Actions:
  1. Sub-step 1 (measure): add the workflow; the `scope` job selects `full` when the head commit's message contains `[mutation full]`; push such a commit and let the full mode run once; record in `reviews/MUTATION.md` the installed Mull version, per-shard mutant counts, baseline test times, job times and scores. If any shard exceeds 90 min, split that file's test list further or apply the `mutation-sharding` skill's per-function split; do not widen the job.
  2. Sub-step 2 (show it can fail): run `tools/mull_score.py` on one real report with `--floor 99` and paste the non-zero exit and message into `reviews/MUTATION.md`; verify that the script's surviving count for one shard equals Mull's own "Surviving mutants" line.
  3. Sub-step 3 (survivors): worst file first, each surviving mutant becomes a new test through `HANDOFF.md` section 6 or a written justification grouped by mutant class in `reviews/MUTATION.md`.
  4. Sub-step 4 (wire): commit `mutation.yml`, `mull.yml`, `tools/mull_score.py`, `tools/mull_include_paths.py`, `tools/mull_shards.py` (generates the matrix), `mutation-baseline.json` (from the full run); in `ci.yml` change `repo-checks` to run the whole of `tests/py` (remove the `-k "not test_workflows"` of S-035). Ask nothing of the owner: enforcing a required check is a repository setting; write in `reviews/MUTATION.md` that `mutation-gate` and `aggregate` should be marked required by the owner, as a recommendation.
- Outputs: `.github/workflows/mutation.yml`, `mull.yml`, `tools/mull_*.py`, `mutation-baseline.json`, `reviews/MUTATION.md`, `deps.lock.json` (Mull version).
- Evidence produced: T-905 (workflow test) turns green
- Done when: a full run on the pushed commit is green in `mutation-gate` with the project score printed and at or above the floor; an incremental run triggered by a one-line change to `core/src/phaser.cpp` mutates only that file (log shows one shard) and a documentation-only push reports `mode: not-applicable`; `python3 -m unittest discover -s tests/py -v` reports `OK` (9 tests); `reviews/MUTATION.md` holds the measured numbers, the failing-gate demonstration and the survivor table.
- Checkpoint: after each sub-step: `active_step_id: "S-MUTATE.<n>"`, run URLs, scores. Resume: re-dispatch the failed shard only (shard reports are separate artefacts).
- On failure: `mull-19` does not install: R-010's response. First real score below 60 %: do not lower the floor; work survivors (sub-step 3) until the floor is met, at most three full runs, then `BLOCKED.md` with the per-file table. A shard times out at 120 min: sharding skill, then `BLOCKED.md` if still over.
- Gate: none
- Relevant decisions/claims: D-060; C-PLANNER-012
- Lessons applied: every lesson tagged `tech:mull` in `research/PRIOR_KNOWLEDGE.md` (score from the report; gate shown to fail; no path-filtered required workflow; survivors by file)
- Exclusive resources: `.github/workflows/mutation.yml`

### S-040 Binary packages for hands-on trial (publication gate)
- Tier: Sonnet
- Profile: software
- Depends on: S-MUTATE
- Inputs: D-041, `plan/GATES.md` G-002.
- Actions:
  1. Create `.github/workflows/packages.yml`: `on: push`, every job guarded by `if: contains(github.event.head_commit.message, '[build-packages G-002-approved]')`; jobs for the four VST3 platforms and four Rack platforms that build and upload `mufferfluffer-vst3-<platform>.zip` and the `.vcvplugin` files as artefacts with `retention-days: 14`, plus a `SHA256SUMS` file; same pin and permission rules as `ci.yml`.
  2. Halt at G-002: write `GATE-G-002.md` (evidence and question in `plan/GATES.md`) and wait.
  3. On `proceed`: push a commit whose message contains `[build-packages G-002-approved]` (the commit that adds `GATE-G-002.RESPONSE.md`); record the run URL and the checksums in `GATE-G-002.RESPONSE.md`. On `decline`: push no such commit; record that hands-on trial needs a local build per `README.md`.
- Outputs: `.github/workflows/packages.yml`, `GATE-G-002.md`, `GATE-G-002.RESPONSE.md`.
- Evidence produced: none
- Done when: the response is recorded; on `proceed`, the run is green and its artefact list shows eight archives plus `SHA256SUMS`, each above 100 kB.
- Checkpoint: `active_step_id: "S-040"`, gate state.
- On failure: no response: S-LISTEN may proceed (its evidence bundle is audio made by this project and needs no approval); the packages stay unbuilt.
- Gate: G-002
- Relevant decisions/claims: D-041; C-LEGAL-036
- Lessons applied: `research/PRIOR_KNOWLEDGE.md` row "public repository artefacts are publication"
- Exclusive resources: none

### S-LISTEN Listening validation
- Tier: Opus
- Profile: software (audio)
- Depends on: S-ADVERSARIAL (it may run while S-MUTATE and S-040 are in progress, but its fixes re-trigger the incremental mutation run)
- Inputs: skill `audio-blind-listening-validation` (read `SKILL.md`); D-040; `plan/GATES.md` G-LISTEN.
- Fixed content:
  - Reference set: **none** (D-040: no redistributable recording and no dry/wet pair exists; search log in `research/SOURCES.md`, strand `references`). The blind A/B test is replaced by demos only. No reference-derived audio exists, so the evidence branch holds only this project's own audio.
  - Frozen metrics that stand under the listening gate: all behavioural tests T-101 to T-707 (ranges, frequencies, times). The gate never relaxes them.
  - Pass condition: the owner's response is `proceed` or `proceed-with-rescope`.
  - Round limit: 3 `iterate` rounds, then escalate with the residual remarks; the owner chooses `proceed-with-rescope` or `stop`.
  - Demo list (22 files, 48 kHz, 24-bit WAV, each 12 to 25 s, peak-normalised to -3 dBFS; every demo starts with 3 s of the dry input, then the processed signal):
    - `demo-01-lowpass-sweep`: saw bass line; cutoff swept 60 Hz to 8 kHz and back by automation, resonance 5.
    - `demo-02-lowpass-envelope`: plucked chords; cutoff 150 Hz, Amount 4, Fast then Smooth (switch at half time), resonance 6.
    - `demo-03-lowpass-selfosc`: silence in; resonance 10, cutoff stepped through 110, 220, 440, 880 Hz; then drum pattern in with resonance 9, 2-pole then 4-pole.
    - `demo-04-ring-tremolo`: held chord; range Lo, carrier 4 Hz to 30 Hz sweep.
    - `demo-05-ring-bells`: plucked tones; range Hi, carrier 350 Hz, mix 0.5 then 1.0.
    - `demo-06-ring-lfo`: plucked tones; carrier 500 Hz, LFO sine 0.5 Hz Amount 1, then square 6 Hz Amount 3.
    - `demo-07-phase-slow`: held chord; 6-stage, LFO Lo 0.2 Hz, Amount 0.6, resonance 3; second half 12-stage.
    - `demo-08-phase-resonant`: saw bass; 12-stage, resonance 9, LFO 0.6 Hz, Amount 0.8.
    - `demo-09-phase-fast`: plucked tones; LFO Hi 8 Hz then 120 Hz, Amount 0.4; stereo file with Aux Out on the right.
    - `demo-10-delay-short`: drum pattern; Short, 120 ms, feedback 4, mix 0.4.
    - `demo-11-delay-long-dark`: plucked tones; Long, 600 ms, feedback 6.5, mix 0.5.
    - `demo-12-delay-runaway`: one plucked note; Short 300 ms, feedback raised 5 to 10 over 10 s then back to 3; Time moved 300 to 150 ms during the tail (pitch glide).
    - `demo-13-delay-lfo`: held chord; 250 ms, LFO sine 0.8 Hz Amount 3, then triangle 5 Hz Amount 2, then random step 4 Hz Amount 4.
    - `demo-14-bank-static`: drum pattern; Mids, pattern 1, sliders shaped as a smile curve, then Bass voicing.
    - `demo-15-bank-patterns`: held chord; Mids, patterns 2, 4, 9, 11 for 5 s each, Rate 6, Envelope 2.
    - `demo-16-bank-envelopes`: held chord; pattern 12, Rate 3, Envelope 0, 2, 5, 6, 8 for 4 s each; stereo.
    - `demo-17-bank-lfo`: saw bass; Bass voicing, pattern 2, LFO on, Sweep 0.3.
    - `demo-18-osc-follow`: plucked tones; frequency 220 Hz, Wave 0.33, mix 0.6, Env Amount 0.
    - `demo-19-osc-sync-sweep`: saw bass; Sync on, frequency swept 100 Hz to 1.6 kHz, Wave 0.66, mix 1.
    - `demo-20-osc-fm-env`: drum pattern; FM Amount 0.7, Env Amount 3, frequency 80 Hz, Wave 0.
    - `demo-21-swarm-chorus`: plucked chords; Chorus range, Time 0.4, LFO 0.6 Hz Amount 0.3, feedback 0.2, mix 0.5; stereo.
    - `demo-22-swarm-flange`: drum pattern then held chord; Flange range, LFO 0.15 Hz Amount 0.9, feedback 0.7 then -0.7, mix 0.5; stereo.
- Actions:
  1. Create `tools/render/muff_render.cpp` (built as target `muff_render` in the top-level `CMakeLists.txt` when `-DMUFF_BUILD_TOOLS=ON`; uses `BlockRunner` at 48 kHz; usage `muff_render <slug> <in.f32> <out.f32> <events.txt>`; raw little-endian float32 mono in, stereo interleaved out; the event file has one event per line: `<seconds> <param id> <value>`, sorted by time, applied at the first sample at or after that time), `tools/make_demo_inputs.py` (writes the four dry inputs of D-040 as float32 files; deterministic, standard library only: additive plucked tones with exponential decay, a 55 Hz sawtooth bass line of eighth notes, a noise-burst drum pattern from a seeded generator, a held three-note chord of detuned sawtooths), `tools/render_demos.py` (renders the 22 demos from `demos/demo-NN-<name>.txt` event files, prepends the dry section, normalises, writes 24-bit WAV with the `wave` module, writes `SHA256SUMS` and `INDEX.md` with one line per demo naming effect and settings, the peak level of the processed section before normalisation in dBFS, and the gain applied, so that level problems are visible and not hidden by normalisation; the script fails if any processed section peaks above +9 dBFS or below -40 dBFS before normalisation), and the 22 event files under `demos/`.
  2. Add workflow `.github/workflows/evidence.yml`: runs on `push` only when the head commit message contains `[gate-evidence G-LISTEN]`; its own concurrency group `evidence-G-LISTEN` (no cancellation); `permissions: contents: write` for this workflow only; builds `muff_render`, renders the bundle, asserts 22 WAV files each between 0.5 MB and 20 MB and a non-silent RMS (above -50 dBFS), and force-pushes them with `SHA256SUMS`, `INDEX.md` and a `README.md` stating that every file is this project's own work under the repository licence to the orphan branch `evidence/G-LISTEN`.
  3. Push a marker commit `S-LISTEN round <n> [gate-evidence G-LISTEN]`; verify the branch updated and one raw URL downloads with the listed hash.
  4. Halt at G-LISTEN: write `GATE-G-LISTEN.md` with the zip URL `https://github.com/qualitycoding/mufferfluffer/archive/refs/heads/evidence/G-LISTEN.zip`, per-file raw URLs, the behavioural test summary, and the questions of `plan/GATES.md`.
  5. On `iterate: <notes>`: follow the skill's section 7: use the owner's words to choose the analysis, make one targeted change per reported difference with a general mechanism, re-run the suite, record the round in `reviews/LISTENING.md` (remarks verbatim, measured difference, change, commit), push a new marker commit, ask the same questions. If the remark matches a pre-decided remedy (R-006 aliasing, R-007 oscillator), apply that remedy first.
- Outputs: the tools, event files and workflow named; branch `evidence/G-LISTEN`; `GATE-G-LISTEN.md`, `GATE-G-LISTEN.RESPONSE.md`; `reviews/LISTENING.md`.
- Evidence produced: SC-10
- Done when: `GATE-G-LISTEN.RESPONSE.md` records `proceed` or `proceed-with-rescope: <text>`; the suite is green on the commit the last bundle was built from; `reviews/LISTENING.md` has one section per round.
- Checkpoint: `active_step_id: "S-LISTEN"`, round number, marker commit, bundle hash.
- On failure: an `iterate` change breaks a frozen test: revert it; if the owner's request cannot be met without changing a frozen test, record that in the next gate text and offer `proceed-with-rescope` or a Phase 2 re-opening (`HANDOFF.md` section 6). `stop`: write `BLOCKED.md` with the owner's text and end the run at S-RETRO. No response: continue independent steps (S-MUTATE, S-040), then wait.
- Gate: G-LISTEN
- Relevant decisions/claims: D-040, D-041; C-REF-028 to C-REF-035; R-003, R-004, R-005, R-006, R-012
- Lessons applied: `research/PRIOR_KNOWLEDGE.md` rows "listening gate", "evidence branch"
- Exclusive resources: branch `evidence/G-LISTEN`

### S-FINAL Final verification
- Tier: Sonnet
- Profile: software
- Depends on: S-MUTATE, S-040, S-LISTEN
- Inputs: the head of `impl-<run_id>`.
- Actions: open a pull request from `impl-<run_id>` to `main` titled `mufferfluffer 0.1.0: seven effects, VST3 and Rack` and do not merge it (merging is the owner's decision; say so in `COMPLETION.md` together with the fact that the scheduled and manually dispatched workflows become live only after the merge). On the head commit run `python3 tools/check_frozen.py`; the core, VST3 and repository test commands of `plan/ENVIRONMENT.md`; read the CI run of that commit (`ci.yml` and `mutation.yml`); fill `plan/TRACEABILITY.md`'s "Result" column for every success criterion with the test ids and the run URL; write `COMPLETION.md` listing each SC-1 to SC-10 as met or not met with evidence, every `DEVIATIONS.md` row, every open R- entry, and what was not verified (R-012 unless the owner reported a hands-on trial).
- Outputs: `COMPLETION.md`; `plan/TRACEABILITY.md` (Result column only).
- Evidence produced: SC-1 to SC-9 (summary)
- Done when: `ctest` 77 of 77; `muff_vst3_tests` `All tests passed` (6); `python3 -m unittest discover -s tests/py` `OK` (9); `aggregate` and `mutation-gate` green on the head commit; `COMPLETION.md` exists with ten SC rows.
- Checkpoint: `active_step_id: "S-FINAL"`.
- On failure: any test red: return to the owning step.
- Gate: none
- Relevant decisions/claims: all SC of `plan/ASSUMPTIONS.md`
- Lessons applied: none specific
- Exclusive resources: none

### S-RETRO Retrospective
- Tier: Opus (fresh context); Haiku collects the digest
- Profile: software
- Depends on: S-FINAL
- Inputs: `EXECUTION_LOG.md`, commits, `DEVIATIONS.md`, `BLOCKED.md` (if any), `TEST_CHALLENGE.md` (if any), `GATE-*.md`, CI history, `research/PRIOR_KNOWLEDGE.md`, `lessons/`, `knowledge/`, `plan/SOURCE_REGISTER.md`.
- Actions: protocol 3.7.1 as written: answer its questions 1, 1a, 2, 3, 4 and 5 (for 1a, diff the workflows and `deps.lock.json` of the implementation branch against `plan/ENVIRONMENT.md`); write `RETROSPECTIVE.md` with every finding linked to an `L-` or `K-` file; add the missing lessons, knowledge items and source-register rows. Re-run nothing; change no deliverable.
- Outputs: `RETROSPECTIVE.md`; new files in `lessons/`, `knowledge/`; `plan/SOURCE_REGISTER.md`.
- Evidence produced: none
- Done when: every finding links to an existing `L-` or `K-` file; each new file has the front-matter keys of the existing ones in its directory (`id`, `title`, `status`, `severity` or `confidence`, `tags`, `recorded_at`).
- Checkpoint: `active_step_id: "S-RETRO"`.
- On failure: none (a one-pass review); missing inputs are themselves findings.
- Gate: none
- Relevant decisions/claims: A-012
- Lessons applied: all (this step audits their application)
- Exclusive resources: none

### S-KNOW Lessons and knowledge push
- Tier: Haiku
- Profile: software
- Depends on: S-RETRO
- Inputs: `lessons/`, `knowledge/`, `plan/SOURCE_REGISTER.md`, the stores of A-012.
- Actions: protocol 3.7.2 as written: halt at G-003 with `GATE-G-003.md` (contents in `plan/GATES.md`); on sign-off copy `lessons/` into the lesson store and `knowledge/` (including the source register as a knowledge item) into the knowledge store in each store's existing layout, regenerate each store's indexes with the store's own index script, push; on a rejected push pull, regenerate and retry up to three times per store; on failure or no response write `git bundle` files `lessons-<run_id>.bundle` and `knowledge-<run_id>.bundle` and report their paths.
- Outputs: `GATE-G-003.md`, `GATE-G-003.RESPONSE.md`; store commits or bundles.
- Evidence produced: none
- Done when: both pushes succeeded and each store's index lists every pushed id; or the response was `do-not-push`; or the bundles were written and reported.
- Checkpoint: `active_step_id: "S-KNOW"`, store commit hashes.
- On failure: as protocol 3.7.2 item 3.
- Gate: G-003
- Relevant decisions/claims: A-012
- Lessons applied: none specific
- Exclusive resources: the two store repositories
