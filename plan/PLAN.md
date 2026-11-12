# Execution plan

Read `HANDOFF.md` first. Step template: section 3.1 of `plan/protocol/planning-protocol.md` (the copy of the protocol on this branch; "protocol" and "Rule <n>" below always mean that file). All paths are relative to the repository root. "The suite" means `muff_core_tests` (80 cases), `muff_vst3_tests` (6 cases) and `tests/py` (9 cases). These counts (and 79 for the sanitizer build) are the counts at the freeze; if tests are added through `HANDOFF.md` section 9.4, every count in this file is read as the frozen count plus the tests added, as tallied in `DEVIATIONS.md`. Commands are for Linux unless a step says otherwise; exact build commands are in `plan/ENVIRONMENT.md`. A filter such as `"[lowpass]"` is passed to the test binary: `build/core/tests/muff_core_tests "[lowpass]"`.

Rules that apply to every step (not repeated below):
- Never edit a file listed in `tests/FROZEN_MANIFEST.sha256`; `python3 tools/check_frozen.py` must exit 0 before every commit.
- A step's work ends in one pushed commit, the **step commit**. Its "Done when" commands and CI results are judged on the step commit. Then append the step's line (naming the step commit) to `EXECUTION_LOG.md`, update `.checkpoints/state.json`, and push both as a separate **log commit** whose subject starts `log:`; nothing is judged on a log commit and its CI run need not be awaited. After every push check that `git ls-remote origin refs/heads/impl-20261111T151412Z-mufferfluffer-vst-vcv` equals `git rev-parse HEAD` (Rule 6).
- A step is done only when its "Done when" commands gave the stated result on the step commit and every file it lists under Outputs exists and is non-empty (Rule 14). "CI green in <jobs>" means those jobs concluded `success` in the `ci.yml` run of the step commit; the `aggregate` job is expected to be red from S-001 until S-018 (the stubs and the temporary filter) and is judged only where a step names it.
- "Locally or in CI": a Done-when command marked so may be met by a CI job log when the host it needs cannot be reached from your machine (S-000 action 6 records which hosts those are).
- Unanticipated situation: the default rule in `HANDOFF.md` section 6. Conventions that every step relies on (sub-agents and tiers, gate answers and waiting, adding a test, records at the repository root, licence headers, commit-message markers): `HANDOFF.md` section 9.
- Tiers name the least capable model allowed; a more capable one may always be used.
- Workflows are triggered by `push` (optionally selected by a marker at the very start of the head commit's subject line, tested with `startsWith(github.event.head_commit.message, '<marker>')`; `HANDOFF.md` section 9.6), because `workflow_dispatch` and `schedule` only work for workflow files that exist on the default branch (`main`), and the implementation branch is not the default branch. Where a step adds `workflow_dispatch` or `schedule` as well, those triggers become live only after the owner merges the pull request of S-FINAL.
- In workflow files, quote every `${{ ... }}` expression that appears inside a YAML flow sequence or mapping, and give shell steps `shell: bash` with `set -euo pipefail` as their first line (the two container jobs of D-009 use `shell: sh` with `set -eu` and no pipes instead: `alpine:3.22` has no bash).
- A gate's `GATE-<id>.md` is committed and pushed when the step halts. `GATE-<id>.RESPONSE.md` (the owner's answer verbatim under the heading `## Answer`) is committed in the same commit as the first change that applies the answer; facts that only exist afterwards (run URLs, checksums) are appended under a second heading `## Record` in a later commit (`HANDOFF.md` section 9.2).

Step order: S-000, S-001, S-010 to S-018, S-020, S-021, S-030 to S-035, S-REVIEW, S-ADVERSARIAL, S-MUTATE, S-040, S-LISTEN, S-FINAL, S-RETRO, S-KNOW.
Independent groups that may run concurrently once their dependencies pass: {S-011, S-012, S-013, S-014, S-016, S-017} (each owns one source file; S-015 follows S-014 because it uses the delay line S-014 writes); {S-020} and {S-030, S-031} after S-018. Running them one after another in numeric order is always correct.

---

### S-000 Environment and access verification
- Tier: Haiku
- Profile: software
- Depends on: none
- Inputs: the generation branch named in `HANDOFF.md`; `plan/ENVIRONMENT.md`; `tests/FROZEN_MANIFEST.sha256`.
- Actions:
  1. Start in a clone of `https://github.com/qualitycoding/mufferfluffer` whose remote `origin` is that URL and whose credential can push branches and workflow files (if you have no clone: `git clone https://github.com/qualitycoding/mufferfluffer && cd mufferfluffer`; the credential is whatever your environment provides: this plan holds none). Then `git fetch origin && git checkout -b impl-20261111T151412Z-mufferfluffer-vst-vcv origin/gen-20261111T151412Z-mufferfluffer-vst-vcv`. The implementation branch is written `impl-<run_id>` elsewhere in this file; it always means `impl-20261111T151412Z-mufferfluffer-vst-vcv`. (The field `run_id` inside `.checkpoints/state.json` holds the generation branch name with its `gen-` prefix; leave that field as it is.)
  2. Install the Linux packages of `plan/ENVIRONMENT.md` (JUCE list). Record `cmake --version`, `g++ --version`, `python3 --version`, `nproc`.
  3. `python3 tools/check_frozen.py` (expect exit 0 and the line `frozen files intact: <n> files`).
  4. Red baseline: run the first three "core + adaptor tests" commands of `plan/ENVIRONMENT.md` (configure, build, `ctest`); expect `ctest` to end with `0% tests passed, 80 tests failed out of 80`. Do not run `muff_bench` here (against the stubs it exits with status 3 by design). Run `python3 -m unittest discover -s tests/py`; expect `FAILED (failures=9)`.
  5. Blocking access checks (any failure: `BLOCKED.md`):
     - `git push -u origin impl-<run_id>` (push permission).
     - Workflow-file permission: create `.github/workflows/ci.yml` holding only the top-level keys and the `repo-checks` job of S-001 action 3, commit, push. A rejection that mentions `workflow` scope or permission means the credential cannot push workflow files.
     - Run and log reading: with `gh` (`gh run list --branch impl-<run_id> --limit 1`, then `gh run view <id> --log | head -5`) or any equivalent GitHub tool or REST call you have, list that push's run and read the first lines of its job log. `gh` is not required; some way of reading run conclusions and job logs is.
     - `git ls-remote https://github.com/juce-framework/JUCE 'refs/tags/9.0.5*'` and `git ls-remote https://github.com/catchorg/Catch2 'refs/tags/v3.16.1*'`: one of the printed lines (the tag itself, or its peeled `^{}` line when the tag is annotated) starts with the commit of `plan/ENVIRONMENT.md`.
  6. Recorded reachability checks (a failure is written to `EXECUTION_LOG.md` and `.checkpoints/state.json`, and is not blocking, because CI runners do these downloads; it decides where "locally or in CI" Done-when commands are met): `curl -sSIL https://vcvrack.com/downloads/Rack-SDK-2.6.6-lin-x64.zip | head -1`; `curl -sSIL https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_Linux.zip | head -1`; `curl -sS -X POST -d '{"commit":"d60b7a78030661574a615b586d9325ba7a7fa7dd"}' https://api.osv.dev/v1/query | head -c 200`; `git ls-remote https://github.com/mull-project/mull | head -1`; `git ls-remote https://github.com/qualitycoding/Lessons | head -1` and the same for `qualitycoding/knowledge` (the stores of S-KNOW). Also record whether you can start sub-agents (`HANDOFF.md` section 9.1).
  7. Create `EXECUTION_LOG.md` with the header `| UTC time | Step | Attempt | Outcome | Commit | Note |` and this step's line. Create empty `DEVIATIONS.md` with the header `| UTC time | Step | Deviation | Reason | Reversible? |`.
- Outputs: branch `impl-<run_id>`; `EXECUTION_LOG.md`; `DEVIATIONS.md`; the first `.github/workflows/ci.yml` (one job).
- Evidence produced: none
- Done when: actions 3, 4 and 5 gave the expected results; action 6's results are recorded; both files exist and are pushed.
- Checkpoint: `.checkpoints/state.json` (schema: `HANDOFF.md` section 9.7): `phase: "impl"`, `active_step_id: "S-000"`, and the `implementer` object.
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
  2. Create `deps.lock.json` with the keys `juce`, `catch2`, `pluginval`, `rack-sdk` exactly as follows (version strings without a leading `v`, as T-905 requires; only `juce` and `catch2` carry a `commit` key, because S-034 queries exactly the entries that have one; `rack-sdk.sha256` is filled by S-030):
     ```json
     {
       "juce": {"version": "9.0.5", "commit": "d60b7a78030661574a615b586d9325ba7a7fa7dd"},
       "catch2": {"version": "3.16.1", "commit": "08092139210881f01a34e60c5f2fd6afa0ea4024"},
       "pluginval": {"version": "1.0.4", "sha256": {
         "Linux": "c01c49d8063965c4c2dea8324468336768f5c9139e0b1caebde14c2400b55352",
         "macOS": "3c4c533bda0c5059eea3ddaea752d757ee2025041f0f47e6bcb0e87f6082b29f",
         "Windows": "c08e61ce3b96db41636f8ec7e76f4c7e2c13ebdac7fa1b5a1f52b4f32ec715ab"}},
       "rack-sdk": {"version": "2.6.6", "sha256": {}}
     }
     ```
  3. Complete `.github/workflows/ci.yml` (begun in S-000): `on: [push, pull_request]`; top-level `permissions:` with `contents: read`; `concurrency` group `ci-${{ github.ref }}` with `cancel-in-progress: true`; a top-level `env:` entry `MUFF_TEST_FILTER: "T-001*,T-002*,T-003*,T-020*,T-021*,T-022*,T-023*,T-024*"` (the temporary filter: a comma-separated list of Catch2 test-name patterns, in which `*` is the only wildcard; widened by later steps and removed by S-018); the jobs `repo-checks`, `core-linux`, `core-sanitizers`, `core-linux-arm64`, `core-musl`, `core-oldest-toolchain` (a container job, like `core-musl`: no JavaScript actions, source fetched by `git`, per D-009), `core-windows`, `core-macos`, `core-macos-intel` and `aggregate` exactly as tabled in D-009 (configure, build and binary paths per platform: `plan/ENVIRONMENT.md`, "Per-platform command lines"), except that until S-018 each core job runs the test binary with the argument `"$MUFF_TEST_FILTER"` instead of `ctest` and does not run `muff_bench`, and `repo-checks` runs only `python3 tools/check_frozen.py` and `python3 tools/gen_expected.py --check tests/fixtures/effects.json tests/core/expected_effects.inc`. Every `uses:` is pinned to the commit in `plan/ENVIRONMENT.md`. Every job sets `timeout-minutes`. Each job's first step prints the tool versions it relies on. In the seven jobs that are not container jobs, test logs are written with `2>&1 | tee <job>.log` under `shell: bash` with `set -euo pipefail`, and uploaded with `actions/upload-artifact` under `if: always()` and `if-no-files-found: error`. In the two container jobs (`core-musl`, `core-oldest-toolchain`) the test binary is run directly under `shell: sh` with `set -eu`, its output goes to the job log, and no action is used.
  4. `aggregate`: `if: always()`, `needs:` every other job that exists in the file at that time (later steps add their jobs to the list), one step that fails unless every entry of `${{ toJSON(needs) }}` has `"result": "success"` (use `python3 -c` reading the JSON from an environment variable).
  5. Push and read the run. The core is still stubs at this step, so the test step of each core job is expected to fail; what this step proves is that every job builds and reaches its test step on every platform. Record each job's conclusion and the name of its failing step in `EXECUTION_LOG.md`. `aggregate` is red at this step and stays red until S-018; that is expected.
