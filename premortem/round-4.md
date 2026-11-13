# Pre-mortem, round 4: mufferfluffer plan (branch `gen-20261111T151412Z-mufferfluffer-vst-vcv`, head `15b9585`)

Reviewer: fresh-context agent (Claude Opus 5.5). I did not write the plan. Date: 2026-11-13. All builds and variants were made under `/tmp/pm4work`. I committed and pushed nothing in `/home/claude/pm4`. One side effect: running the frozen Python tests with `-s /home/claude/pm4/tests/py` rewrote a tracked `.pyc` file in the clone. I restored it with `git checkout --`, and `git status` is clean. The file's existence is itself finding PM4-03.

## 1. Method and what was executed

**Read.**
- The planning documents: `HANDOFF.md`, `plan/DECISIONS.md` (all of it), `plan/PLAN.md` (every step), `plan/GATES.md`, `plan/RISKS.md`, `plan/ENVIRONMENT.md`, `plan/ASSUMPTIONS.md`, the relevant rows of `plan/TRACEABILITY.md`, and `research/spikes/red-verification/RESULT.md`.
- The pre-mortem records: the disposition tables of rounds 1 to 3, and the whole of round 3.
- The frozen tests: every post-freeze test in full (T-110, T-304, T-308, T-410, T-411, T-508, T-509, T-708, T-857, `tests/py/test_repo.py`), plus the neighbours they rely on: T-017, T-018, T-301 to T-307, T-401 to T-409, T-501 to T-507, T-701 to T-707, T-852 and T-853.
- The code: `test_support.hpp`, `tests/CMakeLists.txt`, the top-level `CMakeLists.txt`, the oracle core, and the tail handling of the VST3 oracle.
- The claims behind D-027 items 12 to 16.

**Executed.**

