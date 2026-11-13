# Pre-mortem, round 2: mufferfluffer (seven effects, VST3 and VCV Rack 2)

Reviewer: a fresh-context agent (Opus tier). It did not write the plan and did not take part in round 1. Branch reviewed: clone at `/home/claude/pm2`, head `432a8de` ("REFREEZE after pre-mortem round 1 ..."). Protocol: `plan/protocol/planning-protocol.md`, Phase 4 items 1 to 3, with severity set by Global Rule 4 (the `[All]` and `[software]` lines) applied strictly. Dates: 2026-11-12 and 2026-11-13. Nothing in the clone was edited. Every experiment ran in copies under `/tmp/pm2work`.

## 1. Method and what was executed

**Read in full:**
- `HANDOFF.md`.
- `plan/ASSUMPTIONS.md`, `DECISIONS.md` (D-001 to D-071), `ENVIRONMENT.md`, `PLAN.md` (every step), `GATES.md`, `RISKS.md` (R-001 to R-019), and the parts of `TRACEABILITY.md` that cover the SC map and the new tests.
- The four frozen headers.
- Every frozen test: `tests/core/*.cpp`, `test_support.hpp`, `tests/vst3/test_vst3.cpp`, `tests/py/test_repo.py`, and both test `CMakeLists.txt`.
- The fixture (summarised by script).
- `research/spikes/red-verification/RESULT.md`.
- The oracle sections named below, and `research/spikes/oracle_vst3/processor.cpp`.
- `premortem/round-1.md`, including the dispositions.
- The protocol's Phase 4 and 4.7.
- The two mutation skills: `per-file-matrix.yml` and `mull_include_paths.py`.
- The checklist and severity column of `research/PRIOR_KNOWLEDGE.md`.
- Every load-bearing `single-source` or `unverified` claim cited by a decision or step (49 claims, listed by script). Of these, C-PEDA-041, C-PEDA-044 to C-PEDA-046, C-PEDA-011, C-PEDB-020 and C-PLANNER-009 were read closely.

**Executed** (all outputs are in `/tmp/pm2work`):

| # | What | Result |
|---|---|---|
| X1 | Oracle with the frozen core tests, GCC 13.3, Release | `All tests passed (299451 assertions in 83 test cases)`, 17.7 s |
| X2 | Same with Rack's flags (`-O3 -march=nehalem -funsafe-math-optimizations -fno-omit-frame-pointer`) | 83/83 pass |
| X3 | Same with `-O2 -march=haswell -ffp-contract=fast` (FMA contraction) | 83/83 pass |
| X4 | Same with `-O3 -ffast-math`, a flag no build in the plan uses; checked against JUCE 9.0.5 `juce_recommended_config_flags`, which adds only `-O3` or `/Ox` | 80/83. T-003, T-004 and T-021 fail on NaN handling, as expected. Not a finding |
| X5 | `tools/check_frozen.py` on a copy | `frozen files intact: 21 files` |
| X6 | 20 oracle variants ("mutants", built by `scripts/mut.sh`, full suite each). Results are in section 4 and the risk table | see table below |
| X7 | Replicas of T-811 and T-821 that print the measured worst difference, against the oracle built three ways (X1 to X3) and two variants | T-811 worst 3.8e-6 V (OscBox) against 1e-5 V. T-821 worst 5.96e-7 (Phaser) against 1e-6 |
| X8 | pluginval 1.0.4 (Linux binary from round 1), D-007's corrected command, `--random-seed 0x6d756666` | Negative control on an empty `NotAPlugin.vst3`: exit 1, log `Random seed: 0x6d756666`. Real plug-in (`Fluff Delay`, round 1's build): exit 0, `SUCCESS`, 25 `Starting tests in:` lines, `Random seed: 0x6d756666` |
| X9 | `clang -target arm64-apple-macos11 -march=nehalem -c` (clang 18) | `error: unsupported argument 'nehalem' to option '-march='`, exit 1. With `-march=armv8-a+fp+simd`: exit 0 |
| X10 | `git ls-remote` of every pinned tag: JUCE 9.0.5, Catch2 v3.16.1, Rack v2.6.6, pluginval v1.0.4, the five actions | All match except `msys2/setup-msys2` v2.33.0, which is `ec48f7c5447b…` (commit dated 2026-09-27), not the pinned `2ff06a04f6ed…` |
| X11 | Fetch of `2ff06a04f6ed…` from `msys2/setup-msys2`, and a search of all 702 commits on its branches and tags | `upload-pack: not our ref`. The object is in no branch or tag history |
| X12 | Float round-trip `(5x)/5` and `(5x)·0.2f` over 10⁷ random values | 11.1 % and 23.5 % of values change by one ulp (input to the X7 analysis) |
| X13 | GitHub documentation on skipping workflow runs ([docs](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/skip-workflow-runs)) | For `pull_request`, only the PR's HEAD commit message is checked. A skipped workflow leaves required checks "Pending" and blocks the merge |
| X14 | JUCE 9.0.5 `juce_TargetPlatform.h` and README | A deployment target of 11.0 is accepted (the floor is 10.11), so D-071 holds |