- Outputs: `.gitignore`, `deps.lock.json`, `.github/workflows/ci.yml`.
- Evidence produced: none
- Done when: `python3 tools/check_frozen.py` exits 0; the pushed commit's workflow run shows, for each of the eight core jobs, the build step succeeded and the test step ran (its log contains `test cases:`); `repo-checks` is green.
- Checkpoint: `active_step_id: "S-001"`, run URL.
- On failure: a job fails before its test step: fix the workflow or the build wiring (not the frozen `tests/CMakeLists.txt`); three attempts per job, then `BLOCKED.md`. A runner label that GitHub rejects: stop and write `BLOCKED.md` (labels are frozen by T-905's job list and D-009).
- Gate: none
- Relevant decisions/claims: D-009, D-005; C-PLANNER-009, C-PLANNER-010, C-PLANNER-011
- Lessons applied: `L-20261029T000000Z-green-ci-gate-never-ran`, `L-20261031T100000Z-actions-run-steps-have-no-pipefail`, `L-20261028T200000Z-pinned-tool-version-does-not-exist`, `L-20261009T150801Z-the-plan-s-ci-workflow-template-failed-to-parse-because-gha-`
- Exclusive resources: `.github/workflows/ci.yml`

### S-010 Core foundation: tables, lookups, normalisation, building blocks
- Tier: Sonnet
- Profile: software
- Depends on: S-001
- Inputs: `core/include/muff/core.hpp`, `core/include/muff/blocks.hpp`, `tests/fixtures/effects.json`, `tools/gen_tables.py`, D-011, D-014, D-015, D-017.
- Actions:
  1. `python3 tools/gen_tables.py tests/fixtures/effects.json core/src/generated_tables.inc`; commit the output.
  2. Delete `core/src/stubs.cpp`. Create `core/src/tables.hpp` (private header, namespace `muff::detail`: it declares only the two accessors `int patternLength(int pattern, int channel) noexcept;` and `unsigned long long patternMask(int pattern, int channel) noexcept;`, with pattern 0 to 23 and channel 0 to 7; it does not declare the generated `PatternChannel` type, which stays private to `tables.cpp`), `core/src/tables.cpp` (the only file that includes `generated_tables.inc`; implements those two accessors over `kPatterns` and `effectInfo`, `findParam`, `findInput`, `findOutput`, `findMeter`, `toNormalised`, `fromNormalised` exactly as the header comments specify), `core/src/blocks.cpp` (`softClip`, `lfoShape`, `TapTempo`, `filterBankEnvelopeTimes`), `core/src/effect_base.hpp` and `.cpp` (a private base class providing: parameter storage with clamping and rounding; connection flags; input sanitising, output limiting and denormal protection of D-012; 5 ms smoothing of D-017; the `active` cross-fade; the level meter; `prepare` validation; `latencySamples()` returning 0, which no effect overrides), `core/src/factories.hpp` (declares, in namespace `muff::detail`, the seven functions `std::unique_ptr<muff::Effect> makeLowpass();`, `makeRingMod`, `makePhaser`, `makeDelay`, `makeFilterBank`, `makeOscBox`, `makeChorus`; each is defined later by its effect's step in that effect's `.cpp`), and `core/src/create.cpp` (`createEffect`: a `switch` on the id with one `case` line per effect and `default: throw std::out_of_range`; at this step every `case` line is `case EffectId::<Enum>: throw muff::NotImplemented(...);`; the step that writes an effect replaces only its own line with `case EffectId::<Enum>: return detail::make<Enum>();`, which is what "register it" means in S-011 to S-017).
  3. Licence headers: `HANDOFF.md` section 9.5 (applies to every step).
- Outputs: the files named above.
- Evidence produced: T-001, T-002, T-003, T-020, T-021, T-022, T-023, T-024
- Done when: `build/core/tests/muff_core_tests "T-001*,T-002*,T-003*,T-020*,T-021*,T-022*,T-023*,T-024*"` reports `All tests passed` with 8 test cases; `python3 tools/check_frozen.py` exits 0; the CI run of the step commit is green in all eight core jobs with the filter of S-001 (which names the same eight tests).
- Checkpoint: `active_step_id: "S-010"`.
- On failure: fix and re-run; a test that seems wrong: `HANDOFF.md` section 6 (test challenge), never edit the test.
- Gate: none
- Relevant decisions/claims: D-011, D-012, D-014, D-015, D-017
- Lessons applied: `L-20261008T173531Z-editor-note-labels-diverged-from-midi-map`, `L-20261007T150300Z-hand-typed-frozen-constant`
- Exclusive resources: none

### S-011 Lowpass
- Tier: Opus
- Profile: software
- Depends on: S-010
- Inputs: D-018, D-020, D-030, D-037; `research/spikes/oracle/oracle_core.cpp` may be read as a satisfiability reference (it is not a quality bar and must not be copied wholesale: it lacks the structure of S-010).
- Actions: create `core/src/lowpass.cpp` implementing the effect per D-020 and D-030 (including the resonance fade above 16 kHz) on the base class of S-010; register it in `core/src/create.cpp`. `tailSeconds()`: `1.0e9f` when `resonance >= 8`; `60.0f` when `4 <= resonance < 8`; else `2.0f` (bounds from the linear ladder at its lowest cutoff of 10 Hz: the slowest pole decays 60 dB in under 36 s just below self-oscillation and in under 0.7 s at resonance 4). In `.github/workflows/ci.yml` append `,[lowpass]` to `MUFF_TEST_FILTER`.
- Outputs: `core/src/lowpass.cpp`; edits to `create.cpp`, `ci.yml`.
- Evidence produced: T-101 to T-109
- Done when: `build/core/tests/muff_core_tests "[lowpass]"` reports `All tests passed` with 9 test cases; CI green in all core jobs.
- Checkpoint: `active_step_id: "S-011"`.
- On failure: three implementation attempts; if T-103 (self-oscillation frequency) cannot be met within 2 % with the D-030 equations, stop and write `BLOCKED.md` quoting the measured frequencies (the oracle meets it, so this indicates an implementation defect; do not change the algorithm family without re-opening Phase 2).
- Gate: none
- Relevant decisions/claims: D-020, D-030; C-DSP-008, C-DSP-010, C-DSP-011, C-PEDA-002
- Lessons applied: none specific
- Exclusive resources: none (the edits to `create.cpp` and `ci.yml` are one line each; if effect steps run concurrently, each runs `git pull --rebase` before pushing, and a conflict in the `MUFF_TEST_FILTER` line is resolved by keeping every tag from both sides)

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
- Actions: create `core/src/phaser.cpp` per D-022 and D-032; register it; `tailSeconds()`: with `g = 0.09 * resonance`, `fmin = max(20, 1100 * 2^(6 * (sweep / 10 - 0.5) - 3 * lfo_amount))` Hz and `D = stages / (pi * fmin)` seconds (the chain's low-frequency group delay, `stages` being 6 or 12): `0.1f` if `g <= 0.001`, else `0.1 + 2 * D * (1 + 6.908 / -ln(g))` (at most 25.6 s, so never above 60); append `,[phaser]` to `MUFF_TEST_FILTER`.
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
- Actions: create `core/src/clocked_line.hpp` (class `muff::detail::ClockedLine`, the clocked delay line of D-033, later also used by S-015; header-only; interface: `void size(int samples)` called from `prepare`, `void clear()`, and per sample one write of the input together with the current delay in samples and one read; buffers are sized in `size`, never in `process`; `research/spikes/oracle/oracle_core.cpp` contains a working `ClockedLine` that may be read for the start-up behaviour D-033 describes) and `core/src/delay.cpp` per D-023; register it; `tailSeconds()`: with `g = feedback / 8`: `1.0e9f` if `g >= 1`; `0.9f` if `g <= 0.001`; else `t = 0.8f * (1 + 6.908f / -ln(g)) + 0.1f`, returned as `1.0e9f` when `t > 60`; append `,[delay]` to `MUFF_TEST_FILTER`.
- Outputs: `core/src/delay.cpp`, `core/src/clocked_line.hpp`; edits to `create.cpp`, `ci.yml`.
- Evidence produced: T-401 to T-409
- Done when: `muff_core_tests "[delay]"`: `All tests passed`, 9 test cases; CI green in all core jobs.
- Checkpoint: `active_step_id: "S-014"`.
- On failure: as S-011.
- Gate: none
- Relevant decisions/claims: D-023, D-033; C-PEDB-003, C-PEDB-012
- Lessons applied: none specific
- Exclusive resources: none

### S-015 Chorus
- Tier: Opus
- Profile: software
- Depends on: S-010, S-014
- Inputs: D-014, D-015, D-018, D-026, D-033, D-036; `core/src/clocked_line.hpp` (from S-014).
- Actions: create `core/src/chorus.cpp` per D-026, D-033 and D-036 (using `core/src/clocked_line.hpp` unchanged; a defect found in it is fixed there and S-014's Done-when command is re-run); register it; `tailSeconds()`: with `g = 1.25 * |feedback|`: `1.0e9f` if `g >= 1`; `0.15f` if `g <= 0.001`; else `t = 0.05f * (1 + 6.908f / -ln(g)) + 0.1f`, returned as `1.0e9f` when `t > 60`; append `,[chorus]` to `MUFF_TEST_FILTER`.
- Outputs: `core/src/chorus.cpp`; edits to `create.cpp`, `ci.yml`.
- Evidence produced: T-701 to T-707
- Done when: `muff_core_tests "[chorus]"`: `All tests passed`, 7 test cases; CI green in all core jobs.
- Checkpoint: `active_step_id: "S-015"`.
- On failure: as S-011.
- Gate: none
- Relevant decisions/claims: D-026, D-033, D-036
- Lessons applied: none specific
- Exclusive resources: none

### S-016 FilterBank
- Tier: Opus
- Profile: software
- Depends on: S-010
- Inputs: D-014, D-018, D-024, D-034; the pattern table, read only through `muff::detail::patternLength` and `patternMask` of `core/src/tables.hpp` (S-010).
- Actions: create `core/src/filterbank.cpp` per D-024 and D-034; register it; `tailSeconds()` returns `0.5f` (the sequencer and envelopes only scale the bands and make no sound of their own; a band of Q 5 at the lowest centre, 110 Hz shifted down half an octave, rings 60 dB down in under 0.2 s); append `,[filterbank]` to `MUFF_TEST_FILTER`.
- Outputs: `core/src/filterbank.cpp`; edits to `create.cpp`, `ci.yml`.
- Evidence produced: T-501 to T-507
- Done when: `muff_core_tests "[filterbank]"`: `All tests passed`, 9 test cases (T-501 to T-507, and T-023 and T-024, which carry the same tag and already pass since S-010); CI green in all core jobs.
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
- Actions: create `core/src/oscbox.cpp` per D-025 and D-035 (the one-sample-late oscillator with band-limited steps at wraps, pulse edges and sub-sample-timed sync resets, exactly as D-035 states); register it; `tailSeconds()` returns `1.0f` (the oscillator's level follows the input envelope, whose release D-025 fixes); append `,[oscbox]` to `MUFF_TEST_FILTER`.
- Outputs: `core/src/oscbox.cpp`; edits to `create.cpp`, `ci.yml`.
- Evidence produced: T-601 to T-607
- Done when: `muff_core_tests "[oscbox]"`: `All tests passed`, 7 test cases; CI green in all core jobs.
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
  3. Run the sanitizer commands of `plan/ENVIRONMENT.md` locally (the `g++` package of Ubuntu 24.04 brings the sanitizer run-time libraries; if your machine is not Linux with GCC, the `core-sanitizers` job log serves: locally or in CI). Until this step the jobs ran only the filtered tests, so this is the first time the whole suite runs under the sanitizers, musl, GCC 11, MSVC and Apple Clang; a failure that appears only there belongs to the step that owns the failing test's effect.
- Outputs: `adapters/src/adapters.cpp`; `ci.yml`.
- Evidence produced: T-004 to T-018, T-801, T-802, T-811, T-812, T-821, T-PERF-1, T-CI-SAN
- Done when: `ctest --test-dir build/core --output-on-failure` reports `100% tests passed, 0 tests failed out of 80`; the sanitizer binary reports `All tests passed` (79 test cases); `build/core/tests/muff_bench --limit 5` exits 0 and prints seven lines; the CI run is green in `repo-checks`, all eight core jobs and `aggregate`; `grep -c MUFF_TEST_FILTER .github/workflows/ci.yml` prints 0.
- Checkpoint: `active_step_id: "S-018"`, the seven benchmark numbers from the `core-linux` log.
- On failure: a common test fails for one effect: return to that effect's step. `muff_bench` at or above 5 % for an effect on `core-linux`: time that effect's stages with `std::chrono::steady_clock` in a scratch copy of the benchmark outside `tests/` (not committed), optimise (hoist `tan`/`pow` calls to control rate with per-sample linear interpolation of coefficients, at most every 16 samples; no change may alter any frozen test's result; whether a change is audible beyond the tests is put to the owner at G-LISTEN, so list each optimisation in `EXECUTION_LOG.md`), three attempts, then `BLOCKED.md`; never raise the limit (D-050). The gate failing once and passing on re-run of the same commit: R-009.
- Gate: none
- Relevant decisions/claims: D-013, D-050; C-PLANNER-017, C-PLANNER-018
- Lessons applied: `L-20261101T041800Z-perf-harness-must-exercise-what-it-reports`, `L-20261009T234100Z-intake-defaults-and-draft-numbers-stated-before-measurement`
- Exclusive resources: `.github/workflows/ci.yml`

### S-020 VST3 processor shell
- Tier: Opus
- Profile: software
- Depends on: S-018
- Inputs: `vst3/include/muff/vst3_shell.hpp` (the contract), `vst3/CMakeLists.txt`, D-016; `research/spikes/oracle_vst3/processor.cpp` as a satisfiability reference.
- Actions:
  1. Delete `vst3/src/stubs.cpp`; create `vst3/src/processor.cpp` (the `juce::AudioProcessor` of the contract; parameter values are read from the parameter objects, which are atomics, at the top of `processBlock`; `juce::ScopedNoDenormals` in `processBlock`; `hasEditor()` returns true and, until S-021 supplies the real editor, `createEditor()` returns `new juce::GenericAudioProcessorEditor (*this)`).
  2. Add job `vst3-linux` to `ci.yml` (D-009): install the JUCE package list, configure with `-DMUFF_BUILD_VST3=ON`, build target `muff_vst3_tests`, run it with `xvfb-run -a`. Add it to `aggregate`'s `needs`.
- Outputs: `vst3/src/processor.cpp`; `ci.yml`.
- Evidence produced: T-851 to T-856
- Done when: `xvfb-run -a ctest --test-dir build/vst3 -C Release -V -R muff_vst3_tests` (the `-V` makes `ctest` print the binary's own output) ends with `100% tests passed, 0 tests failed out of 1` and shows the binary's summary `All tests passed` with 6 test cases; CI green in `vst3-linux`.
- Checkpoint: `active_step_id: "S-020"`.
- On failure: three attempts; a JUCE API difference from the contract (a method the contract names does not exist in 9.0.5): stop and write `BLOCKED.md` (the oracle shell compiled against 9.0.5, so first diff against it).
- Gate: none
- Relevant decisions/claims: D-016, D-005; C-PLANNER-002, C-PLANNER-016
- Lessons applied: `L-20261009T160900Z-juce-api-assumed-from-memory-compile-caught-and-version-floor-unchecked`, `L-20261008T173536Z-juce-glue-delivered-uncompiled`, `L-20261028T215000Z-vst3-validator-not-in-local-suite`
- Exclusive resources: `.github/workflows/ci.yml`

### S-021 Editor, seven plug-in targets, validation on all platforms
- Tier: Sonnet
- Profile: software
- Depends on: S-020
- Inputs: D-005 (table of targets), D-007, D-016 (editor description), `plan/ENVIRONMENT.md` (pluginval hashes and URLs).
- Actions:
  1. Create `vst3/src/editor.hpp`, `vst3/src/editor.cpp` (the editor of D-016; `createEditor()` in `processor.cpp` returns it) and `vst3/src/entry/entry.cpp` (D-005).
  2. Below the marked line of `vst3/CMakeLists.txt` add the seven `juce_add_plugin` targets with the properties of D-005; each links `muff_vst3_sources` and `juce::juce_recommended_warning_flags`, adds `src/entry/entry.cpp`, and defines `MUFF_EFFECT_ID`.
  3. Create `tools/fetch_pluginval.py <platform> <dest>`: downloads the archive for `Linux`, `macOS` or `Windows`, verifies its SHA-256 against `deps.lock.json` before unpacking, exits non-zero on mismatch or after three failed attempts 30 s apart; after unpacking it finds the one file under `<dest>` named `pluginval` (Linux; on macOS it is inside `pluginval.app/Contents/MacOS/`) or `pluginval.exe`, fails if there is not exactly one, makes it executable, on macOS removes the quarantine attribute from `<dest>` (`xattr -dr com.apple.quarantine`, ignoring "no such attribute"), and prints the binary's absolute path as its last line of output.
  4. Create `tools/run_pluginval.py <pluginval-binary> <vst3-bundle>...`: for each bundle runs the command of D-007, captures the log to `pluginval-<name>.log`, and applies the pass rule of D-007; exits non-zero if any bundle fails. With `--self-test <pluginval-binary>` it creates an empty directory `NotAPlugin.vst3`, applies the same rule and exits 0 only if the rule reports failure (D-007 negative control); every validating job runs the self-test first.
  5. Add jobs `vst3-windows`, `vst3-macos`, `vst3-macos-intel` and extend `vst3-linux`: compiler cache on Linux and macOS as D-005 states; build all targets; run `muff_vst3_tests`; on Linux, Windows and macOS arm64 run steps 3 and 4 over the seven bundles (Linux under `xvfb-run -a`). Upload the pluginval logs (text) with `if: always()`. No bundle is uploaded.
- Outputs: the files named; `ci.yml`.
- Evidence produced: T-CI-PLUGINVAL; first part of T-901 (plug-in targets exist)
- Done when: on Linux, `cmake --build build/vst3 --config Release --parallel` produces seven bundles, listed by `find build/vst3 -name '*.vst3' -type d` (expected under `build/vst3/vst3/Muff*_artefacts/Release/VST3/`), each containing a shared library larger than 1 MB (print sizes with `find build/vst3 -name '*.so' -path '*.vst3*' -size +1M`); `PV=$(python3 tools/fetch_pluginval.py Linux build/pluginval | tail -1)` then `xvfb-run -a python3 tools/run_pluginval.py --self-test "$PV"` and `xvfb-run -a python3 tools/run_pluginval.py "$PV" $(find build/vst3 -name '*.vst3' -type d)` both exit 0 (locally or in CI); CI green in all four `vst3-*` jobs and `aggregate`.
- Checkpoint: `active_step_id: "S-021"`, per-platform build minutes from the job summaries.
- On failure: pluginval rejects an option of D-007's command line (`--seed`): print `pluginval --help`, drop only that option, record it in `DEVIATIONS.md`. pluginval reports a failure: re-run once with the same seed to tell a repeatable failure from an intermittent one and record both logs; then fix the shell or editor (the contract is fixed); if the failing check contradicts the frozen contract, stop and write `BLOCKED.md` (R-011). A VST3 job over 45 min (the compiler cache of D-005 is already in place on Linux and macOS): record the per-target build times in `DEVIATIONS.md`, write `BLOCKED.md` for this step (D-005's revisit rule and R-008: the next measure is a build-structure change that needs review), and continue with steps that do not depend on S-021; a job that finishes inside its 60-minute limit still counts as green meanwhile. pluginval exits 0 for a real plug-in but its log lacks the strings of D-007's pass rule: print the last 40 log lines, change the rule's strings to ones present in the passing logs of all three platforms, keep the exit-status condition, confirm the negative control still fails, and record the change with the log excerpt in `DEVIATIONS.md`. pluginval download hash mismatch: stop and write `BLOCKED.md` (do not update the hash).
- Gate: none
- Relevant decisions/claims: D-005, D-007, D-016; C-PLANNER-006, C-PLANNER-007, C-PLANNER-008
- Lessons applied: `L-20261028T200000Z-pinned-tool-version-does-not-exist`, `L-20261028T215000Z-vst3-validator-not-in-local-suite`, `L-20261101T140000Z-intermittent-host-failure-needs-a-stress-test`, `L-20261009T160200Z-ui-compiled-but-never-run-and-smoke-test-abandoned`, `L-20261029T000000Z-green-ci-gate-never-ran`
- Exclusive resources: `.github/workflows/ci.yml`

### S-030 Rack SDK acquisition
- Tier: Sonnet
- Profile: software
- Depends on: S-018
- Inputs: D-003, `deps.lock.json`.
- Actions:
  1. Create `tools/fetch_rack_sdk.py <platform> <dest> [--record]`: downloads the archive of D-003 for `lin-x64`, `win-x64`, `mac-x64` or `mac-arm64` (three attempts, 30 s apart); without `--record` it verifies the SHA-256 against `deps.lock.json` and fails on mismatch or missing entry; with `--record` it skips verification and prints one line `RACK_SDK_RECORD {"platform": "<platform>", "sha256": "<hash>", "bytes": <size>}`, failing if the archive is smaller than 5 000 000 bytes (a floor chosen by the planner to reject an error page or a truncated download; an SDK archive holds headers and link libraries); then it unpacks, finds the directory that contains `plugin.mk`, moves it to `<dest>/Rack-SDK` if it is not already there, and asserts `<dest>/Rack-SDK/plugin.mk` and `<dest>/Rack-SDK/include/rack.hpp` exist.
  2. Add workflow `.github/workflows/record-rack-sdk.yml` (same permissions and pin rules; `on: push`, with the single job guarded by `if: startsWith(github.event.head_commit.message, '[record-rack-sdk]')`) whose `ubuntu-24.04` job runs the tool with `--record` for the four platforms; push a commit whose subject starts with `[record-rack-sdk]` (the commit that adds the workflow must not start so; use `git commit --allow-empty -m "[record-rack-sdk] first recording"` afterwards) and read the job log.
  3. Create `tools/apply_rack_record.py <logfile> <run URL>`: reads the four `RACK_SDK_RECORD` lines from a saved job log and writes `rack-sdk.sha256.<platform>` and `rack-sdk.bytes.<platform>` into `deps.lock.json`, with the sibling key `"sha256_note": "trust on first use: recorded from vcvrack.com on <UTC date of the run> by run <run URL>"`; it fails unless exactly four distinct platforms are present. Save the log (`gh run view <id> --log > build/rack-record.log` or the equivalent) and run the tool, so that no hash is typed by hand (Rule 9).
- Outputs: `tools/fetch_rack_sdk.py`, `tools/apply_rack_record.py`, `.github/workflows/record-rack-sdk.yml`, `deps.lock.json`.
- Evidence produced: first part of T-905 (lock file complete)
- Done when: `python3 -m unittest discover -s tests/py -k test_dependency_lock` reports `Ran 1 test` and `OK`; `deps.lock.json` holds four `rack-sdk.bytes` values, each at least 5000000.
- Checkpoint: `active_step_id: "S-030"`, the four hashes.
- On failure: download fails three times in the workflow: wait 30 min, push another marker commit (`git commit --allow-empty -m "[record-rack-sdk] retry"`), at most three runs, then `BLOCKED.md` (D-003). The archive does not contain a directory with `plugin.mk`: `BLOCKED.md` with the archive's file listing (the layout is an assumption from the knowledge store, never downloaded in planning).
- Gate: none
- Relevant decisions/claims: D-003; C-PLANNER-014
- Lessons applied: L-20261111T151500Z-download-output-committed-unchecked
- Exclusive resources: `deps.lock.json`

### S-031 Rack panels
- Tier: Sonnet
- Profile: software
- Depends on: S-018
- Inputs: D-006 (panel rules and widths), `tests/fixtures/effects.json`.
- Actions: create `tools/gen_panels.py` which, for each effect, writes `rack/res/<Enum>.svg` (width `HP * 5.08` mm, height 128.5 mm, matching `viewBox`, background rectangle, the effect name at the top, a label under every control and port position, all as `<path>` elements from the built-in stroke-letter table) and `rack/src/generated_layout.hpp` (for each effect, arrays of millimetre positions for parameters, inputs, outputs and lights in table order). Layout rule, the same for every effect (positions are centres in mm from the panel's top-left corner; `W` is the panel width; this is the planner's design and its fit was checked by arithmetic against the fixture: the largest cases are FilterBank with 19 controls in 4 rows of 6, and Delay and Chorus with 11 ports in 2 rows of 8): title: the effect name in letters 4 mm high, left edge at x = 3, baseline at y = 9. Lights: meter `k` (0, 1, 2) at x = `W - 5.08 - 5.08 k`, y = 14. Controls: `n = floor(HP / 3)` columns at x = `7.62 + 15.24 i`, rows at y = 26, 42, 58, 74, filled row by row in table order; each control's label is centred at y + 9. Ports: `m = HP / 2` columns at x = `5.08 + 10.16 j`, rows at y = 97, 109, 121, filled row by row with the inputs in table order followed by the outputs in table order; each port's label is centred at y - 6.5; behind each output a filled square of side 9 mm in a darker shade marks it as an output. Labels: the table name in upper case, letters 2 mm high, shrunk in steps of 0.1 mm down to 1.2 mm until the label is at least 1 mm narrower than its column pitch; the script fails if a label still does not fit, if any control row would pass y = 74, or if any port row would pass y = 121. `python3 tools/gen_panels.py --check` exits 1 if committed files differ from regenerated ones. Run it and commit the outputs.
- Outputs: `tools/gen_panels.py`, `rack/res/*.svg` (7), `rack/src/generated_layout.hpp`.
- Evidence produced: T-903
- Done when: `python3 -m unittest discover -s tests/py -k test_panels` reports `Ran 1 test` and `OK`; `python3 tools/gen_panels.py --check` exits 0; each SVG is between 2 kB and 400 kB (planner's sanity bounds: a panel with every label drawn as paths is tens of kB; under 2 kB means the labels are missing).
- Checkpoint: `active_step_id: "S-031"`.
- On failure: three attempts, then `BLOCKED.md`.
- Gate: none
- Relevant decisions/claims: D-006; C-LEGAL-014, C-LEGAL-015
- Lessons applied: `L-20261008T173531Z-editor-note-labels-diverged-from-midi-map` (positions and labels generated from the one table)
- Exclusive resources: none

### S-032 Rack module shells and manifest
- Tier: Opus
- Profile: software
- Depends on: S-030, S-031
- Inputs: D-006, D-012, D-013, `rack/src/generated_layout.hpp`, the Rack SDK headers (unpacked by `tools/fetch_rack_sdk.py lin-x64 .`).
- Actions:
  1. Create `rack/plugin.json` with the fields T-902 checks: slug, name and brand `Mufferfluffer`; version `2.0.0`; license `AGPL-3.0-only`; author `qualitycoding`; sourceUrl `https://github.com/qualitycoding/mufferfluffer`; seven modules with `slug` = the fixture's `enum`, `name` = the fixture's `name`, the description given here, and tags (descriptions: Lowpass "Resonant 2-pole or 4-pole ladder low-pass filter with envelope follower and drive"; RingMod "Ring modulator with a wide-range sine carrier and an LFO"; Phaser "6-stage or 12-stage phaser with resonance and a two-range LFO"; Delay "Dark analogue-style delay with feedback loop insert, six-shape LFO and tap tempo"; FilterBank "Eight-band resonant filter bank animated by a pattern sequencer"; OscBox "Oscillator that follows your signal: hard sync, FM and envelope-controlled level"; Chorus "Chorus and flanger with positive or negative feedback and a six-shape LFO"): Lowpass `["Filter", "Effect", "Hardware clone"]`; RingMod `["Ring modulator", "Effect", "Hardware clone"]`; Phaser `["Phaser", "Effect", "Hardware clone"]`; Delay `["Delay", "Effect", "Hardware clone"]`; FilterBank `["Filter", "Sequencer", "Effect", "Hardware clone"]`; OscBox `["Oscillator", "Effect", "Hardware clone"]`; Chorus `["Chorus", "Flanger", "Effect", "Hardware clone"]`.
  2. Create `rack/Makefile` (D-006), `rack/src/plugin.hpp`, `rack/src/plugin.cpp` (registers seven models) and `rack/src/module_shell.hpp` (two class templates, `template <muff::EffectId ID> struct MuffModule : rack::engine::Module` and `template <muff::EffectId ID> struct MuffWidget : rack::app::ModuleWidget`, built from `muff::effectInfo(ID)` and the generated layout, following D-006), plus `rack/src/<Enum>.cpp` for each effect holding only its licence header, the include and `rack::plugin::Model* model<Enum> = rack::createModel<MuffModule<muff::EffectId::<Enum>>, MuffWidget<muff::EffectId::<Enum>>>("<Enum>");`. Licence file in the package: Rack's `plugin.mk` copies `DISTRIBUTABLES` with their relative paths, so `../LICENSE` cannot be listed; the Makefile instead has the rule `LICENSE: ../LICENSE` with the recipe `cp ../LICENSE LICENSE`, the lines `DISTRIBUTABLES += res LICENSE` before the `include` and `dist: LICENSE` after it, and `.gitignore` gains the line `rack/LICENSE`.
  3. Build on Linux: `python3 tools/fetch_rack_sdk.py lin-x64 .` then `make -C rack RACK_DIR="$PWD/Rack-SDK" dist`.
- Outputs: `rack/plugin.json`, `rack/Makefile`, `rack/src/*`.
- Evidence produced: T-902; T-906 precondition (`rack/src` exists)
- Done when: `python3 -m unittest discover -s tests/py -k test_manifest` reports `Ran 1 test` and `OK`; locally or in CI (a temporary `rack-linux` job running these commands, which S-033 then completes): `make -C rack RACK_DIR="$PWD/Rack-SDK" dist` exits 0, `ls -l rack/dist/*.vcvplugin` shows exactly one file `Mufferfluffer-2.0.0-lin-x64.vcvplugin` larger than 100 kB, and `nm -D --defined-only rack/plugin.so | grep -c ' init$'` prints 1.
- Checkpoint: `active_step_id: "S-032"`.
- On failure: a Rack API named in D-006 does not exist in SDK 2.6.6 (`configSwitch`, `configBypass`, `getVoltageSum`, `ParamQuantity`): read `Rack-SDK/include/engine/Module.hpp` and `Port.hpp`, use the present equivalent, and record it in `DEVIATIONS.md`; anything that changes the user-visible behaviour of D-006: `BLOCKED.md`.
- Gate: none
- Relevant decisions/claims: D-006, D-004; C-PLANNER-004, C-PLANNER-015, C-LEGAL-011
- Lessons applied: `L-20261009T234102Z-draft-test-unsatisfiable-found-by-oracle-t-021`, `L-20261105T214500Z-sdk-source-channel`
- Exclusive resources: none

### S-033 Rack builds on four platforms
- Tier: Sonnet
- Profile: software
- Depends on: S-032
- Inputs: D-009, `plan/ENVIRONMENT.md` (packages), `tools/fetch_rack_sdk.py`.
- Actions: add jobs `rack-linux`, `rack-windows` (steps under `msys2/setup-msys2` with `msystem: MINGW64` and `shell: msys2 {0}`), `rack-macos`, `rack-macos-intel`: fetch and verify the SDK for the job's platform, `make -C rack RACK_DIR=... dist` (on `rack-linux` also: `cmake -S . -B build/rackflags -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=nehalem -funsafe-math-optimizations -fno-omit-frame-pointer" && cmake --build build/rackflags --parallel && build/rackflags/tests/muff_core_tests`, the flags of D-006), then `python3 tools/check_rack_package.py rack/dist` (new tool: asserts exactly one `*.vcvplugin` file larger than 100 kB whose name contains the platform token; unpacks it (it is a zstd-compressed tar: `zstd -d` then `tar -t`) and asserts it contains `Mufferfluffer/plugin.json`, the platform's plug-in library (`plugin.so`, `plugin.dll` or `plugin.dylib`), `Mufferfluffer/LICENSE*` and the seven `res/*.svg`). Add the four jobs to `aggregate`. No package is uploaded.
- Outputs: `tools/check_rack_package.py`; `ci.yml`.
- Evidence produced: T-CI-RACKPKG
- Done when: CI green in the four `rack-*` jobs and `aggregate`; each job's log shows the package name and size.
- Checkpoint: `active_step_id: "S-033"`.
- On failure: a platform fails to build: fix forward (three attempts per platform), then `BLOCKED.md`. The package lacks `LICENSE`: correct the Makefile rule of S-032 action 2.
- Gate: none
- Relevant decisions/claims: D-003, D-006, D-009
- Lessons applied: `L-20261106T063000Z-zstd-cli-shim`, `L-20261007T150900Z-windows-sha256sum-backslash`, `L-20261028T210000Z-crlf-breaks-hash-manifests-on-windows`, `L-20261029T000000Z-green-ci-gate-never-ran`
- Exclusive resources: `.github/workflows/ci.yml`

### S-034 Dependency vulnerability scan
- Tier: Sonnet
- Profile: software
- Depends on: S-030
- Inputs: D-061, `deps.lock.json`.
- Actions: create `tools/check_advisories.py <lockfile>` per D-061 (standard library `urllib`; prints one line per dependency: name, commit, number of advisories; for entries without a `commit` key (Rack SDK, pluginval archives) prints `not queried: no commit pin`; the query URL is `https://api.osv.dev/v1/query` unless the environment variable `MUFF_OSV_URL` is set, which exists so that the failure path can be exercised; an optional top-level key `"advisory_allow": ["<advisory id>", ...]` in the lock file lists advisories judged not to affect this project, which the tool prints and does not count); add job `deps-scan` and add it to `aggregate`.
- Outputs: `tools/check_advisories.py`; `ci.yml`.
- Evidence produced: T-CI-ADVISORIES
- Done when: CI green in `deps-scan`; its log shows two queried commits (JUCE, Catch2) each with a count; locally or in CI (as extra steps of `deps-scan`): running the tool with a lock file whose JUCE commit is replaced by `0000000000000000000000000000000000000000` still exits 0 with count 0 (documents the "none known" limit), and running it with `MUFF_OSV_URL=https://api.osv.dev/v1/nonexistent` exits non-zero (shows the gate can fail).
- Checkpoint: `active_step_id: "S-034"`.
- On failure: the API reports an advisory for a pinned commit: read it; if it affects code this project uses, stop and write `BLOCKED.md` (a pin change re-opens `plan/ENVIRONMENT.md`); if it does not, record the advisory ID and the reason in `plan/RISKS.md` and add the ID to `advisory_allow` in `deps.lock.json`.
- Gate: none
- Relevant decisions/claims: D-061; C-PLANNER-019
- Lessons applied: `L-20261029T000000Z-green-ci-gate-never-ran`
- Exclusive resources: `.github/workflows/ci.yml`

### S-035 Documentation
- Tier: Sonnet
- Profile: software
- Depends on: S-021, S-033
- Inputs: T-907's requirements (`tests/py/test_repo.py`, class `T907Docs`); D-004, D-012, D-016, D-020 to D-027; the fixture.
- Actions: rewrite `README.md` with sections `## Build`, `## Install`, `## Tests`, `## Licence`. Its description of the product is these two sentences and no other statement about hardware: "Seven effects modelled on the documented behaviour of a family of analogue effect pedals, as VST3 plug-ins and VCV Rack 2 modules. This project is not affiliated with, endorsed by or derived from the products of any manufacturer." (the existing README names no manufacturer and the new one does not either; no claim of sounding like any hardware: no test or comparison supports one). `## Install` says: VST3 bundles go in `~/.vst3/` (Linux), `~/Library/Audio/Plug-Ins/VST3/` (macOS) or `C:\Program Files\Common Files\VST3\` (Windows), the standard locations of the VST3 format; the `.vcvplugin` file goes in the `plugins-<os>-<cpu>` folder of the Rack user folder, which Rack opens from its menu Help > Open user folder; and that no binaries are published, so both are built as `## Build` describes. The sentence "VST is a registered trademark of Steinberg Media Technologies GmbH." appears in `README.md` and in `docs/USER_GUIDE.md` (D-004); create `docs/USER_GUIDE.md` with one `## <effect name>` section per effect that names every parameter, input and output by its table name and describes it in one or two sentences from D-020 to D-026, including the D-027 design choices that a user would notice (own pattern set; no MIDI); the guide opens with a section `## Common behaviour` (before the effect sections) that states, from D-012, that control-voltage inputs reach the end of a control's travel at 5 V so a 0 to 10 V source saturates half-way, and, from D-016, the difference between the host's bypass and the Active control; inside an effect's section use no `## ` heading (T-907 cuts sections at the next `## `; use `### ` for sub-headings), and never write a manufacturer, product-family or model name anywhere in the guide; create `CHANGELOG.md` with a `0.1.0` entry. British English.
- Outputs: `README.md`, `docs/USER_GUIDE.md`, `CHANGELOG.md`.
- Evidence produced: T-907; T-901 (naming), T-906 (licence headers) become checkable
- Done when: `python3 -m unittest discover -s tests/py -v -k T901 -k T902 -k T903 -k T904 -k T906 -k T907 -k test_dependency_lock` reports `Ran 8 tests` and `OK` (`-k` is repeated because `unittest` has no "or"; the one remaining test, `test_workflows`, stays red until S-MUTATE adds `mutation.yml`); `grep -c "Steinberg Media Technologies" README.md docs/USER_GUIDE.md` prints 1 for each file; in `ci.yml`, `repo-checks` now also runs that same `unittest` command; CI green in `repo-checks` and `aggregate`.
- Checkpoint: `active_step_id: "S-035"`.
- On failure: T-901 reports a forbidden mark in a file: remove the mark from that file (T-901 scans every tracked file outside `research/`, `plan/`, `premortem/`, `reviews/`, `lessons/`, `knowledge/`, `.checkpoints/`, `evidence/`, `HANDOFF.md` and `LICENSE`, so it also scans the records of `HANDOFF.md` section 9.3). Three attempts, then `BLOCKED.md`.
- Gate: none
- Relevant decisions/claims: D-004, D-027
- Lessons applied: `L-20261102T232000Z-american-conventions-and-us-sources-for-british-english-user`, `L-20261009T234120Z-readme-claims-not-tied-to-a-test-or-to-the-segmentation-condition`
- Exclusive resources: `.github/workflows/ci.yml`

### S-REVIEW Code review
- Tier: Opus
- Profile: software
- Depends on: S-018, S-021, S-033, S-034, S-035
- Inputs: `git diff origin/<generation branch>...HEAD`; the frozen tests; `plan/DECISIONS.md`; D-062.
- Actions: one review of the whole implementation diff at this point (this is the form D-062 fixes). Three fresh-context reviewers (`HANDOFF.md` section 9.1) that wrote none of the code, each given the diff and one brief: (1) correctness against the design decisions D-011 to D-018, D-020 to D-027 and D-030 to D-037, and anti-patterns (dead code, duplication, hidden mutation, naming, needless complexity); (2) security and real-time safety (memory handling in delay lines and state parsing, allocation or locking on the audio thread, denormals, integer overflow in sample counters, division by zero at parameter extremes); (3) untested code paths (every new function mapped to a T- id or named as a gap) and dependency currency (JUCE, Catch2, pluginval and Mull: the pin compared with the newest release tag printed by `git ls-remote --tags --sort=-v:refname <repository URL> | head -5`; Rack SDK: the pin compared with the newest tag of `https://github.com/VCVRack/Rack`; the five actions: the pinned commit compared with the newest release tag of each action's repository; differences are listed as Low findings and no pin is changed in this step). Merge into `reviews/CODE_REVIEW.md` with one row per finding: id, severity (Rule 4, restated in `HANDOFF.md` section 7), file and line, description, resolution (fix commit or R- entry). Fix every Critical and High finding; record each fix of an implementer mistake as a lesson (Rule 11).
- Outputs: `reviews/CODE_REVIEW.md`; fix commits; lessons.
- Evidence produced: none
- Done when: `reviews/CODE_REVIEW.md` has no Critical or High row without a fix commit; the suite status is unchanged or better (`ctest` 80 of 80; `muff_vst3_tests` 6 of 6); CI green.
- Checkpoint: `active_step_id: "S-REVIEW"`, counts by severity.
- On failure: a finding needs a new test: `HANDOFF.md` section 9.4; a finding disputes a decision: record it as an R- entry at its severity and continue unless Critical or High, in which case `BLOCKED.md`.
- Gate: none
- Relevant decisions/claims: D-062
- Lessons applied: none specific
- Exclusive resources: none

### S-ADVERSARIAL Adversarial review
- Tier: Opus
- Profile: software
- Depends on: S-REVIEW
- Inputs: the built core library, test binaries and plug-ins; `plan/ASSUMPTIONS.md`; D-012, D-016.
- Actions: one fresh-context agent tries to break the product, writing throw-away programs against the public headers in `build/adversarial/` (not committed): parameter changes on every sample; sample rates at both limits and prepare/reset storms; block sizes 1 and 8192 in the VST3 shell; state blobs of 0 bytes, 64 MB of zeros, deeply nested XML, XML with 100 000 `param` children, values `nan`, `inf`, `1e400`; all inputs at +/-1e30 and alternating NaN; tap triggers every sample; 24 hours of simulated audio at 8 kHz for the Delay and for the FilterBank (the two effects with tap-tempo time counters; counter overflow); `RackIO` with every connection flag toggled each sample; denormal inputs (1e-38) with timing compared against normal inputs (slow-down above 3x is a finding); a host-style automation stress on the VST3 shell: one thread calls `setValueNotifyingHost` on every parameter in a tight loop and calls `getStateInformation`/`setStateInformation` while another runs `processBlock` for 60 s, built with `-fsanitize=thread` (configure a separate tree: `cmake -S . -B build/tsan -DMUFF_BUILD_VST3=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"`, link the throw-away program to `muff_vst3_sources` as `tests/vst3/CMakeLists.txt` does for its test, and run it under `xvfb-run -a`; if ThreadSanitizer stops at start-up with "unexpected memory mapping", run it under `setarch "$(uname -m)" -R`; a finding is a report with at least one stack frame in a file under `vst3/src/`, `core/` or `adapters/`; reports whose frames are all inside JUCE are listed in the review as not assessed). Each confirmed break becomes a new frozen test through `HANDOFF.md` section 9.4 or an R- entry with a severity of Rule 4. Output `reviews/ADVERSARIAL_REVIEW.md` (what was tried, what broke, resolution).
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
- Inputs: the skills `plan/protocol/skills/mutation-testing-in-ci/` and `plan/protocol/skills/mutation-sharding/` (read both `SKILL.md` files; copy `scripts/mull_score.py` and `scripts/mull_include_paths.py` to `tools/` and start the workflow from `references/example-mutation-workflow.yml` and `references/per-file-matrix.yml`; the skills-repository commit is in `plan/protocol/README.md`); D-060.
- Pre-decided record (protocol 3.8):
  - Tool and pin: Mull for LLVM 19 on `ubuntu-24.04` with `clang-19` and `libclang-rt-19-dev` from the Ubuntu 24.04 archive (`sudo apt-get install -y clang-19 libclang-rt-19-dev`), Mull itself installed from a release `.deb` of `https://github.com/mull-project/mull/releases` (sub-step 1 lists the releases' assets with `gh release list --repo mull-project/mull --limit 10` and `gh release view <tag> --repo mull-project/mull --json tagName,assets`, or the REST path `repos/mull-project/mull/releases`, using the workflow's `GITHUB_TOKEN` when run in CI, takes the newest release that has an asset for Ubuntu 24.04 and LLVM 19, and writes its tag, asset name and SHA-256 to `deps.lock.json` under `mull`; later runs verify the hash before `dpkg -i`). The Cloudsmith repository is not used. The runner executable is whatever that package installs (`ls /usr/bin/mull*` is printed; earlier packages named it `mull-runner-19`, newer ones `mull-runner`); the configuration file `mull.yml` (mutators: the tool's default set; `includePaths: ["core/src/.*", "adapters/src/.*"]`; `excludePaths: [".*generated_tables.*", ".*tests/.*", ".*_deps/.*"]`) is part of the pin.
  - Scope: `core/src/*.cpp`, `adapters/src/*.cpp`. Out of scope: generated tables, `vst3/`, `rack/` (shells; covered by T-851 to T-856, pluginval and package checks).
  - Floor: the project-score floor is the key `"project_floor"` in `mutation-baseline.json`, read by `mutation-gate`. It is 60 for the first full run (chosen before any run). After sub-step 3 it is set once to the whole number five points below the score of the last full run, or kept at 60 if that is higher, and from then on that value is "the floor"; it is never lowered without a D- entry.
  - Test list: the whole of `muff_core_tests` is too slow per mutant, so each shard runs only the tests that exercise its file: `lowpass.cpp` -> `"[lowpass]"`; `ringmod.cpp` -> `"[ringmod]"`; `phaser.cpp` -> `"[phaser]"`; `delay.cpp` -> `"[delay]"`; `chorus.cpp` -> `"[chorus]"`; `filterbank.cpp` -> `"[filterbank]"`; `oscbox.cpp` -> `"[oscbox]"`; `tables.cpp`, `blocks.cpp`, `effect_base.cpp`, `create.cpp` -> `"[tables],[params],[blocks],[lifecycle],[cv],[bypass],[smoothing],[meters]"`; `adapters.cpp` -> `"[adapters]"`. A shard runs every test that carries one of its tags, whichever file the test is in (so `"[filterbank]"` includes T-023 and T-024, and `[cv]` and `[bypass]` include some effect tests). Omitted: for the effect shards, every test without that effect's tag (the common tests T-004 to T-018); for the shared-code shard, every test with none of the listed tags (among them T-007, T-009, T-011, T-016, T-017 and T-018). Mutants that only an omitted test would kill count as survivors, so the score is conservative (this omission is decision D-060 and is restated in `reviews/MUTATION.md`, together with the exact list of omitted tests per shard printed by `muff_core_tests --list-tests "<filter>"`).
  - Estimates (not measured: Mull is not installable in the planning sandbox, so these are estimates to be replaced by sub-step 1): mutant count 600 to 1200 over 12 files; per-shard test time 1 to 4 s in a Release build, assumed 5 to 20 s instrumented and unoptimised; per-shard job time `mutants x time / 4 workers` = about 10 to 60 min; instrumented build about 5 min per shard. Runtime budget: 90 min per shard job, 6 h for the full run across shards.
  - Shards: job `mutate` (T-905 requires the three job ids `scope`, `mutate` and `mutation-gate`), a generated matrix with one entry per in-scope file (12), `fail-fast: false`, `ubuntu-24.04` (4 vCPU, 16 GB), `--workers 4`, `timeout-minutes: 120`, per-mutant timeout 10x the shard's measured baseline (minimum 30 s), warm-up timeout 120 s.
  - Triggers (three tiers, one required check), all in `.github/workflows/mutation.yml`, never path-filtered: (a) `pull_request` and `push`: job `scope` computes `incremental` (only changed in-scope files), `full` (when anything under `tests/`, `mull.yml`, `tools/mull_*`, or `mutation.yml` itself changed) or `not-applicable`; (b) `schedule` weekly (Monday 03:17 UTC) full run on the default branch (live after the merge of S-FINAL's pull request), whose failure opens an issue titled `Scheduled mutation run failed` with no assignee (`qualitycoding` may be an organisation; repository watchers are notified) through a job that alone has the job-level `permissions: issues: write` (the top-level block stays `contents: read`, as T-905 requires of every workflow); (c) on demand: `workflow_dispatch` with a `mode` input (live after that merge) and, on any branch, a push whose head commit subject starts with `[mutation full]`; (d) job `mutation-gate` with `if: always()`: the only status meant to be required; it fails unless every expected shard report is present and the score computed **from the SQLite reports** by `tools/mull_score.py` meets the project floor (full) or the per-file baseline `mutation-baseline.json` with tolerance 2 points (incremental), with `--min-mutants 5` per shard; `not-applicable` is a success that prints `mode: not-applicable`.
- Actions:
  1. Sub-step 1 (measure): add the workflow; the `scope` job selects `full` when the head commit's subject starts with `[mutation full]`; push such a commit and let the full mode run once; record in `reviews/MUTATION.md` the installed Mull version, per-shard mutant counts, baseline test times, job times and scores. If any shard exceeds 90 min, split that file's test list further or apply the `mutation-sharding` skill's per-function split; do not widen the job.
  2. Sub-step 2 (show it can fail): run `tools/mull_score.py` on one real report with `--floor 99` and paste the non-zero exit and message into `reviews/MUTATION.md`; verify that the script's surviving count for one shard equals Mull's own "Surviving mutants" line.
  3. Sub-step 3 (survivors): files in order of rising score. For each file, every surviving mutant is either killed by a new test (`HANDOFF.md` section 9.4; one agent may add all the new tests for one source file in one `REFREEZE:` commit, each shown red by applying its mutant by hand) or covered by a written justification grouped by mutant class in `reviews/MUTATION.md` (equivalent mutant; only an omitted test kills it, naming the test; behaviour no decision fixes). Stopping rule: stop when every file scoring below 60 % has had all its survivors dealt with in one of those two ways and the project score is at or above 60; survivors in files at or above 60 % are listed by count only. At most three full runs.
  4. Sub-step 4 (wire): commit `mutation.yml`, `mull.yml`, `tools/mull_score.py`, `tools/mull_include_paths.py`, `tools/mull_shards.py` (generates the matrix), `mutation-baseline.json` (from the full run); in `ci.yml` change `repo-checks` to run the whole of `tests/py` (`python3 -m unittest discover -s tests/py -v`, replacing the command with the `-k` options of S-035). Ask nothing of the owner: enforcing a required check is a repository setting; write in `reviews/MUTATION.md` that `mutation-gate` and `aggregate` should be marked required by the owner, as a recommendation.
- Outputs: `.github/workflows/mutation.yml`, `mull.yml`, `tools/mull_*.py`, `mutation-baseline.json`, `reviews/MUTATION.md`, `deps.lock.json` (Mull version).
- Evidence produced: T-905 (workflow test) turns green
- Done when: the last full run is green in `mutation-gate` with the project score printed and at or above the `project_floor` then in `mutation-baseline.json`; an incremental run mutates only the changed file: push a commit that adds one comment line to `core/src/phaser.cpp`, confirm the log shows one shard (`phaser.cpp`), then push `git revert` of it (which runs the same shard again); the next log commit (it touches no in-scope file) reports `mode: not-applicable`; `python3 -m unittest discover -s tests/py -v` reports `OK` (9 tests); `reviews/MUTATION.md` holds the measured numbers, the failing-gate demonstration and the survivor table.
- Checkpoint: after each sub-step: `active_step_id: "S-MUTATE.<n>"`, run URLs, scores. Resume: re-run only the failed jobs of the run (`gh run rerun <run id> --failed` or the equivalent; shard reports are separate artefacts).
- On failure: no release asset for Ubuntu 24.04 with LLVM 19 exists or it does not install: R-010's response (the asset of the newest release for whichever LLVM major it supports on Ubuntu 24.04, with `clang-<that major>` and `libclang-rt-<that major>-dev` from the Ubuntu archive; if the archive lacks that clang, `BLOCKED.md`). First real score below 60 %: do not lower the floor; work survivors (sub-step 3) until the floor is met, at most three full runs, then `BLOCKED.md` with the per-file table. A shard times out at 120 min: sharding skill, then `BLOCKED.md` if still over.
- Gate: none
- Relevant decisions/claims: D-060; C-PLANNER-012
- Lessons applied: `L-20261029T010000Z-mull-reruns-whole-suite-per-mutant`, `L-20261029T020000Z-start-mutation-review-at-lowest-file`, `L-20261029T000000Z-green-ci-gate-never-ran` (per-shard test lists; survivors by file, lowest score first; score from the report and a gate shown to fail)
- Exclusive resources: `.github/workflows/mutation.yml`

### S-040 Binary packages for hands-on trial (publication gate)
- Tier: Sonnet
- Profile: software
- Depends on: S-MUTATE
- Inputs: D-041, `plan/GATES.md` G-002.
- Actions:
  1. Create `.github/workflows/packages.yml`: `on: push`, every job guarded by `if: startsWith(github.event.head_commit.message, '[build-packages G-002-approved]')`; jobs for the four VST3 platforms (`linux-x64`, `windows-x64`, `macos-arm64`, `macos-x64`), each uploading one archive `mufferfluffer-vst3-<platform>.zip` that holds the seven `.vst3` bundles and `LICENSE`, and for the four Rack platforms, each uploading its one `.vcvplugin` file, all as artefacts with `retention-days: 14`; a last job downloads the eight files and uploads `SHA256SUMS` (eight lines); same pin and permission rules as `ci.yml`.
  2. Halt at G-002: write `GATE-G-002.md` (evidence and question in `plan/GATES.md`) and wait.
  3. On `proceed`: commit `GATE-G-002.RESPONSE.md` (the answer under `## Answer`) with the subject `[build-packages G-002-approved] owner approved G-002`, push; when the run has finished, append the run URL and the contents of `SHA256SUMS` under `## Record` in a log commit. No other commit subject may start with that marker; the commit that adds `packages.yml` therefore cannot start the workflow. On `decline`: push no such commit; record that hands-on trial needs a local build per `README.md`.
- Outputs: `.github/workflows/packages.yml`, `GATE-G-002.md`, `GATE-G-002.RESPONSE.md`.
- Evidence produced: none
- Done when: the response is recorded; on `proceed`, the run is green and its artefact list shows eight archives plus `SHA256SUMS`, each above 100 kB.
- Checkpoint: `active_step_id: "S-040"`, gate state.
- On failure: no answer by the time S-MUTATE and S-LISTEN are done (`HANDOFF.md` section 9.2): treat as `decline` for continuing and record "no answer" under `## Answer`; S-LISTEN never waits for this gate (its evidence bundle is audio made by this project and needs no approval); the packages stay unbuilt.
- Gate: G-002
- Relevant decisions/claims: D-041; C-LEGAL-036
- Lessons applied: `L-20261015T090000Z-public-repo-artifacts-are-publication`, `L-20261101T042000Z-commit-gate-records-with-the-response`
- Exclusive resources: none

### S-LISTEN Listening validation
- Tier: Opus
- Profile: software (audio)
- Depends on: S-ADVERSARIAL (it may run while S-MUTATE and S-040 are in progress, but its fixes re-trigger the incremental mutation run)
- Inputs: the skill `plan/protocol/skills/audio-blind-listening-validation/SKILL.md`; D-040; D-020 to D-026 (parameter mappings); `plan/GATES.md` G-LISTEN.
- Fixed content:
  - Reference set: **none** (D-040: no redistributable recording and no dry/wet pair exists; search log in `research/SOURCES.md`, strand `references`). The blind A/B test is replaced by demos only. No reference-derived audio exists, so the evidence branch holds only this project's own audio.
  - Frozen metrics that stand under the listening gate: all behavioural tests of the seven effects (T-101 to T-109, T-201 to T-206, T-301 to T-307, T-401 to T-409, T-501 to T-507, T-601 to T-607, T-701 to T-707: ranges, frequencies, times). The gate never relaxes them.
  - Pass condition: the owner's response is `proceed` or `proceed-with-rescope`.
  - Round limit: 3 `iterate` rounds, then the gate text carries the residual remarks and offers only `proceed-with-rescope: <text>`, `stop` or `extend-rounds: <n>` (only the owner can grant further rounds; `plan/GATES.md`).
  - Dry inputs (five, all synthesised by `tools/make_demo_inputs.py`, mono, 48 kHz, 25 s each, peak 0.5 in core units): `plucked tones` (single decaying notes, one per 0.75 s, on A2 C3 E3 A3 repeating), `plucked chords` (the same generator playing the three notes A2 E3 A3 together, one chord per 1.5 s), `saw bass` (55 Hz sawtooth bass line of eighth notes at 120 beats per minute), `drum pattern` (noise-burst pattern from a seeded generator, 120 beats per minute), `held chord` (three detuned sawtooths on A2, E3, A3 held throughout). "One plucked note" is the first 3 s of `plucked tones` followed by silence. "Silence in" is digital silence.
  - How a demo line is read: the effect is named by the second word of the file name (`lowpass`, `ring` = `ringmod`, `phase` = `phaser`, `delay`, `bank` = `filterbank`, `osc` = `oscbox`, `swarm` = `chorus`; demo 23 uses `lowpass`). Every parameter not mentioned is at its fixture default, with `active` on. Values are given in musical units (Hz, ms, a step's name, a pattern number 1 to 12); the event file needs the fixture's unit, so `tools/render_demos.py` holds the demo table in musical units and converts: Hz or ms to a normalised 0-to-1 knob by inverting the formula of the effect's decision (D-021 carrier frequency, D-022 LFO rate, D-023 and D-026 time and LFO rate), a step name to its index in the fixture's step list, pattern number `p` to the value `p - 1`. A sweep "A to B" is written as one event every 10 ms, moving exponentially for frequencies and linearly otherwise; "X then Y" switches at half of the processed time; a list "for N s each" switches every N s. The event files under `demos/` are generated by `python3 tools/render_demos.py --write-events` and committed; none is written by hand. Each WAV is stereo: left = the effect's output 0; right = the output named in the demo line where it says "stereo" (Aux Out, or the right output of effects that have one), otherwise a copy of the left.
  - Demo list (23 files, 48 kHz, 24-bit WAV, each 12 to 25 s, peak-normalised to -3 dBFS; every demo starts with 3 s of the dry input, then the processed signal):
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
    - `demo-23-drive`: plucked chords through Fluff Lowpass with cutoff 12 kHz, resonance 0, mix 1; Drive stepped 0, +9, +18, +28 dB for 5 s each (aliasing and limiter character at high drive).
- Actions:
  1. Create `tools/render/muff_render.cpp` (built as target `muff_render` in the top-level `CMakeLists.txt` when `-DMUFF_BUILD_TOOLS=ON`; uses `BlockRunner` at 48 kHz; usage `muff_render <slug> <in.f32> <out.f32> <events.txt>`; raw little-endian float32 mono in; stereo interleaved out with the effect's outputs 0 and 1, or output 0 twice when it has one audio output; an optional fifth argument `--right <output id>` selects another output for the right channel; the event file has one event per line: `<seconds> <param id> <value>`, sorted by time, applied at the first sample at or after that time), `tools/make_demo_inputs.py` (writes the five dry inputs defined above as float32 files; deterministic, standard library only), `tools/render_demos.py` (renders the 23 demos from `demos/demo-NN-<name>.txt` event files, prepends the dry section, normalises, writes 24-bit WAV with the `wave` module, writes `SHA256SUMS` and `INDEX.md` with one line per demo naming effect and settings, the peak level of the processed section before normalisation in dBFS, and the gain applied, so that level problems are visible and not hidden by normalisation; taking 0 dBFS as 1.0 in core units; the script fails if any processed section peaks below -40 dBFS, or above +12.1 dBFS, which the output limiter of D-012 makes impossible in a correct build; a peak above +9 dBFS is printed as a warning and marked "loud" in `INDEX.md`, and is expected for demos 03, 12 and 23), and the 23 generated event files under `demos/`.
  2. Add workflow `.github/workflows/evidence.yml`: runs on `push` only when the head commit subject starts with `[gate-evidence G-LISTEN]`; its own concurrency group `evidence-G-LISTEN` (no cancellation); the top-level `permissions:` block is `contents: read` like every workflow (T-905), and the one job that pushes the evidence branch carries the job-level `permissions: contents: write`; builds `muff_render`, renders the bundle, asserts 23 WAV files each between 0.5 MB and 20 MB and a non-silent RMS (above -50 dBFS), and force-pushes them with `SHA256SUMS`, `INDEX.md` and a `README.md` stating that every file is this project's own work under the repository licence to the orphan branch `evidence/G-LISTEN`.
  3. Push a marker commit `git commit --allow-empty -m "[gate-evidence G-LISTEN] round <n>"` on top of the commit to be judged; verify the branch updated and one raw URL downloads with the listed hash.
  4. Halt at G-LISTEN: write `GATE-G-LISTEN.md` with the zip URL `https://github.com/qualitycoding/mufferfluffer/archive/refs/heads/evidence/G-LISTEN.zip`, per-file raw URLs, the behavioural test summary, and the questions of `plan/GATES.md` (H-1 and H-2 are included only when `GATE-G-002.RESPONSE.md` records `proceed`; otherwise the text says that a hands-on check needs a local build per `README.md` and that the owner may answer H-1 and H-2 if they made one).
  5. On `iterate: <notes>`: follow the skill's section 7: use the owner's words to choose the analysis, make one targeted change per reported difference with a general mechanism, re-run the suite, record the round in `reviews/LISTENING.md` (remarks verbatim, measured difference, change, commit), push a new marker commit, ask the same questions. If the remark matches a pre-decided remedy (R-006 aliasing, R-007 oscillator), apply that remedy first.
- Outputs: the tools, event files and workflow named; branch `evidence/G-LISTEN`; `GATE-G-LISTEN.md`, `GATE-G-LISTEN.RESPONSE.md`; `reviews/LISTENING.md`.
- Evidence produced: SC-10
- Done when: `GATE-G-LISTEN.RESPONSE.md` records `proceed` or `proceed-with-rescope: <text>`; the `ci.yml` run of the last marker commit (an empty commit, so it tests the code the bundle was rendered from) is green in `aggregate`, and the `evidence.yml` run of the same commit is green; `reviews/LISTENING.md` has one section per round.
- Checkpoint: `active_step_id: "S-LISTEN"`, round number, marker commit, bundle hash.
- On failure: an `iterate` change breaks a frozen test: revert it; if the owner's request cannot be met without changing a frozen test, record that in the next gate text and offer `proceed-with-rescope` or a test challenge (`HANDOFF.md` section 6). A demo fails the level check: if its peak is above +12.1 dBFS the effect breaks D-012 (return to its step); if below -40 dBFS, check the demo table's conversion first, then the effect. `stop`: write `BLOCKED.md` with the owner's text and end the run at S-RETRO. No answer: continue independent steps (S-MUTATE, S-040), then wait as `HANDOFF.md` section 9.2 describes.
- Gate: G-LISTEN
- Relevant decisions/claims: D-040, D-041; C-REF-028 to C-REF-035; R-003, R-004, R-005, R-006, R-012
- Lessons applied: `L-20261008T173530Z-gain-staging-masked-by-normalising-renderers`, `L-20261009T120001Z-wav-writer-clipped-baseline-before-analysis`, `L-20261007T151000Z-large-evidence-off-impl-branch`, `L-20261101T042000Z-commit-gate-records-with-the-response`
- Exclusive resources: branch `evidence/G-LISTEN`

### S-FINAL Final verification
- Tier: Sonnet
- Profile: software
- Depends on: S-MUTATE, S-040, S-LISTEN
- Inputs: the head of `impl-<run_id>`.
- Actions: open a pull request from `impl-<run_id>` to `main` titled `mufferfluffer 0.1.0: seven effects, VST3 and Rack` and do not merge it (merging is the owner's decision; say so in `COMPLETION.md` together with the fact that the scheduled and manually dispatched workflows become live only after the merge). On the head commit run `python3 tools/check_frozen.py`; the core, VST3 and repository test commands of `plan/ENVIRONMENT.md`; read the CI runs: `ci.yml` on the head commit, and `mutation.yml` on the newest commit that changed a file under `core/src/` or `adapters/src/` (on later commits `mutation-gate` reports `not-applicable`, which says nothing about the score); fill `plan/TRACEABILITY.md`'s "Result" column for every success criterion with the test ids and the run URL; write `COMPLETION.md` listing each SC-1 to SC-10 as met or not met with evidence, every `DEVIATIONS.md` row, every open R- entry, and what was not verified (R-012 unless the owner reported a hands-on trial).
- Outputs: `COMPLETION.md`; `plan/TRACEABILITY.md` (Result column only).
- Evidence produced: SC-1 to SC-9 (summary)
- Done when: `ctest` 80 of 80; `muff_vst3_tests` `All tests passed` (6); `python3 -m unittest discover -s tests/py` `OK` (9); `aggregate` green on the head commit; `mutation-gate` green, in `full` or `incremental` mode, on the newest commit that changed an in-scope source file; `COMPLETION.md` exists with ten SC rows.
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
- Actions: section 3.7.1 of `plan/protocol/planning-protocol.md` as written (a fresh-context agent, `HANDOFF.md` section 9.1): answer its questions 1, 1a, 2, 3, 4 and 5 (for 1a, diff the workflows and `deps.lock.json` of the implementation branch against `plan/ENVIRONMENT.md`); write `RETROSPECTIVE.md` with every finding linked to an `L-` or `K-` file; add the missing lessons, knowledge items and source-register rows. Re-run nothing; change no deliverable.
- Outputs: `RETROSPECTIVE.md`; new files in `lessons/`, `knowledge/`; `plan/SOURCE_REGISTER.md`.
- Evidence produced: none
- Done when: every finding links to an existing `L-` or `K-` file; each new lesson follows the template of Appendix A and each new knowledge item the template of Appendix B of `plan/protocol/planning-protocol.md` (the files already in `lessons/` and `knowledge/` are worked examples), with tags taken from the taxonomy of Appendix C or from `knowledge/TAXONOMY-ADDITIONS.md`; the schema check of Appendix C passes.
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
- Actions: section 3.7.2 of `plan/protocol/planning-protocol.md` as written: halt at G-003 with `GATE-G-003.md` (contents in `plan/GATES.md`); on sign-off clone each store, copy `lessons/` into the lesson store and `knowledge/` (including the source register as a knowledge item) into the knowledge store in the layout of Appendix C (which is also the layout the clone shows), regenerate each store's indexes as Appendix C describes (if the store carries its own index script, named in its `README.md`, run that; otherwise write the index files by the rules of Appendix C), push; on a rejected push pull, regenerate and retry up to three times per store; on failure or no response write `git bundle` files `lessons-<run_id>.bundle` and `knowledge-<run_id>.bundle` and report their paths.
- Outputs: `GATE-G-003.md`, `GATE-G-003.RESPONSE.md`; store commits or bundles.
- Evidence produced: none
- Done when: both pushes succeeded and each store's index lists every pushed id; or the response was `do-not-push`; or the bundles were written and reported.
- Checkpoint: `active_step_id: "S-KNOW"`, store commit hashes.
- On failure: as item 3 of section 3.7.2 (pull, regenerate, retry up to three times per store, then the bundles).
- Gate: G-003
- Relevant decisions/claims: A-012
- Lessons applied: none specific
- Exclusive resources: the two store repositories