| # | What was run | Result |
|---|---|---|
| Z1 | Oracle with the frozen tests, Release, GCC 13.3 | `All tests passed (299647 assertions in 87 test cases)` |
| Z2 | `python3 tools/check_frozen.py` | `frozen files intact: 21 files`. The manifest lists 13 test files, the four headers and four tools, as `HANDOFF.md` §4 says |
| Z3 | Built a "knobs" variant of the oracle in which environment variables change permitted details without rebuilding. The details: LFO start phase (`MUFF_PH0`), a `float` phase accumulator (`MUFF_FLOATPH`), returning the value before advancing the phase (`MUFF_POST`), a rate multiplier (`MUFF_RATEMUL`), the FilterBank LFO start phase (`MUFF_FBPH0`), Phaser, Delay and FilterBank LFO depth and depth law (`MUFF_PHDEPTH`, `MUFF_PHPOW`, `MUFF_DLDEPTH`, `MUFF_FBDEPTH`), an RBJ bilinear Butterworth biquad in place of the SVF for the Delay and Chorus wet filters (`MUFF_BIQUAD`), and three readings of the Lowpass seed noise (`MUFF_NOISEMODE`) | With nothing set: 87 of 87 |
| Z4 | LFO start phase at 21 values from 0 to 0.99, tests T-308, T-411, T-304, T-307, T-405, T-705, T-706, T-203, T-204 | T-308, T-411, T-304, T-307, T-405 and T-203 pass at every phase. T-204, T-705 and T-706 (pre-freeze) fail at non-zero phases. D-015 fixes phase 0, so that is not a permitted variation |
| Z5 | `[phaser],[delay],[chorus],[ringmod]` (33 cases) with `float` phase; with post-advance; with both; with rate × (1 ± 3e-7), (1 ± 1e-5), (1 ± 1e-3) and (1 ± 5e-3), each with and without `float` phase | 33 of 33 in all 19 combinations |
| Z6 | Phaser depth 1, 2, 2.5, 2.7, 2.85, 2.9, 3.1, 3.15, 3.3, 3.5 and 4 octaves; depth laws amount² and amount^0.5; depth 1 and amount² at start phases 0, 0.3, 0.6 and 0.9 with `float` phase | Each fails T-308 and only T-308 among the Phaser tests. Depths 2.95 and 3.05 pass, so the tolerance is about ±0.07 octave around the specified 3 |
| Z7 | FilterBank LFO start phase 0.1, 0.25, 0.5, 0.75, 0.9 (T-507, T-508); FilterBank depth 0.4, 0.45, 0.55, 0.6, 0.7 octave (T-508) | Every phase passes. Depths 0.45 and 0.55 pass; 0.4, 0.6 and 0.7 fail |
| Z8 | Delay depth × 0.9, 0.95, 1.05, 1.1 (T-411); T-410 and T-411 at start phases 0.1 to 0.9 with `float` phase | 0.95 and 1.05 pass; 0.9 and 1.1 fail; every phase passes |
| Z9 | Biquad wet filter, `[delay],[chorus]` (19 cases) | 19 of 19 |
| Z10 | Lowpass seed noise read as unipolar [0, 1e-5), as half amplitude, and as a different draw from the generator, `[lowpass]` | 10 of 10 each |
| Z11 | The knobs variant with Rack's x64 flags (`-O3 -march=nehalem -funsafe-math-optimizations -fno-omit-frame-pointer`), whole suite, with: `float` phase; `float` phase and post-advance; biquad; unipolar noise; `float` phase and rate × (1 − 3e-7) | 87 of 87 each |
| Z12 | Synthetic repository tree from `research/spikes/oracle_repo/make_tree.py`, with `python3 -I -m unittest discover` | `Ran 9 tests`, `OK` |
| Z13 | The same tree plus a root file `GATE-G-LISTEN.RESPONSE.md` whose `## Answer` names a model code and the product family, as an owner's verbatim `iterate:` note might | `FAIL: test_no_third_party_marks_in_the_product`, `['GATE-G-LISTEN.RESPONSE.md: lines [4]']` (PM4-01) |
| Z14 | `ctest --test-dir <empty dir> -C Release`, CMake 3.28.3 | `No tests were found!!!`, exit status 0 (PM4-02) |
| Z15 | `git ls-files tests/py`; the `.pyc` header; `git status` after running the frozen Python suite in the clone | `tests/py/__pycache__/test_repo.cpython-313.pyc` is tracked. It is timestamp-validated (flags 0), so stale bytecode is never run. Running the suite marks it modified (PM4-03) |
| Z16 | `git ls-remote` of the nine pins (peeled for annotated tags); the newest Mull tags | All nine match. The newest Mull tag is still 0.38.0 |

**Not executed.**
- JUCE builds: T-857 was judged by reading it against the contract and `BlockRunner`.
- ThreadSanitizer.
- Rack SDK archives and Mull assets (proxy).

## 2. Verification of round 3 fixes

| Item | Disposition | Check made | Holds? |
|---|---|---|---|
| PM3-01 T-308 | Signed maximum, re-frozen | Read the amended test. The search window `[240, n − 16384)` is 6.65 s, longer than the 6.32 s LFO period, so a whole +1 apex always lies inside it. `top > n/2` chooses the bottom search window correctly for every apex position, including a first apex at 3.49 s, whose trough at 6.65 s is still inside the 6.659 s limit. Z4 covers 21 start phases, Z5 `float` phase, post-advance and rate errors up to ±0.5 %, and Z11 Rack flags: all pass. Z6: every wrong depth or depth law fails, and only ±1.7 % of depth passes. | **Yes**: satisfiable for any start phase or phase precision, and it still fails a wrong depth. |
| PM3-02 D-016 thread rule | Atomic refreshed only in `prepareToPlay` and `processBlock` | D-016 now says these are "the only two places that touch the effect", and that `setStateInformation` changes only the JUCE parameter values. T-852 reads the tail after processing, so it still passes. T-853 is unaffected. | **Yes** (a stale value before the first block: PM4-04, Low) |
| PM3-03 S-FINAL, S-MUTATE triggers | Done-when names the mutation commit; no `pull_request` trigger; base stated | S-FINAL's Done-when now matches its Actions: `full` or `incremental` on the newest commit `scope` did not skip, and any mode on the head commit. `scope` diffs against `github.event.before` and selects `full` when that is all zeros. The empty `final:` commit therefore reports `not-applicable`, which counts as success. Status checks are matched by commit, so the push-run on the PR's head SHA also serves a required check on the PR. | **Yes** (a small cost item: PM4-07) |
| PM3-04 tap retarget | D-027 item 15 | The text matches C-PEDB-017 ("holding ... about one second retargets"). | Yes |
| PM3-05 D-024 rise law | "same slope" | The oracle uses a fixed slope `1/(attack·period)`. T-504 retriggers only after a finished envelope (pattern 12 fires every 4 steps), so every rise starts at 0 and the law does not matter there. T-509 steady state: a triangle between 0.5 and 1, which matches its comment "0.5 expected", and both readings pass it. No test contradicts the stated law. | Yes |
| PM3-06 interpretations | D-027 item 16 | All three readings listed. C-PEDA-041 ("full sweep range", six-octave sweep) is consistent with ±3 octaves at Amount 1. | Yes |
| PM3-07 V/oct, RingMod Mix | One sentence each | D-017: V/oct is not smoothed. D-021: Mix is linear. The oracle does both. | Yes |
| PM3-08 Mull asset | Name rule | The rule excludes `Mull-Extras-`. 0.38.0 is still the newest tag (Z16). | Yes |
| PM3-09 knowledge item | Checked, nothing to supersede | Accepted with the stated response. Not re-verified against the stores (no need). | Accepted |