**Variants run in X6** (full frozen core suite each, 83 cases):

| Variant | Decision | Result |
|---|---|---|
| Phaser LFO sweep ±1 octave instead of ±3 (`3.0*amt*l` → `1.0*amt*l`) | D-022 | **83/83 pass** |
| Delay LFO depth ¼ of D-023 (`/10` → `/40`) | D-023 | **83/83 pass** |
| Delay LFO depth 3× of D-023 (`/10` → `/3.333`) | D-023 | **83/83 pass** |
| FilterBank retrigger restarts the envelope from 0 | D-024 | **83/83 pass** |
| FilterBank LFO shift ±0.2 octave instead of ±0.5 | D-024 | **83/83 pass** |
| Trigger inputs with no hysteresis (re-arm below 0.2 instead of 0.02) | D-012 | **83/83 pass** |
| OscBox Env Amount without the `min(env, 1)` cap | D-025 | **83/83 pass** |
| Phaser feedback without `tanh` | D-022 | **83/83 pass** |
| OscBox phase accumulator in `float` instead of `double` (a legitimate choice) | D-035 | 83/83 pass |
| RackBridge input `volts / 5` (literal header) instead of `× 0.2f` | D-013 | 83/83 pass |
| Phaser `tanh` replaced by a Padé approximation (legitimate) | D-022 | 83/83 pass |
| Delay Long-range filter at 1.6 kHz and at 1.85 kHz (within T-404's tolerance) | D-023 | 83/83 pass each (T-410 accepts them) |
| Phaser LFO rate −0.3 % and +1.5 % (inside the 2 % tolerance) | D-022 | 83/83 pass each (amended T-304 accepts them) |
| Phaser LFO rate +3 % (outside the tolerance) | D-022 | 82/83: T-304 fails, as it should |
| OscBox sync arming changed | D-025 | 82/83 (T-607 catches it) |
| Delay tap hand-back ignores CV | D-014 | 82/83 (T-811 fails by coincidence: the changed tap state amplified the ulp differences of X12 to 1.38e-5 V. See PM2-13) |

**Not executed:**
- Windows and macOS builds: no such hosts.
- A JUCE build of the VST3 test binary with a deliberately wrong shell. Not needed: PM2-07 follows from the absence of any mono-to-mono processing in T-851 to T-856, which reading confirms.
- The Rack SDK archives: `vcvrack.com` is unreachable.
- Mull: not installed.
- The GitHub REST API (403 from the proxy), so I could not ask GitHub whether it can still resolve the dangling msys2 SHA for Actions.

## 2. Verification of round 1 fixes

| Item | Disposition | Checked | Holds? | Notes |
|---|---|---|---|---|
| PM1-01 | Renderer through `BlockRunner`, mono = one output channel, self-check | S-LISTEN "Fixed content" and action 1; `adapters.hpp` numOut rule; T-802 pins the mono sum | **Yes** | The demo-14 self-check (mono ≠ stereo-left by ≥10 % RMS) can fail, and does fail for the old renderer. |
| PM1-02 | `--random-seed`; pass rule checks the seed line | D-007, S-021 action 4; X8 | **Yes** | The seed is honoured in hex, and the pass strings appear in a real passing log. |
| PM1-03 | T-304 counts cycles for 250 Hz | `test_m1.cpp` l.333-356; X6 rate variants | **Yes** | −0.3 % and +1.5 % pass, +3 % fails. The Lo-range and 1 Hz cases keep the slope measure at ≥3200 samples per cycle, where the peak-sampling loss is negligible. The amplitude bound `≤ 0.56·1.001` is unchanged from the original freeze. |
| PM1-04 | T-410, T-708 | `test_m2.cpp`; RESULT.md mutant; X6 cutoff variants | **Yes** | They catch the unfiltered loop, and cutoffs inside T-404's tolerance still pass. Fixed 48 kHz and 96 kHz rates; robust under X2 and X3. |
| PM1-05 | T-110 | `test_m1.cpp` T-110 | **Yes** | A separate two-stage loop cannot self-oscillate (phase only reaches 180° at infinity), so it fails. Correct designs pass under X1 to X3. |
| PM1-06 | D-071 floor, `minos` check, install notes | D-071, S-021 action 5, S-035, S-040; X14 | **Yes** | The `minos` check runs only in `packages.yml` (after G-002), but S-021 sets the target in every VST3 job. |
| PM1-07 | Accepted, R-016 | R-016 | Accepted | Response adequate. |
| PM1-08 | Meters through atomics; stress opens the editor | D-016 "Threads", S-021 action 1, S-ADVERSARIAL | **Partly** | The meter path is fixed. The same thread rule now conflicts with the frozen contract for `getTailLengthSeconds()` (PM2-08). |
| PM1-09 | `[skip ci]` log commits | PLAN rule l.7, S-MUTATE Done-when | **Partly** | The queueing is fixed. It creates a new problem: the branch head handed to the owner, and every PR head, is a `[skip ci]` commit (PM2-05). |
| PM1-10 | Score rule on killed statuses only, cross-check against Mull | S-MUTATE "Score rule" | **Yes** | The status numbers are still to be confirmed in sub-step 1, as written. |
| PM1-11 | Accepted, R-017 | R-017 | Accepted | |
| PM1-12 | `.gitignore` | S-001 action 1 | **Yes** | `rack/core/` and `rack/adapters/` cover the paths round 1 observed. |
| PM1-13 | `mutation-floor.json` | S-MUTATE "Floor" | **Yes** | |
| PM1-14, PM1-15 | Accepted, R-019 | R-019 | Accepted | |
| PM1-16 | Two rows rewritten | PRIOR_KNOWLEDGE rows | **Yes** | |
| PM1-17 | Accepted, R-018 | R-018 | Accepted | Pins of actions are not covered by R-018, and one is wrong (PM2-01). |
| PM1-18 | Dedupe, owner note | S-MUTATE triggers (b), S-FINAL | **Yes** | |
| PM1-19 | `rack-macos` runs the core tests with Rack's flags | S-033, D-006 | **No** | `rack-macos` is arm64 (`macos-15`), and Apple Clang on arm64 rejects `-march=nehalem` (X9). The job fails at CMake configure, before any test runs (PM2-04). |
| PM1-20 | D-027 item 12 | D-027 | **Yes** | |

**Counts:**
- 83 core cases = T-001 to T-018 (18), T-020 to T-024 (5), M1 (10+6+7), M2 (10+8), M3 (7+7), adaptors (5).
- 82 in the sanitizer build (T-011 is compiled out).
- These numbers are consistent in `HANDOFF.md` §4, `PLAN.md` l.3, l.30, l.218, l.222, l.379 and l.511, and D-006.
- Per-step counts: Lowpass 10, Delay 10, Chorus 8, FilterBank 9. Ranges in S-LISTEN and TRACEABILITY are correct.
- The only stale count is in `reviews/COLD_READ.md` l.39 ("79 cases"). It is a historical record, so it is not a finding.

## 3. Incident reports (six months after hand-over; the plan was followed exactly)

**IR-1. The Windows Rack build never ran.**
- At S-033 the `rack-windows` job failed at "Set up job". `msys2/setup-msys2@2ff06a04f6ed…` does not resolve: the pinned SHA is in no branch or tag of the action's repository, and tag v2.33.0, dated six weeks before planning, is `ec48f7c5447b…` (X10, X11).
- `HANDOFF.md` §6 makes any change to a pin of `plan/ENVIRONMENT.md` a halt. The implementer therefore wrote `BLOCKED.md`, and every step after S-033 that needs a green `aggregate` (S-034, S-035, S-REVIEW onwards) stalled until the owner answered.
- C-PLANNER-009 records this SHA as `verified` by `git ls-remote`, and the R0 application of lesson `L-20261028T200000Z-pinned-tool-version-does-not-exist` says "every pin was read by `git ls-remote` on the planning day". For this pin, neither statement is true. (PM2-01)

**IR-2. Fluff Phase barely swept.**
- The implementer read D-022 as "LFO moves the corner by `lfo_amount` octaves" and wrote `fc = … 2^(… + amt × lfo)`, which is ±1 octave at full Amount instead of ±3.
- T-307, titled "Phaser LFO Amount sweeps the dips by up to +/-3 octaves", only checks that a 1.1 kHz tone is modulated by more than 20 dB, and a ±1-octave sweep does that easily (X6: 83/83 pass).
- At G-LISTEN the owner heard demos 07 and 08 as "a subtle wobble rather than the big swoosh" and spent an iterate round on it.
- C-PEDA-041 says Amount reaches "the full sweep range" (the six-octave Sweep range), so ±3 octaves is the documented identity of the effect. (PM2-02)

**IR-3. A regression in the delay line passed the mutation gate.**
- After hand-over a contributor optimised `core/src/clocked_line.hpp`, the header-only clocked line that S-014 requires and that Delay and Chorus share.
- `mutation.yml`'s `scope` job reported `mode: not-applicable`, because D-060's scope is `core/src/*.cpp` and `adapters/src/*.cpp`. `mutation-gate` was green.
- The full runs had never mutated that header either: each shard's include path is its own `.cpp`, so code inlined from `clocked_line.hpp`, `effect_base.hpp` and `tables.hpp` is never a mutant.
- The project's mutation score therefore excluded the D-033 algorithm, the most intricate code in two effects, without saying so. (PM2-03)

**IR-4. `rack-macos` went red at S-033 for a reason the plan did not anticipate.**
- The job ran `cmake … -DCMAKE_CXX_FLAGS="-O3 -march=nehalem …"` on the arm64 `macos-15` runner, and CMake's compiler check failed (X9).
- S-033's pre-decided branch covers "a numerical difference", not a compile error. The implementer spent three attempts before replacing the flags with Rack's arm64 set from `compile.mk` (`-march=armv8-a+fp+simd`). (PM2-04)

**IR-5. The owner could not merge.**
- The owner followed `reviews/MUTATION.md` and marked `aggregate` and `mutation-gate` as required, then opened the pull request.
- Its head was S-KNOW's log commit, `log: … [skip ci]`. GitHub skipped both workflows for the PR, the required checks stayed "Pending", and the merge button stayed disabled (X13).
- The owner then squash-merged with an admin override. GitHub's default squash message lists every commit message, many of which contain `[skip ci]`, so the push to `main` started no workflow either.
- `main` has never had a CI run. (PM2-05)

**IR-6. Mono DAW tracks lost half of Fluff Bank.**
- The VST3 shell called `BlockRunner` with two output pointers, using a scratch buffer for the right channel when the host's output bus was mono.
- T-854 checks that mono→mono is *supported* but processes only mono→stereo, so nothing ran a mono output bus. On mono tracks the user heard only filters 1, 3, 5 and 7: the product-side twin of PM1-01. (PM2-07)

**IR-7. Thread sanitizer on a user's host reported a race in `getTailLengthSeconds`.**
- The frozen contract says `getTailLengthSeconds(): effect.tailSeconds()`. D-016 says the `Effect` is used from the audio thread only.
- JUCE's VST3 wrapper calls `getTailLengthSeconds()` from the host's thread. The literal implementation reads parameter state that `processBlock` writes concurrently.
- S-ADVERSARIAL's stress does not call it. (PM2-08)

**Lower-impact incidents:**
- Delay LFO depth (¼ or 3× of D-023), FilterBank LFO depth and FilterBank retrigger-from-zero all passed every test and were noticed only as "choppy" patterns in dense sequences (PM2-06).
- A Rack user found that the Delay's Time CV at 12 o'clock reaches the top at 2.5 V. One copy of the manual says 0 to 5 V covers the whole range from 12 o'clock (C-PEDB-020), and D-012 never recorded that contradiction (PM2-09).

## 4. Lens-by-lens notes

**(a) Technical correctness of DSP decisions and frozen tests.**

Ring modulator:
- Sum and difference, ranges, LFO law, external carrier and dry path are pinned (T-201 to T-206).
- The `carrier_in` clamp of ±2 is untested (Low, folded into PM2-10).

Phaser:
- Dips, complementarity, sweep law, Sweep In, resonance and LFO rate are pinned.
- **LFO depth is not pinned (PM2-02).**
- `tanh` on the feedback is not observable at resonance ≤ 0.9 (Low).

Delay:
- Time law, feedback, bandwidth, clocked replay, bypass and spillover, insert, tap and the new loop darkening are pinned.
- **LFO depth is not pinned (PM2-06).**

Filter bank:
- Centres, Q, split, sliders, sequencer timing, envelope shapes 0/2/5/8, all 22 non-static patterns, rate law, tap (quarter→eighth), Step input and the Sweep end points are pinned.
- **LFO depth and retrigger-from-present-value are not pinned (PM2-06).**

Oscillator box:
- Range, V/oct, morph, VCA, sync, FM law, Env Amount and band-limiting are pinned.
- Sync hysteresis is partly caught by T-607. The `min(env, 1)` cap is not pinned (Low).

Tap tempo:
- Knob hand-back and range limits are pinned (T-406, T-506, T-706, T-812).
- CV hand-back is caught only by coincidence (see X6).

CV mapping:
- `CvNormalised` (T-012, two effects), V/oct (T-602) and trigger thresholds (T-812) are pinned. Re-arm hysteresis is not (Low).

Bypass:
- T-010 and T-407 are sound. The cross-fade duration is untested (Low, not raised).

Smoothing:
- T-014 accepts time constants of about 0.3 to 10 ms, and smoothing in the value domain instead of the normalised domain (D-017) also passes (Low, not raised).

VST3 shell (T-851 to T-856):
- Sound and satisfiable.
- Gaps: mono→mono processing (PM2-07) and the thread context of the tail query (PM2-08).
- T-852 checks the tail against the reference effect after 150 blocks, so an implementation that caches the tail in an atomic per block also passes. The remedy for PM2-08 needs no test change.

Adaptor tests:
- T-811 and T-821 are deterministic and margined, but the margin is thin: up to 1.7× (X7).
- One-ulp input differences (X12) are amplified by stateful paths. A variant that changed only tap state exceeded T-811's 1e-5 V (X6). Three legitimate variants stayed inside (PM2-13, Low).

Python repository tests:
- T-901 to T-907 read correctly.
- T-905's `aggregate` check is `(?s)\n  aggregate:.*?if:\s*always\(\)`, which a step-level `if: always()` (or a comment) also satisfies. GitHub treats a skipped required job as passing (PM2-11, Low).
- The guide-section split in T-907 is safe, because no effect name is a prefix of another.

**(b) Supply chain.**
- JUCE, Catch2, Rack, pluginval and four actions match their tags (X10).
- **`msys2/setup-msys2` does not match, and the pinned object cannot be fetched (PM2-01).**
- pluginval's seed option is confirmed working (X8).
- Mull is trust on first use, as round 1 found.
- T-905 checks only that a pin is 40 hexadecimal characters, not that it exists or matches the tag. An existence check (`git ls-remote` plus a fetch of the SHA) in `repo-checks` would have caught PM2-01.

**(c) Research assumptions.**
- C-PEDA-041 (Phaser Amount reaches the full six-octave range) underpins the ±3 octaves that PM2-02 leaves unpinned.
- C-PEDB-020 (MF-104M Time: 0 to 5 V from 12 o'clock covers the whole range) contradicts D-012's uniform depth-1.0 additive law, which is generalised from C-PEDA-011 (MF-101). The contradiction is not recorded in `contradicted_by`, in D-012 or in D-027 (PM2-09).
- C-PEDA-046 (Sweep In ±2.5 V) gives no octave scaling. D-022's 6 octaves per core unit (±3 octaves at ±2.5 V) is a design choice missing from D-027's owner-visible list (PM2-12).
- C-PEDA-044 supports the Phaser's Out/Aux stereo default (not a finding).
- C-PLANNER-009 is labelled `verified` but contains a wrong SHA (PM2-01).
- S-016 cites C-PEDB-020 and C-PEDB-021, which are Delay claims, as the FilterBank's claims (PM2-12).

**(d) Implementer misinterpretation.**
- S-033 uses x86 flags on an arm64 job (PM2-04).
- S-FINAL says "read the CI runs: `ci.yml` on the head commit", but at that moment the head is a `[skip ci]` log commit with no run (PM2-05).
- S-014's "header-only" combined with D-060's `*.cpp` scope (PM2-03).
- The thread rule against the tail contract (PM2-08).
- D-022's "3 × lfo_amount" is clear in the decision, but T-307's title is the more likely source an implementer reads, and its assertion accepts any depth (PM2-02).

**(e) Integrity (Rule 9, checks that cannot fail, hand-transcribed numbers).**
- The msys2 SHA is hand-transcribed or recalled, yet labelled verified (PM2-01).
- T-307's title asserts a property its assertions do not check (PM2-02).
- `mutation-gate` reports success (`not-applicable`) for changes to header-only DSP code, and the full-run score silently omits that code (PM2-03).
- T-905's `aggregate` regex (PM2-11).
- Checked and sound:
  - pluginval's pass rule fails on the negative control (X8);
  - `check_frozen.py` passes (X5);
  - the T-304 amendment is honest about its estimator;
  - RESULT.md's numbers match my reruns (299451 assertions, 83 cases).

**(f) Past lessons.**
- `L-20261028T200000Z-pinned-tool-version-does-not-exist` (Medium in the store) recurs in a new form: a pin that does not exist (PM2-01). Its recorded application is not true for this pin.
- `L-20261112T205000Z-test-estimator-tighter-than-its-stated-tolerance` (this run's own lesson) was applied to T-304. I applied its idea to the other zero-crossing and edge-count estimators (T-202, T-203, T-406, T-506, T-706) and found them adequately margined.
- `L-20261007T150600Z-control-mapping-formulas-unverified` (High): the formulas were exercised by the oracle, so the lesson's own mistake does not recur. But several mapping formulas are not frozen by any test (PM2-02, PM2-06). The implementer's version of those formulas is unconstrained, which is the risk class the lesson was written for. I rate them by the rubric, not at the lesson's severity, because the lesson concerns the planner's formulas.

**(g) Performance and CI time.**
- The `[skip ci]` fix removes the queueing of round 1.
- New CI time: `rack-macos` builds the core twice (Rack and CMake), about 2 to 3 minutes. Acceptable.
- Mutation timing: unchanged from round 1.
- The benchmark gate's margin is still above 2.5×.

**(h) Security.**
- Workflow permissions: `contents: read` at the top level (T-905). Job-level write only in `evidence.yml` and the scheduled-issue job.
- Triggers: push with marker prefix. No `pull_request_target`.
- Evidence-branch push: GITHUB_TOKEN pushes do not trigger workflows. The job builds pinned code with write scope (accepted in round 1). `actions/checkout` persists the token in `.git/config` for later steps unless `persist-credentials: false` is set. That is good practice, but only commit-pinned Catch2 runs there (not raised).
- Downloaded binaries: pluginval is hash-verified; Rack SDK and Mull are trust on first use. If GitHub still serves the dangling msys2 object, the `rack-windows` job would run code whose provenance is unknown under a read-only token. That is the security face of PM2-01; it is not Critical, because no write scope or secret is present.
- State parsing: JUCE 9.0.5 parser limits (round 1).
- No new Critical.

**(i) Operations after hand-over.**
- PR and merge blocked by `[skip ci]` heads (PM2-05).
- `deps-scan` runs only on push, so after hand-over no advisory query runs unless someone pushes (PM2-14, Low).
- The R-017 and R-018 upkeep lists are in `COMPLETION.md`.

**(j) CI platform coverage.**
- Every A-006 platform maps to jobs.
- New breakages, both certain:
  - `rack-windows` fails on the action pin (PM2-01);
  - `rack-macos` fails on the x86 flags (PM2-04).
- No label drift beyond R-017.
- Linux floor: the Rack plug-in built on Ubuntu 24.04 needs glibc 2.39 like the VST3 bundles. D-071 states the floor as the build image, which covers it.

## 5. Risk table

| ID | Description | Lens | Severity | Likelihood | Root cause | Trace | Evidence checked | Proposed mitigation |
|---|---|---|---|---|---|---|---|---|
| PM2-01 | `msys2/setup-msys2` is pinned to `2ff06a04f6ed…`, which is not tag v2.33.0 (that is `ec48f7c5447b…`, dated 2026-09-27) and is in no branch or tag history. `rack-windows` cannot resolve the action, so S-033 fails. HANDOFF §6 makes a pin change a halt, so the implementer is blocked at S-033 and everything gated on `aggregate` waits. If GitHub still serves the dangling object, an unknown commit runs instead. The claim is labelled `verified`, and a recorded lesson's application asserts every pin was read. | (b), (e), (f), (j) | **High** | certain | SHA recalled or transcribed (from the knowledge store), not re-read. T-905 checks only the format of a pin. | `plan/ENVIRONMENT.md` actions table; C-PLANNER-009; S-033; HANDOFF §6; L-20261028T200000Z | X10, X11 | Replace with `ec48f7c5447b3140e2b088413ae3a55687bccb6e` after re-reading the tag. Correct C-PLANNER-009 and its knowledge item (`supersedes`). Add a lesson recurrence. Add to S-001 a check that every `uses:` SHA is fetchable and equals its tag's commit (for example `git ls-remote` of the tag in `repo-checks`). |
| PM2-02 | Phaser LFO depth (D-022: ±3 octaves at Amount 1, the "full sweep range" of C-PEDA-041) is not pinned. A ±1-octave implementation passes all 83 tests. T-307's title states "up to +/-3 octaves", but its only assertion is >20 dB of tone modulation. Rated High rather than Medium because the swept range is the Phaser's defining behaviour (SC-2 "ranges"), and because the frozen test claims to pin it. | (a), (e) | **High** | medium | Assertion weaker than the test title. Depth is measured indirectly. | D-022; T-307; C-PEDA-041; SC-2 | X6 variant `phaser_lfo_depth` (83/83) | Amend through the test-challenge route: with Square LFO at 1 Hz, Amount 1 and Sweep 5, measure the middle dip by `dips()` in each half-cycle and require 1.1 kHz·2^(±3) within 3 % (clamped at 20 Hz and 0.45 fs; at 96 kHz the upper value 8.8 kHz is inside). Add Amount 0.5 → ±1.5 octaves. Show it red against the variant. |
| PM2-03 | Header-only DSP is outside mutation testing. S-014 requires `clocked_line.hpp` to be header-only (D-033, used by Delay and Chorus), and `effect_base.hpp` and `tables.hpp` may hold inline code. D-060 scopes `core/src/*.cpp`, and the shards are per `.cpp` with per-file include paths. So (i) full runs never mutate header code and the score omits it silently, and (ii) an incremental change to a header gives `mode: not-applicable` and a green `mutation-gate`. The gate reports success for DSP changes it did not test. | (e), (a), (i) | **High** | certain (by plan text) | Scope written as a file glob of `.cpp` files. Header-only placement decided separately. | D-060; S-MUTATE "Scope", "Shards", "Triggers (a)"; S-014; `mull_include_paths.py` | PLAN.md S-014, S-MUTATE; skill scripts | Scope `core/src/*.{cpp,hpp}` and `adapters/src/*.{cpp,hpp}`. Map `clocked_line.hpp` to `"[delay],[chorus]"` and the other private headers to the shared list. Each shard's include paths list its `.cpp` and the headers it owns. `scope` treats a header change as in-scope. State it in `reviews/MUTATION.md`. Or move `ClockedLine` into `clocked_line.cpp`. |
| PM2-04 | PM1-19's fix does not hold: `rack-macos` (arm64 `macos-15`) runs CMake with `-march=nehalem`, which Apple Clang on arm64 rejects. The core tests with Rack's flags never run there, and S-033's failure branch covers only numerical differences. | (j), (d) | Medium | certain | x86 flag set copied to an arm64 job. | S-033; D-006 | X9 | Use the flags `compile.mk` applies per architecture: `-march=armv8-a+fp+simd` on arm64, `-march=nehalem` on x64. Say so in D-006 and S-033. |
| PM2-05 | `[skip ci]` heads at hand-over. Every step ends with a `log: … [skip ci]` commit, so the branch head, and therefore the PR head, never runs `ci.yml` or `mutation.yml`. Required checks (which `reviews/MUTATION.md` recommends) stay Pending and block the merge. A default squash merge carries `[skip ci]` into `main`'s commit message, which skips the post-merge run. S-FINAL's "read `ci.yml` on the head commit" has no run to read. | (i), (d) | Medium | high | PM1-09's fix applied to every log commit, including the last. | PLAN rule l.7; S-FINAL; S-KNOW; S-MUTATE sub-step 4 | X13 | S-FINAL (and S-KNOW last) ends with an empty commit without the marker, for example `final: CI on hand-over head`, and judges CI on it. `COMPLETION.md` tells the owner to use a merge commit, or to remove the `[skip ci]` lines from the squash message. |
| PM2-06 | Other modulation laws are not pinned: Delay LFO depth (D-023, one octave each way at Amount 10; ¼ and 3× both pass); FilterBank LFO shift amplitude (±0.5 octave; ±0.2 passes); FilterBank retrigger "restarts the rise from the present value" (from-zero passes, which gives clicks and choppy dense patterns with slow envelopes). | (a) | Medium | medium | Tests check presence of modulation, not its size. | D-023; D-024; T-405; T-504; T-507 | X6 variants `delay_lfo_depth`, `delay_lfo_depth3`, `fb_lfo_depth`, `fb_retrigger_zero` (all 83/83) | Add through HANDOFF §9.4 or a re-freeze: Delay square LFO, Amount 10, compare echo times in the two half-cycles (ratio 4 within 5 %); FilterBank LFO at 0.08 Hz, peak frequency over a cycle reaches 2^(±0.5)·fc; FilterBank pattern with consecutive triggers at envelope 6, envelope never falls below its value at the retrigger. |
| PM2-07 | VST3 mono→mono processing is untested. A shell that always passes two output pointers (a scratch right channel) passes T-851 to T-856, but on mono tracks Fluff Bank loses filters 2, 4, 6 and 8. (A literal `getWritePointer(1)` on a one-channel buffer is a JUCE assertion or out-of-range read instead.) | (a), (d) | Medium | low | T-854 checks only that the layout is supported. | D-013, D-016, D-024; T-854 | `tests/vst3/test_vst3.cpp` T-854; oracle shell uses `numOut = min(2, outputs)` | Add a VST3 test (§9.4): mono→mono layout, FilterBank, output equals `BlockRunner` with `numOut = 1`. Or state the rule explicitly in S-020 action 1. |
| PM2-08 | Thread contract conflict. `vst3_shell.hpp` says `getTailLengthSeconds()` returns `effect.tailSeconds()`. D-016 says the `Effect` is used from the audio thread only. Hosts call the tail query from their own thread, so the literal implementation is a data race that the S-ADVERSARIAL TSan stress does not exercise. | (d), (h) | Medium | medium | PM1-08's thread rule did not revisit the other host-thread entry points. | `vst3_shell.hpp`; D-016; S-021; S-ADVERSARIAL | Oracle shell calls `tailSeconds()` from the caller's thread | D-016: `processBlock` stores `tailSeconds()` in a `std::atomic<float>` and `getTailLengthSeconds()` reads it (T-852 still passes, because it calls after processing). Add the tail query to the stress thread. |
| PM2-09 | C-PEDB-020 (MF-104M, two copies of the manual): with Time at 12 o'clock, 0 to 5 V covers the whole range. D-012 applies an additive depth-1.0 law to every CV, from C-PEDA-011 (MF-101), so at noon the Delay's Time CV saturates at 2.5 V. The contradiction is not in `contradicted_by`, D-012 or D-027. | (c) | Medium | medium | Generalised from one pedal's manual. Contradicting evidence not recorded. | D-012; C-PEDB-020; C-PEDA-011; fixture `cv_time` depth | `research/claims.json` | Record the contradiction. Either list "Delay Time CV: knob + CV, not centred" in D-027 for the owner, or amend the depth through the amendment route. |
| PM2-10 | Smaller untested details: trigger re-arm hysteresis (0.02); OscBox `min(env, 1)` on the frequency term; Phaser `tanh` on the feedback; RingMod `carrier_in` clamp ±2. | (a) | Low | medium | No test at those points. | D-012, D-021, D-022, D-025 | X6 variants (83/83) | Add with the next §9.4 batch, or record in R-019. |
| PM2-11 | T-905's `aggregate` regex is satisfied by any later `if: always()`, including a step-level one or a comment. A job-level `if:` missing on `aggregate` would make it skipped on failure, and GitHub counts a skipped required job as passing. | (e) | Low | low | Lazy DOTALL match. | T-905 | `test_repo.py` | Next re-freeze: parse the job block up to the next two-space key and require a job-level `if: always()` line. |
| PM2-12 | Documentation slips: S-016 cites the Delay claims C-PEDB-020 and C-PEDB-021. The Sweep In scaling (6 octaves per core unit) is a design choice missing from D-027. | (c) | Low | high | Copy-paste; incomplete owner list. | S-016; D-022; D-027; C-PEDA-046 | PLAN.md l.191; claims | Cite the FilterBank claims (C-PEDB-03x). Add D-027 item 6a. |
| PM2-13 | Adaptor parity margins are thin: T-821 Phaser 5.96e-7 against 1e-6 (1.7×) and T-811 OscBox 3.8e-6 V against 1e-5 V (2.6×). One-ulp input differences from the volts round-trip (11 to 23 % of samples) are amplified by stateful paths. One variant exceeded T-811 by changing only tap state. Legitimate variants tested so far pass. | (a) | Low | low | Absolute tolerances chosen at the oracle's level. | T-811, T-821; SC-5 | X6, X7, X12 | No change now. If an implementation fails by a few ulps, the test-challenge route should consider a tolerance relative to the output level. Note in R-019. |
| PM2-14 | After hand-over `deps-scan` runs only on push, so no advisory query happens in a quiet repository. | (i) | Low | high | No schedule on the scan. | D-061; S-034 | PLAN.md S-034 | Add a weekly `schedule` to the scan (live after merge), with the same issue de-duplication as the mutation schedule. |

## 6. Counts by severity

| Severity | Count | IDs |
|---|---|---|
| Critical | 0 | none |
| High | 3 | PM2-01, PM2-02, PM2-03 |
| Medium | 6 | PM2-04, PM2-05, PM2-06, PM2-07, PM2-08, PM2-09 |
| Low | 5 | PM2-10, PM2-11, PM2-12, PM2-13, PM2-14 |

**Calibration notes:**
- **PM2-03:** I considered Critical ("a verification that reported success without having run"). It is rated High because `mutation-gate` prints its mode and the frozen suite still runs on such changes. The defect is that the plan's own placement of DSP code in headers puts it outside the gate without saying so.
- **PM2-01:** I considered Critical as a security exposure. It is rated High: the job holds a read-only token and no secrets. The certain consequence is that the implementer is blocked under HANDOFF §6.
- **PM2-02:** rated High under the brief's rule for untested behaviour central to an effect's identity. PM2-06's laws are not central to their effects, so they are Medium.

**Round 1 fixes that do not hold:**
- PM1-19 (PM2-04).
- PM1-08 and PM1-09 hold for what they fixed but introduced PM2-08 and PM2-05.

CRITICAL: 0 HIGH: 3 MEDIUM: 6 LOW: 5

---

## Planner's disposition (added after the round; the text above is the reviewer's report, unedited)

| Item | Severity | Disposition | Where |
|---|---|---|---|
| PM2-01 | High | Fixed: the `msys2/setup-msys2` commit was wrong; corrected to the tag's commit after re-reading every pin with `git ls-remote`; claim and knowledge item corrected; S-001 gains `tools/check_pins.py`, which compares every pinned commit with its tag in CI. Lesson recorded. | `plan/ENVIRONMENT.md`, C-PLANNER-009, S-001 |
| PM2-02 | High | Fixed with new frozen test T-308; the one-octave variant fails exactly that test. | D-063 |
| PM2-03 | High | Fixed: the clocked delay line is a `.cpp` with its own mutation shard; private headers hold declarations only; a header change selects a full mutation run. | S-014, S-MUTATE |
| PM2-04 | Medium | Fixed: per-architecture flags. | S-033 |
| PM2-05 | Medium | Fixed: the hand-over head is a commit without `[skip ci]`; merge advice in `COMPLETION.md`. | S-FINAL, S-KNOW |
| PM2-06 | Medium | Fixed with new frozen tests T-411, T-508, T-509; each variant fails exactly its test. | D-063 |
| PM2-07 | Medium | Fixed with new frozen test T-857. | D-016, D-063 |
| PM2-08 | Medium | Fixed: the tail length crosses threads through an atomic; the stress test calls it. | D-016, S-ADVERSARIAL |
| PM2-09 | Medium | Recorded: the contradiction is on the claim and in the owner-visible list. | C-PEDB-020, D-027 item 13 |
| PM2-10, PM2-13, PM2-14 | Low | Accepted. | R-019 |
| PM2-11 | Low | Fixed in the frozen test. | T-905 |
| PM2-12 | Low | Fixed. | S-016, D-027 item 14 |

Result of round 2: 0 Critical, 3 High, all fixed. The frozen tests changed again (87 core cases, 7 VST3 cases), so a cold read of the changed parts and a third round follow.