**D-027 items 12 to 16.**
- Item 12: the centre of a 15 Hz to 12 kHz log taper is √(15 × 12000) = 424 Hz, against roughly 500 Hz in C-PEDA-001. Correct.
- Item 13: Time at noon plus a depth-1 CV reaches the end at 2.5 V. Correct.
- Item 14: D-022 uses 6 × `sweep_in`, the same scale as a depth-1 Sweep CV over six octaves. Correct.
- Items 15 and 16: correct.
- None of the five lines contains a mark that T-901 forbids, so the list can go into `GATE-G-LISTEN.md`.

## 3. Satisfiability review of the post-freeze tests

| Test | What was varied (all permitted by the decisions) | Result | Still rejects the defect it targets? |
|---|---|---|---|
| T-110 | Seed-noise sign, amplitude reading and draw (Z10); Rack flags (Z11) | Pass | Yes: the separate two-stage loop of round 2 cannot self-oscillate (by reasoning, as round 2 found) |
| T-304 | Start phase at 21 values (Z4); `float` phase, post-advance, rate errors (Z5); Rack flags | Pass | Round 2 showed +3 % rate fails. Not re-run |
| T-308 | As section 2: start phase, `float` phase, post-advance, rate ±3e-7 to ±5e-3, Rack flags | Pass | Yes: 11 wrong depths and 2 wrong laws fail (Z6) |
| T-410 | Biquad wet filter (Z9); LFO phase and precision (Z8); Rack flags | Pass | RESULT.md: the unfiltered-loop variant fails |
| T-411 | Start phase (Z4, Z8); `float` phase; rate errors; biquad; Rack flags. By reasoning: eight bursts 0.25 s apart cover the 2 s square cycle, and each half leaves room for at least two settled transits | Pass | Yes: depth ×0.9 and ×1.1 fail (Z8) |
| T-508 | FilterBank LFO start phase at five values (Z7) | Pass | Yes: 0.4 and 0.6 octave fail (Z7) |
| T-509 | The two rise laws (round 3 Y14; D-024 now fixes one); D-024's trigger and envelope order within a sample (by reasoning, the extremes 0.5 and 1 are unchanged) | Pass | RESULT.md: restart-from-zero fails |
| T-708 | Biquad wet filter with tracking cut-off (Z9); Rack flags | Pass | RESULT.md |
| T-857 | Read only. The contract maps mono→mono to `BlockRunner` with one input and one output, and the test compares against exactly that. A shell honouring D-016 cannot differ beyond the 1e-6 allowance. Parameters are untouched, so defaults on both sides | Satisfiable by construction | RESULT.md: the two-output shell fails |
| `aggregate` (T-905) | Synthetic tree (Z12). The regex accepts `    if: always()` at job level, with or without trailing spaces. CRLF files read through `read_text()` are normalised | Pass | A step-level `if:` fails (round 3) |

**Verdict.** I found no post-freeze test that a conforming implementation can fail. The tolerances are about ±2 % (T-308), ±5 % (T-411) and ±10 % (T-508) of the specified depth. The oracle sits near the centre of each.

## 4. Consistency review

**Counts agree.**
- The documents say 87 core cases, 86 under the sanitizer build, 7 VST3 and 9 Python. These figures appear in `HANDOFF.md` §4; PLAN line 3, S-000, S-010 (8), S-011 to S-017 (10, 6, 8, 11, 8, 11, 7), S-018, S-020, S-REVIEW, S-MUTATE and S-FINAL; and `RESULT.md`.
- Counted from the `TEST_CASE` lines: 18 common, 3 block tests outside `[filterbank]`, 61 effect-tagged and 5 adaptor cases make 87.
- T-011 is the one case excluded under `MUFF_NO_ALLOC_HOOK`.

**File names agree.**
- There are ten mutation shards (7 effects, `clocked_line.cpp`, `shared`, `adapters.cpp`).
- `mutation-floor.json` is the same in D-060 and S-MUTATE.
- `clocked_line.cpp` is the same in S-014, S-015 and S-MUTATE.
- `tools/check_pins.py` is defined in S-001 3a and run by `repo-checks`. However, D-009's `repo-checks` row does not list it (PM4-05).

**Decision numbering.** D-071 is the highest D- number, as HANDOFF §6 says. D-070 sits after D-071 in the file, which is harmless.

**Gates.**
- G-002's "no response" branch in `GATES.md` agrees with S-040 and HANDOFF §9.2.
- The G-LISTEN questions invite the owner to compare with the hardware ("as far as you know the hardware"; L-5 "hardware patterns"). That collides with the verbatim rule for answers (PM4-01).

**D-063** still says each test added after round 2 failed "against no other variant", and it does not record the round 3 amendment of T-308. Only `RESULT.md` does (PM4-05).

**TRACEABILITY.** The T-509 row cites D-024 only, not D-027 item 16 (PM4-05).

**Comment drift (cosmetic, frozen).** The header comment of `tests/vst3/test_vst3.cpp` still says "T-851 .. T-856".

**No contradiction** found between RISKS R-001 to R-019 and the amended steps.

## 5. Lens-by-lens notes

**(a) Technical correctness.**
- Post-freeze tests: section 3.
- The tail formulas S-011 to S-015 prescribe are not the oracle's. I checked them against T-017:
  - Lowpass at resonance 7.9 gives 35.2 s, below 1e8.
  - Lowpass at 8 gives 1e9.
  - Delay and Chorus match the oracle.
- The Lowpass formula is the correct 60 dB time of the ladder's slowest pole. The closed-loop pole has real part ωc((r/8)^¼ − 1), which is zero at resonance 8.
- The pre-freeze tests T-204, T-705 and T-706 depend on the LFO start phase that D-015 fixes. That is legitimate, but it means D-015's "phase 0" is load-bearing.

**(b) Supply chain.**
- All nine pins hold (Z16).
- Mull 0.38.0 is still the newest release.
- The Rack SDK archives remain trust on first use (R-018).

**(c) Research assumptions.**
- D-027 items 12 to 16 are faithful to their claims.
- Nothing new beyond the accepted R-001, R-003 and R-004.

**(d) Implementer misinterpretation.**
- PM4-01: the verbatim gate answer against T-901.
- PM4-06: which thread is JUCE's message thread in the TSan stress.

**(e) Integrity.**
- PM4-02: a CI job can be green with zero tests run.
- PM4-05: D-063's claim is out of date.
- The tracked `.pyc` (PM4-03) is not stale bytecode, so it cannot make a test run old code.

**(f) Past lessons.**
- PM4-02 is the class of `L-20261029T000000Z-green-ci-gate-never-ran`. The plan applies that lesson to S-001 (`test cases:` must appear in each log) and to the sanitizer job (86 cases), but not to the seven `ctest` jobs after S-018.

**(g) Performance.** No new item. PM4-07 is runner time only.

**(h) Security.**
- No new exposure.
- `evidence.yml` alone holds `contents: write`, and it runs only on a collaborator's marker push.
- Pushes made with the `GITHUB_TOKEN` start no workflows.

**(i) Operations.**
- PM4-07.
- R-017 and R-018 unchanged.

**(j) CI platform coverage.**
- PM4-02 concerns exactly the platforms that run only in CI: Windows MSVC, both macOS architectures, musl, GCC 11 and arm64 Linux. On these, a wrong `--test-dir` would pass silently.
- Catch2's multi-configuration guard (a `*_NOT_BUILT` test when the configuration file is missing) covers the Windows `-C` case, but not a wrong directory.

## 6. Risk table

| ID | Description | Lens | Severity | Likelihood | Root cause | Trace | Evidence checked | Proposed mitigation |
|---|---|---|---|---|---|---|---|---|
| PM4-01 | The owner's gate answers are copied verbatim into `GATE-<id>.RESPONSE.md` at the repository root (HANDOFF §9.2, GATES, PLAN rule). T-901 scans that file and fails on the product-family name or a model code. The G-LISTEN questions ask the owner to compare with the hardware, so an `iterate:` note naming a pedal is likely. The owner may also commit the file themselves. From round 2 on, the marker commit carries that file, so S-LISTEN's Done-when ("`aggregate` green on the last marker commit") and S-FINAL's Python-suite check fail. Two plan rules then conflict (verbatim in the root file; hardware names only under `reviews/`). The failure is loud (repo-checks red) and §9.3's last sentence points to a resolution, so this is Medium, not High. | (d), (e) | Medium | high | §9.2's verbatim rule was written without the T-901 scan of root records in view. | HANDOFF §9.2, §9.3; GATES G-LISTEN; S-LISTEN Done-when; T-901 | Z13 | In §9.2: "if an answer names the hardware, put the verbatim text in `reviews/gate-answers/GATE-<id>-<round>.md` and write in `## Answer` the response keyword, the project's names for the effects and a link". Say the same for a response file the owner commits. Add one line to the gate text asking the owner to name effects by this project's names. |
| PM4-02 | `ctest` exits 0 when it finds no tests (Z14). From S-018 the seven non-sanitizer core jobs are judged on exit status alone. The local Done-when counts 87 for Linux only. A wrong `--test-dir` in one job would make Windows, macOS, musl, GCC 11 or arm64 green without a test running: a Rule 14 false green on exactly the platforms nobody runs locally. Rated Medium, not Critical, because the plan gives the exact path and nothing points to a slip. The gap is a missing guard, not an observed false green. | (e), (f), (j) | Medium | low | The S-001 check (`test cases:` in each log) was not carried over when the jobs switched to `ctest` in S-018. | S-018 action 2; D-009; ENVIRONMENT "Commands" | Z14; S-018 text | Use `ctest ... --no-tests=error` (CMake 3.17 and later, so also 3.22). Have each core job also fail unless its log contains `out of 87`. |
| PM4-03 | `tests/py/__pycache__/test_repo.cpython-313.pyc` is tracked (added in the planner's freeze commits). Running the frozen Python suite with Python 3.13 rewrites it, leaving a modified tracked file under `tests/` right after S-000. S-001's `.gitignore` cannot untrack it. An implementer then sees an edit under `tests/` that `check_frozen.py` does not flag, and may commit it or halt over it. It is not stale bytecode (timestamp-validated), so results are unaffected. | (e), (d) | Low | medium (Python 3.13 hosts) | `git add -A` during planning, before any `.gitignore` existed. | `tests/py/`; HANDOFF §4 | Z15 | `git rm --cached tests/py/__pycache__/test_repo.cpython-313.pyc` before hand-over, and add `__pycache__/` to a `.gitignore` now. |
| PM4-04 | D-016 sets the tail atomic at the end of `prepareToPlay` from the effect's current parameters. Those are the last values a `processBlock` copied, not those `setStateInformation` restored. A host that restores a project and queries the tail on activation gets a stale tail (for example, a finite tail for a self-oscillating Delay) until the first block. A bounce may then cut the tail short. | (a) | Low | low | Parameters reach the effect only in `processBlock`. | D-016; `vst3_shell.hpp` | D-016 text; oracle VST3 shell | At the start of `prepareToPlay`, copy every parameter's current value into the effect (no audio is running then), then set the atomic. |
| PM4-05 | Documentation drift after three rounds: (1) D-063 does not record the round 3 amendment of T-308, and keeps "against no other variant", which round 3 disproved for the earlier T-308; (2) D-009's `repo-checks` row omits `tools/check_pins.py`, which S-001 3a has it run; (3) the TRACEABILITY row of T-509 does not cite D-027 item 16. | (e) | Low | high | Fixes applied to step text, not to the summary records. | D-063, D-009, TRACEABILITY | Section 4 | Three one-line edits. |
| PM4-06 | S-ADVERSARIAL's stress has "a third" thread create, run and delete the editor, without saying that this must be the thread that set up JUCE's MessageManager. An editor created off it may give ThreadSanitizer reports with frames in `vst3/src/editor.cpp`. By the step's rule these count as findings, though they come from the test program, not the product. | (d) | Low | low | Thread roles unstated. | S-ADVERSARIAL | Step text | State that the main thread initialises JUCE (`ScopedJuceInitialiser_GUI`) and runs the message loop that creates, pumps and deletes the editor, while the other two threads are plain threads. |
| PM4-07 | S-MUTATE sub-step 1 adds `mutation.yml`. Because `mutation.yml` changed, `scope` already selects `full` on that push, and the following `[mutation full]` marker starts a second full run of up to 6 h. | (g), (i) | Low | high | The marker rule predates the "file changed, so full" rule. | S-MUTATE triggers, sub-step 1 | Step text | Push the workflow with the marker in the same (single) commit, or treat the run of the adding commit as the measuring run. |

## 7. Counts by severity

| Severity | Count | IDs |
|---|---|---|
| Critical | 0 | none |
| High | 0 | none |
| Medium | 2 | PM4-01, PM4-02 |
| Low | 5 | PM4-03, PM4-04, PM4-05, PM4-06, PM4-07 |

**Round 3 fixes.** All hold.
- T-308 now passes every permitted variant I built: 21 start phases, single-precision phase, post-advance, rate errors up to ±0.5 %, and Rack flags.
- It still rejects every wrong depth outside about ±0.07 octave.

**Calibration.** Nothing found in this round makes a delivered effect wrong or invalidates a frozen test. PM4-01 is the only item likely to occur. It fails loudly and has a documented direction, so it is Medium. PM4-02 would be Critical if it happened, but it needs an implementer slip that the given commands avoid, so it is Medium as a missing guard. The review has converged.

CRITICAL: 0 HIGH: 0 MEDIUM: 2 LOW: 5

---

## Planner's disposition (added after the round; the text above is the reviewer's report, unedited)

| Item | Severity | Disposition | Where |
|---|---|---|---|
| PM4-01 | Medium | Fixed: an owner's answer that names the hardware is kept verbatim under `reviews/`, with a restatement in the response file. | `HANDOFF.md` 9.2, `plan/GATES.md` |
| PM4-02 | Medium | Fixed: `--no-tests=error` and a check of the test total. | S-018 |
| PM4-03 | Low | Fixed: the bytecode file is no longer tracked; `.gitignore` added. | repository |
| PM4-04 | Low | Fixed: the tail value is initialised at construction. | D-016 |
| PM4-05 | Low | Fixed: three records updated. | D-063, D-009, `plan/TRACEABILITY.md` |
| PM4-06 | Low | Fixed: thread roles stated. | S-ADVERSARIAL |
| PM4-07 | Low | Fixed: the workflow is added in the marker commit. | S-MUTATE |

Result of round 4: 0 Critical, 0 High. The pre-mortem loop ends here (protocol Phase 4, item 5). Totals over four rounds: 0 Critical; 6 High, all fixed (2, 3, 1, 0); 20 Medium, of which 3 were accepted as risks R-016 and R-017 or recorded in D-027 and the rest fixed; 24 Low.
