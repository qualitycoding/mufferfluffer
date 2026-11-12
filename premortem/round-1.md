# Pre-mortem, round 1: mufferfluffer (seven effects, VST3 and VCV Rack 2)

Reviewer: fresh-context agent (Opus tier) that did not write the plan. Branch reviewed: clone at `/home/claude/pm1`, head `65882b4` ("Phase 3 complete: cold-read gate passed at pass 4"). Protocol: `plan/protocol/planning-protocol.md`, Phase 4 steps 1 to 3, severity per Global Rule 4 (lines for `[All]` and `[software]`). Date: 2026-11-12. Nothing in the clone was edited; all experiments ran in copies under `/tmp/pm1work`.

## 1. Method and what was executed

Read in full: `HANDOFF.md`; `plan/ASSUMPTIONS.md`, `DECISIONS.md`, `ENVIRONMENT.md`, `PLAN.md`, `GATES.md`, `RISKS.md`, the first part of `TRACEABILITY.md`; the four frozen headers; every frozen test (`tests/core/*`, `tests/vst3/*`, `tests/py/test_repo.py`, `tests/perf/bench.cpp`, both test `CMakeLists.txt`); `tools/check_frozen.py`; the fixture; `research/spikes/red-verification/RESULT.md`; `research/spikes/juce-vst3-build/*`; the parts of `research/spikes/oracle/oracle_core.cpp` named below; `research/PRIOR_KNOWLEDGE.md` (lesson table); the load-bearing `single-source`, `unverified` and planner claims in `research/claims.json` (list in section 3c); the three skills under `plan/protocol/skills/` (mutation SKILL files, `mull_score.py`, `per-file-matrix.yml`); the run's own lessons in `lessons/`.

Executed (all outputs kept in `/tmp/pm1work`):

| # | What | Result |
|---|---|---|
| E1 | Oracle core + frozen core tests, GCC 13.3, Release (`-DMUFF_IMPL_DIR=research/spikes/oracle`, cached Catch2) | `All tests passed (299407 assertions in 80 test cases)`; `ctest` 80/80; `muff_bench --limit 5` exit 0 (0.65 % to 1.89 %) |
| E2 | Same with Rack's flags `-O3 -march=nehalem -funsafe-math-optimizations -fno-omit-frame-pointer` | 80/80 pass (confirms the planner's claim) |
| E3 | Same with `-march=haswell -ffp-contract=fast` (emulates FMA contraction as Apple clang on arm64 does by default) | 80/80 pass |
| E4 | Per-shard test time of the S-MUTATE test lists (Release and `-O0`) | `[filterbank]` 9.5 s Release / 17.1 s `-O0`; `[oscbox]` 2.8 / 4.2 s; `[phaser]` 2.4 / 3.8 s; `[delay]` 0.9 / 2.1 s; `[lowpass]` 0.7 / 1.3 s; shared list 0.6 s; `[adapters]` 0.3 s |
| E5 | Downloaded the three pluginval v1.0.4 archives and hashed them | all three SHA-256 values equal those in `plan/ENVIRONMENT.md` and S-001 |
| E6 | `pluginval --help` (Linux binary) | option is `--random-seed`; there is no `--seed` |
| E7 | pluginval negative control on an empty `NotAPlugin.vst3` with the exact D-007 command line | exit 1, log `FAILURE`; log line `Random seed: 0x194c455` although `--seed 0x6d756666` was given |
| E8 | Built one real plug-in (`Fluff Delay`, oracle core + `research/spikes/oracle_vst3/processor.cpp`, JUCE 9.0.5, `juce_add_plugin` with the D-005 properties) and ran the D-007 command under `xvfb-run -a` | exit 0 in 13 s; log contains `SUCCESS` and 27 `Starting tests in:` lines; `Random seed: 0x3272c3a` (random again); `INFO: Skipping vst3 validator as validator path hasn't been set` |
| E9 | Mutant M1 of the oracle: Delay feedback taken from the unfiltered line read instead of the filtered `wet` (D-023) | all 80 core tests still pass |
| E10 | Mutant M2 of the oracle: Phaser LFO rate multiplied by 1.0002 (0.02 %) | T-304 fails: `542.67 == Approx(560.0)`; 6 of 7 `[phaser]` cases pass |
| E11 | `make -n` of a minimal `rack/Makefile` written as D-006 prescribes, against Rack v2.6.6 `plugin.mk` | objects of `../core/src/*.cpp` go to `build/../core/src/<f>.cpp.o`, i.e. `rack/core/src/` |
| E12 | Read Rack v2.6.6 `compile.mk`, `plugin.mk`, `arch.mk`, `Makefile` (sdk target), `include/plugin/callbacks.hpp`, `src/engine/Engine.cpp` | `-std=c++11` precedes the plug-in's `FLAGS` (C++17 wins); `init` is `extern "C"`; engine dispatches `onSampleRateChange` on add |
| E13 | Cloned `mull-project/mull` (head `b383a52`, tag 0.38.0 dated 12 Nov 2026) | execution status enum, Mull's own score rule, SQLite schema, Ubuntu 24.04 LLVM 16-20 support read from source |
| E14 | Read JUCE 9.0.5 `juce_XmlDocument.cpp`, `juce_AudioProcessor.cpp` (headless), `juce_audio_plugin_client_VST3.cpp` | XML parser has nesting and entity-amplification limits with unit tests; `getTailSamples` maps infinity to `kInfiniteTail` |
| E15 | `grep` of every tracked product file for the T-901 pattern | no hits today |
| E16 | Web: GitHub changelog on macOS runner images | Intel macOS support ends when the macOS 15 image is retired in autumn 2027 ([GitHub changelog](https://github.blog/changelog/2025-09-19-github-actions-macos-13-runner-image-is-closing-down/)) |

Not executed: Windows and macOS builds (no such hosts); the Rack SDK archives (`vcvrack.com` refused by this sandbox's proxy, as in planning); a Mull run (not installed here); the GitHub REST API for Mull release assets (not enabled for this session).

## 2. Incident reports (six months after hand-over; the plan was followed exactly)

**IR-1. The owner rejected the filter bank on evidence that showed half of it.** At G-LISTEN round 1 the owner answered L-5 "the patterns sound sparse and lopsided; please transcribe the hardware's patterns". The implementer could not do that without the legal decision R-003 leaves to the owner, so round 2 offered `proceed-with-rescope`, and the owner accepted a "thin" filter bank. Months later the owner loaded Fluff Bank in Rack with only the left output patched and heard all eight bands for the first time. Cause: S-LISTEN tells `muff_render` to call `muff::createEffect` directly and write `left = output 0`, `right = copy` for every demo not marked "stereo" (`plan/PLAN.md` line 461 and action 1, line 487). For FilterBank, output 0 carries only filters 1, 3, 5, 7 (D-024); the Left + Right sum is applied by the adaptors (`monoSumsRight` in `BlockRunner` and `RackBridge`), which the renderer bypasses. Demos 14, 15 and 17 therefore contain the odd bands only; demo 16 is the only filter-bank demo with both halves. No check in `render_demos.py` (peak, RMS, size) can see it. (PM1-01)

**IR-2. pluginval went red on Windows twice a month and nobody could reproduce it.** D-007 runs pluginval with `--seed 0x6d756666` "so that a failure is repeatable". pluginval 1.0.4 has no `--seed` option: it silently ignores it and prints a fresh `Random seed:` every run (E6 to E8). S-021's pre-decided branch for a rejected option never fired, because the option is not rejected. When a state-restoration check failed intermittently (the R-011 pattern), S-021's diagnosis "re-run once with the same seed" ran with a different seed, the failure looked intermittent, both logs were recorded, and the job was re-run until green. After hand-over the owner saw sporadic red `aggregate` runs with no way to replay them. (PM1-02)

**IR-3. A correct phaser failed a frozen test, and three attempts were spent before the test was challenged.** During S-018's optimisation round the implementer moved the Phaser LFO's rate computation to control rate. The rate moved by a few parts in ten thousand; T-304's 250 Hz case (fs 8 kHz) failed at 3 % while its stated tolerance is 2 %. T-304 measures rate as mean absolute slope of a sampled triangle; with exactly 32 samples per cycle the sampled peaks land on the true peaks only if the rate is exact to about 1e-5. Any drift sweeps the sample grid across the peaks and loses on average 1/32 of the travel (E10: a 0.02 % rate change gives 542.7 instead of 560). (PM1-03)

**IR-4. "The repeats are too clean" was answered by turning the filters down, not by fixing the loop.** The implementer's Delay took its feedback from the raw delay-line read instead of the filtered wet signal. D-023 says the loop is fed by the low-passed `wet`, which is what makes analogue repeats darken one after another. All 80 frozen tests passed with this defect (E9: the oracle with exactly this change passes 80/80), because T-402 measures repeat gain with a 500 Hz burst (inside the 3 kHz pass-band) and T-404 measures a single pass. The same gap exists for the Chorus loop (D-026, T-703 measures at 100 and 200 Hz). (PM1-04)

**IR-5. Fluff Lowpass's 2-pole mode never self-oscillated.** The implementer built 2-pole mode as a separate two-stage loop. D-020 and D-027 item 4 require the four-stage loop to stay intact and the output to be tapped at stage 2. The only 2-pole test (T-102) runs at resonance 0, where the two designs are identical, and every resonance test (T-103, T-104, T-105, T-109, T-018) uses the default `poles` = 4-pole. Demo 03 plays 2-pole briefly at resonance 9; the owner heard "the 2-pole mode is tamer" and accepted it. (PM1-05)

**IR-6. The hands-on packages did not load on the owner's machines.** After G-002 `proceed` the owner downloaded the macOS and Linux bundles. On an Intel Mac that cannot run macOS 15, the host refused the bundle: nothing in D-005, `plan/ENVIRONMENT.md` or the workflows sets `CMAKE_OSX_DEPLOYMENT_TARGET`, so builds on `macos-15` runners record the SDK's macOS version as the minimum (contrast Rack's `compile.mk`, which sets 10.9 for the Rack build). The Linux bundle needed the glibc and libstdc++ of Ubuntu 24.04. The macOS download also carried the quarantine attribute, which `README.md`'s `## Install` text (S-035) does not mention. H-1 came back "does not load", which cost an S-LISTEN iterate round. (PM1-06)

**IR-7. A host rejected the plug-ins on a VST3 conformance point that pluginval never checked.** pluginval runs Steinberg's VST3 validator only when given `--vst3validator <path>`; otherwise it logs a skip (E8). The plan never builds or passes the validator. Lesson `L-20261028T215000Z-vst3-validator-not-in-local-suite` is recorded as applied by T-851's "Default" program name, which fixes the one symptom that lesson saw, not the class. A later conformance issue that the validator would have reported reached the owner's DAW instead. (PM1-07)

**IR-8. ThreadSanitizer on the owner's machine (or a stricter host) flagged the editor.** `core.hpp` line 8 says an `Effect` is used from one thread at a time. D-016 says the editor reads meters with a 30 Hz timer, and nothing in the plan says how the meter values cross from the audio thread to the message thread. The implementer called `effect.getMeter()` from the timer. S-ADVERSARIAL's TSan stress drives parameters and state but never opens the editor, so the race went unreported. (PM1-08)

**IR-9. Implementation took twice as long as planned because CI queued on macOS.** Every step pushes a step commit and then a log commit; both run the full `ci.yml` (six macOS jobs once S-033 is done), and the per-SHA concurrency group can never cancel. GitHub's macOS concurrency for a free account (five jobs) is smaller than one run's macOS job count, so every Done-when waited behind the previous log commit's macOS jobs. (PM1-09)

**IR-10. The mutation score was higher than Mull's own.** To shorten the 9.5 s filter-bank shard, the implementer enabled Mull's coverage reporting. `mull_score.py` (copied from the skill) counts every status other than 2 as killed, so status 8 (`NotCovered`) and status 6 (`DryRun`) became kills. Mull itself counts `NotCovered` as not killed. Sub-step 2's cross-check compares only the survivor count, so it could not catch this. (PM1-10)

**IR-11. In autumn 2027 CI went permanently red.** GitHub retired the macOS 15 Intel image. `core-macos-intel`, `vst3-macos-intel` and `rack-macos-intel` could no longer run, `aggregate` failed on every push, and the job names are frozen in T-905's `CI_JOBS`. The owner had no pre-decided response. (PM1-11)

Lower-impact incidents (one line each): Rack `make` objects appeared as untracked files in `rack/core/` and `rack/adapters/` and were nearly committed to the public repository (PM1-12); `mull_score.py --write-baseline` overwrote `project_floor` in `mutation-baseline.json` (PM1-13); a host cut off the Phaser's resonant tail because `tailSeconds()` is untested for four effects (PM1-14); an aligned allocation in `process` escaped T-011 (PM1-15); the weekly mutation run stopped silently after 60 days without commits (PM1-18).

## 3. Lens-by-lens notes

**(a) Technical correctness of DSP decisions and frozen tests.** Checked: the ladder algebra of D-030 (the closed form for `S` and `u` is correct for four TPT one-poles); the phaser dip geometry and power complementarity (T-301, T-302); the SVF (D-034); the clocked line (D-033, T-409); tap rules (T-022, T-406, T-506, T-706); counts per tag in every Done-when (9/6/7/9/7/9/7 plus 80 and 79); robustness under Rack flags and FMA contraction (E2, E3, both pass). Findings: two decisions are not pinned by any test (PM1-04 feedback-loop filtering; PM1-05 2-pole loop); one test is far stricter than its stated tolerance (PM1-03, demonstrated); `tailSeconds()` is tested only for Lowpass, Chorus and Delay (PM1-14); smoothing (T-014) accepts any time constant from about 0.3 ms to 10 ms, which is acceptable for a "no click" test. Flakiness: no core test uses wall-clock time. Exact-equality assertions (T-008, T-013, T-801, T-802) compare the same code path in one process, so they do not depend on the compiler. T-821's 1e-6 parity holds with the oracle's `× 0.2f` input scaling. The phase lattice of T-304 is the only sample-rate-commensurability trap found.

**(b) Dependency and supply-chain drift.** pluginval hashes verified (E5). JUCE and Catch2 are pinned by commit and fetched by git. Actions are pinned by SHA and T-905 enforces it. Rack SDK: trust on first use is stated in D-003, and the response to a later archive change (hash mismatch, halt) is acceptable. The `rack-sdk.bytes` floor of 5 000 000 could not be checked because `vcvrack.com` is unreachable here. By the sdk target, the Windows archive holds headers plus an import library, so it may be the smallest; if it falls below the floor, S-030 blocks on a planner-chosen number (unverified, not raised as a risk). Mull: 0.38.0 was tagged today, after the plan's "0.37.0 newest" note. `mull-runner-N` has been deprecated since 0.36. Ubuntu 24.04 packages still cover LLVM 16 to 20 at head (E13). S-MUTATE's "newest release with an LLVM 19 asset" rule absorbs this. Unpinned: container images by tag (`alpine:3.22`, `ubuntu:22.04`), `apt`, `brew` and MSYS2 packages (rolling MinGW GCC for the Rack Windows build) (PM1-17, Low).

**(c) Invalid research assumptions (single-source, inferred and unverified claims that decisions rest on).** Reviewed the following:
- C-PLANNER-008: the claim is right as far as it goes, but D-007 adds `--seed`, which no claim supports and which is false (PM1-02).
- C-PLANNER-010 and 011 (runner labels): true today, but the Intel macOS label ends in autumn 2027 (PM1-11).
- C-PLANNER-012: partly stale (0.38.0; the runner was renamed), already acknowledged in R-010.
- C-PLANNER-015: confirmed from `compile.mk` (E12).
- C-PLANNER-018: plausible.
- C-LEGAL-036 (artefacts of a public repository): consistent with GitHub's documented behaviour.
- C-DSP-061: R-007 already covers it.
- C-PEDA-001 says the cutoff knob midpoint is about 500 Hz, while D-020's log law and T-003 freeze 424 Hz (a 0.24-octave difference; fidelity, Low, PM1-20).
- C-PEDB-011: contains its own internal contradiction (unity at 8 versus oscillation at "3 o'clock"). D-027 already records it as a design choice.
- C-PEDA-029 and C-PEDC-011: single-source figures frozen in T-202, T-203 and T-012. R-001's amendment route covers them.

**(d) Implementer misinterpretation.** Literal-reading traps found:
- The demo renderer's channel rule (PM1-01).
- `--seed` (PM1-02).
- Meter access across threads (PM1-08).
- The Rack object-path side effect (PM1-12).
- The dual schema of `mutation-baseline.json` (PM1-13).
- The S-MUTATE Done-when depends on a log commit's mutation run, which interacts with any `[skip ci]` fix for PM1-09.

Checked and found sound:
- The `-k` arithmetic in S-035 (8 tests).
- The Catch2 filter syntax and the "no tests matched" exit behaviour.
- The marker-commit mechanics: a push of an empty commit carries `head_commit`.
- Pushes made with `GITHUB_TOKEN` do not re-trigger `ci.yml` on the evidence branch.

**(e) Integrity (Rule 9; checks that cannot fail).**
- pluginval's pass rule can fail (E7, negative control) and its strings exist in a real passing log (E8), so S-021's fallback that edits the strings will not be triggered.
- `aggregate` fails on `skipped`.
- The benchmark exits non-zero against stubs.
- `check_frozen.py` detects new files.
- Hashes are recorded by `apply_rack_record.py`, not typed.
- One near-integrity defect: `mull_score.py` can turn not-run mutants into kills (PM1-10). Its cross-check is blind to this.
- Sub-step 2 of S-MUTATE pastes a message, which is acceptable: the `computational` integrity line of Rule 9 does not apply to this profile.

**(f) Past lessons.**
- `L-20261028T215000Z-vst3-validator-not-in-local-suite` (Medium): the class can recur (PM1-07).
- `L-20261101T041800Z-perf-harness-must-exercise-what-it-reports` (High): its "application" column is boilerplate about T-905 and `pipefail`, unrelated to the lesson. The benchmark uses static parameters and never exercises host automation or CV. Because the bench's LFO and modulation settings already force per-sample coefficient work, I judge the residual gap Low (PM1-16).
- `L-20261008T173531Z-editor-note-labels-diverged-from-midi-map` is marked N/A in `research/PRIOR_KNOWLEDGE.md` but cited as applied in S-010 and S-031 (documentation inconsistency, PM1-16).
- `L-20261101T140000Z-intermittent-host-failure-needs-a-stress-test` is applied through S-ADVERSARIAL. The broken seed (PM1-02) weakens the S-021 half of that defence.
- `L-20261112T120500Z` (dispatch and schedule only on the default branch): applied correctly everywhere I checked.

**(g) Scale and performance.**
- 5 % gate: the oracle measures at most 1.9 % (E1), which leaves room.
- pluginval takes 13 s per plug-in at strictness 10 on Linux (E8): about 1.5 min for seven, well inside the 60-min VST3 jobs.
- Mutation: measured shard baselines (E4) are larger than the plan's "1 to 4 s" estimate for `[filterbank]` (9.5 s), but even 250 mutants × 17 s / 4 workers is about 18 min. The 90-min shard budget holds unless many mutants time out at 10× baseline (about 95 s each).
- CI wall clock is dominated by macOS queueing (PM1-09).
- The Rack path calls `setParam` for every parameter on every sample, and it is not benchmarked (Low, not raised).

**(h) Security.**
- State parsing: JUCE 9.0.5's XML parser caps element nesting (256), entity nesting (40) and entity amplification, with its own unit tests (E14). S-ADVERSARIAL's deeply-nested and 100 000-children attacks remain worthwhile, but there is no billion-laughs exposure.
- `copyXmlToBinary` / `getXmlFromBinary` checks a magic number and a length.
- Workflow permissions: `contents: read` at the top level is enforced by T-905. Write scopes are job-level only (`evidence.yml`, the scheduled-failure issue job).
- No `pull_request_target` (enforced by T-905). Fork pull requests get read-only tokens.
- The evidence job compiles repository code and FetchContent's pinned Catch2 while holding a write token; acceptable for an owner-only marker trigger.
- Downloaded binaries: pluginval is hash-verified, the Rack SDK is trust on first use, and the Mull `.deb` is trust on first use at S-MUTATE.1.
- No new security finding.

**(i) Operations and maintenance after hand-over.** Required checks are only recommended. Scheduled workflows go live only after the merge. A public repository disables scheduled workflows after 60 days without activity, and the issue-on-failure job opens a new issue each week with no deduplication (PM1-18). The Intel macOS label ends in autumn 2027 (PM1-11). OSV outages turn `aggregate` red (pre-decided in S-034). Node-runtime deprecations of pinned actions will need SHA bumps. The owner's to-do list after hand-over is not written down in one place; COMPLETION.md covers merge and live triggers only.

**(j) CI platform coverage and label drift.** Every A-006 platform maps to jobs. macOS x64 pluginval is waived by D-007, and Rack modules are never loaded in Rack (R-012). The ARM64 Linux, musl and oldest-toolchain extras are present. Label drift: `macos-15-intel` has a published end of life (PM1-11). `windows-2022` is fine today. Rack-flag numerics are tested only with GCC on `rack-linux`; the clang builds on `rack-macos` and `rack-macos-intel` use `-funsafe-math-optimizations` with no core test run (PM1-19, Low). Binary portability floor (minimum macOS, glibc, MSVC runtime) is undecided (PM1-06).

## 4. Risk table

| ID | Description | Lens | Severity | Likelihood | Root cause | Traces to | Evidence checked | Proposed mitigation |
|---|---|---|---|---|---|---|---|---|
| PM1-01 | Demos 14, 15 and 17 (Fluff Bank, not marked "stereo") contain only filters 1, 3, 5, 7. The renderer writes output 0 and a copy, bypassing the adaptors' `monoSumsRight` rule. The owner judges L-5 (and through it R-003's transcription question) on half the bank. Rated High, not Critical: the product itself is unaffected and an attentive listener can compare with demo 16. | (d), (a) | High | high | S-LISTEN defines the stereo rule per demo instead of reusing the host rule of D-013/D-024. `muff_render` calls `createEffect` directly. | S-LISTEN "Fixed content" (PLAN.md l.461) and action 1 (l.487); D-024; fixture `mono_sums_right: true` for FilterBank | PLAN.md l.461, l.487; DECISIONS.md D-024; `tests/fixtures/effects.json`; `adapters.hpp` BlockRunner comment | `muff_render` drives the effect through `BlockRunner` with `numOut = 2`, or applies `monoSumsRight` itself when `right` is `copy`. Mark every FilterBank demo "stereo", or sum. Add a renderer self-check that a FilterBank demo's left channel has energy at a bass-voicing even-band centre (for example 160 Hz with pattern 1). |
| PM1-02 | `--seed 0x6d756666` is silently ignored by pluginval 1.0.4 (the option is `--random-seed`), so every run uses a random seed. S-021's "rejected option" branch never fires, and its "re-run with the same seed" diagnosis cannot work. Intermittent failures become irreproducible CI noise. | (b), (c), (e) | Medium | high (certain) | Command line taken from memory; C-PLANNER-008 never covered `--seed`; not spike-run. | D-007; S-021 On failure (PLAN.md l.263); R-011; C-PLANNER-008 | E6 (`--help`), E7 and E8 logs: `Random seed: 0x194c455`, `0x3272c3a` | Use `--random-seed 0x6d756666`. Make `run_pluginval.py` assert that the log's `Random seed:` line equals the requested seed. Record the corrected command in D-007 with E6 as its verification. |
| PM1-03 | T-304's 250 Hz case at 8 kHz effectively demands the LFO rate to about 1e-5. A 0.02 % rate error fails a test whose stated tolerance is 2 % (measured 542.67 against 560). | (a) | High | low | Mean-absolute-slope estimator of a sampled triangle with exactly 32 samples per cycle; it passes only on an exactly commensurate phase lattice. Average loss when incommensurate is 1/32 (3.1 %). | T-304 (test_m1.cpp l.320-336); D-022 | E10 (mutant M2) | Amend through section 6: measure the rate by rising zero crossings (as T-203 does), or use fs = 96 kHz for the 250 Hz case, or a window and tolerance that cover the worst peak-sampling loss (2/N). Re-verify red and green. |
| PM1-04 | The Delay (and Chorus) feedback loop may take the unfiltered line read instead of the filtered `wet`; all 80 tests still pass. Repeats would not darken progressively, contrary to D-023/D-026. | (a) | Medium | medium | Loop tests measure gain in the pass-band (500 Hz, 100/200 Hz) and the bandwidth tests measure one pass. | D-023, D-026; T-402, T-404, T-703, T-707 | E9 (mutant M1 passes 80/80); oracle_core.cpp l.380-384 | Add a frozen test: with Long range and feedback 6, the ratio of the second to the first echo at 2.5 kHz is at least 6 dB lower than at 300 Hz. Do the same for Chorus range at time 1.0 (4 kHz wet band). Red against M1, green against the oracle. |
| PM1-05 | The 2-pole mode's "four-stage loop intact" (D-020, D-027 item 4) is untested. T-102 runs at resonance 0 and every resonance test uses the default 4-pole, so a separate 2-stage loop passes. | (a) | Medium | low | No 2-pole test at non-zero resonance. | D-020; D-027 item 4; T-102 to T-105; fixture `poles` default 1 | test_m1.cpp T-102 to T-105, T-109; fixture defaults | Add a test: `poles` = 0, resonance 10, cutoff 1 kHz, no input: self-oscillates at the 4-pole frequency within 2 %. Optionally also the 2-pole gain at cutoff for k = 3. |
| PM1-06 | Binary portability floor undecided: no `CMAKE_OSX_DEPLOYMENT_TARGET` (bundles built on `macos-15` require the SDK's macOS version); Linux bundles need Ubuntu 24.04's glibc and libstdc++; MSVC dynamic runtime; README install text omits the macOS quarantine step. Hands-on packages (G-002) and H-1 fail on older systems. | (j), (i), (d) | Medium | medium | A-006 names architectures but no OS-version floor; D-005 omits deployment settings. | A-006; D-005; S-040; S-035 `## Install`; H-1 | DECISIONS.md D-005 property list; ENVIRONMENT.md per-platform lines; Rack `compile.mk` sets `-mmacosx-version-min=10.9` for Rack only | Add a decision: macOS deployment target (for example 11.0 for both architectures), static MSVC runtime (`CMAKE_MSVC_RUNTIME_LIBRARY`), stated Linux floor = build image. `packages.yml` checks `minos` with `otool -l` / `vtool`. README tells the user to remove the quarantine attribute. |
| PM1-07 | Steinberg's VST3 validator is never run: pluginval skips it without `--vst3validator`. A recorded lesson's class of failure (validator-only conformance errors) can recur. | (f), (j) | Medium | high (skip certain); impact medium | Lesson applied to its symptom (program name) rather than its class. | L-20261028T215000Z; D-007; S-021 | E8 log line `Skipping vst3 validator as validator path hasn't been set`; PRIOR_KNOWLEDGE row l.120 | Build Steinberg's `validator` from a pinned `vst3sdk` commit in `vst3-linux` (or one platform). Pass `--vst3validator`. Assert in the log that the validator ran. |
| PM1-08 | Meter values cross threads without a stated mechanism: `core.hpp` makes an `Effect` single-threaded, yet D-016's editor timer reads meters. A literal implementation calls `getMeter()` from the message thread (a data race), and the TSan stress never opens the editor. | (d), (h) | Medium | medium | Interface contract and editor decision not reconciled; no instruction to publish meters through atomics. | `core.hpp` l.8; D-016; S-021; S-ADVERSARIAL | core.hpp l.8; DECISIONS.md D-016 l.119; PLAN.md l.236 (atomics mentioned for parameters only) | D-016: `processBlock` copies `getMeter(k)` into `std::atomic<float>` members after each block, and the editor reads only those. Add an editor-open thread to the S-ADVERSARIAL TSan stress. |
| PM1-09 | Each step pushes twice (step and log commit). Every push runs the full `ci.yml` with six macOS jobs and no cancellation. That exceeds the free-plan macOS concurrency of five, so Done-when waits queue behind log-commit runs. | (g), (i) | Medium | high | Log commits are not exempted from CI; the concurrency group is per SHA. | PLAN.md rules l.7; S-001 action 3 (concurrency) | PLAN.md l.7, l.67; D-009 job table | Put `[skip ci]` in log-commit subjects (GitHub honours it for push and pull_request). Change S-MUTATE's Done-when to use a dedicated docs-only commit for the `not-applicable` check. |
| PM1-10 | `mull_score.py` counts every status other than 2 as killed, including `NotCovered` (8), `DryRun` (6) and `Invalid` (0). Mull itself counts `NotCovered` as not killed. Sub-step 2's survivor-count cross-check cannot detect this. If coverage reporting is enabled (a common speed-up), not-run mutants inflate the score; the impact would then be Critical-class. | (e), (b) | Medium | low | Score script written for one observed status pair; the skill copied into the plan. | S-MUTATE sub-steps 1-2; `plan/protocol/skills/mutation-testing-in-ci/scripts/mull_score.py` l.43; D-060 | E13: `rust/mull-state/src/execution_status.rs`, `rust/mull-tools/reporting.rs` at `b383a52` | Count as killed only statuses 1, 3, 4, 5 (and 7 if fail-fast is used). Fail on any 0, 6 or 8 unless an explicit flag allows them. Add a cross-check against Mull's printed score as well as its survivor count. Forbid `includeNotCovered` in `mull.yml`. |
| PM1-11 | `macos-15-intel` (the only Intel macOS label) ends with the macOS 15 image in autumn 2027. Three jobs then cannot run, `aggregate` is permanently red, and T-905 freezes the job names. No pre-decided response. | (j), (i) | Medium | medium | Label lifetime not recorded with the pin; no waiver path for a declared platform losing its runner. | D-009; A-006; T-905 `CI_JOBS`; S-001 On failure | GitHub changelog (E16) | Record the end-of-life in RISKS.md with a pre-decided response: build x86_64 on `macos-15` via `CMAKE_OSX_ARCHITECTURES=x86_64`, run tests under Rosetta 2 if available, or waive by a D- entry. Allow T-905's job list to be amended by the owner. |
| PM1-12 | Rack's `plugin.mk` writes objects of `../core/src` and `../adapters/src` to `rack/core/src/` and `rack/adapters/src/`. `.gitignore` covers only `rack/build/`, so object and dependency files can be committed to the public repository (built code without G-002) and the tree is dirty after every Rack build. | (d) | Low | medium | `SOURCES` with `../` paths combined with `build/%.o` rules. | D-006; S-001 action 1 (`.gitignore`); S-032 | E11 (`-o build/../core/src/lowpass.cpp.o`) | Add `rack/core/` and `rack/adapters/` to `.gitignore`, or compile the shared sources through a `rack/src/core_unity.cpp` that includes them. |
| PM1-13 | `mutation-baseline.json` holds both `project_floor` and the per-file scores, but `mull_score.py --write-baseline` rewrites the file with per-file scores only, dropping `project_floor`. | (d) | Low | medium | Two schemas in one file. | S-MUTATE "Floor" and sub-step 4 | PLAN.md l.411, l.423; mull_score.py `--write-baseline` | Keep the floor in a separate file or key the per-file map under `"files"`, and adapt the script. |
| PM1-14 | `tailSeconds()` is unchecked for Phaser, RingMod, FilterBank and OscBox (T-852 only checks the VST3 value equals the core's). A wrong short tail makes hosts stop processing a ringing phaser. | (a) | Low | low | T-017 covers three effects. | T-017; S-012, S-013, S-016, S-017 | test_common.cpp T-017 | One test per remaining effect: output below -60 dB after the reported bound with resonance or feedback at maximum. |
| PM1-15 | The T-011/T-855 hooks miss aligned `operator new` overloads and direct `malloc`. SC-8's "no lock" has no test. | (a), (e) | Low | low | Hook covers only the plain overloads. | T-011, T-855; SC-8 | test_common.cpp l.17-24 | Add `align_val_t` overloads to the counter in a future amendment. Keep SC-8's lock clause in S-REVIEW brief 2 (already there). |
| PM1-16 | Prior-knowledge table inconsistencies: the perf-harness lesson's "application" text is unrelated boilerplate; the editor-labels lesson is N/A in the table but cited as applied in S-010 and S-031. | (f) | Low | high | Table rows generated by group. | `research/PRIOR_KNOWLEDGE.md` rows for L-20261101T041800Z and L-20261008T173531Z; S-010, S-031 | PRIOR_KNOWLEDGE.md l.45 and editor-labels row; PLAN.md S-010, S-031 | Rewrite the two rows. Add a benchmark note in D-050 that automation and CV are not exercised. |
| PM1-17 | Container images pinned by tag; `apt`, `brew` and MSYS2 packages float (rolling MinGW GCC for Rack Windows); Mull is "newest at run time" with trust on first use. | (b) | Low | medium | Pin policy covers actions, JUCE, Catch2 and pluginval only. | D-009; ENVIRONMENT.md packages; S-MUTATE pin rule | ENVIRONMENT.md; E13 (0.38.0 released 2026-11-12) | Pin container images by digest. Print and record package versions in each job log. Accept the rest with an R- entry. |
| PM1-18 | After the merge, the weekly mutation schedule is disabled by GitHub after 60 days without repository activity, and while live it opens a new issue on every failure (no deduplication). | (i) | Low | medium | Operations behaviour of scheduled workflows on public repositories not planned for. | S-MUTATE triggers (b) | PLAN.md S-MUTATE triggers | Search for an open issue with the same title before creating one. Tell the owner in COMPLETION.md how to re-enable the schedule. |
| PM1-19 | Core numerics under Rack's flags are tested only with GCC (`rack-linux`). The clang Rack builds on macOS use `-funsafe-math-optimizations` (reassociation, reciprocal maths) with no core test. | (j), (a) | Low | low | Flag test placed on one job. | D-006; S-033 | E2, E3 (GCC passes); `compile.mk` | Also run `muff_core_tests` with Rack's flags in `rack-macos`. |
| PM1-20 | C-PEDA-001 gives the cutoff midpoint as about 500 Hz, but D-020's log law and T-003 freeze 424 Hz. | (c) | Low | medium | Single-source figure not reconciled with the chosen taper. | C-PEDA-001; D-020; T-003 | claims.json C-PEDA-001; test_common.cpp T-003 | List it under D-027 as a design choice, or reconsider the taper through the amendment route if the owner cares. |

## 5. Counts by severity

| Severity | Count | IDs |
|---|---|---|
| Critical | 0 | none |
| High | 2 | PM1-01, PM1-03 |
| Medium | 9 | PM1-02, PM1-04, PM1-05, PM1-06, PM1-07, PM1-08, PM1-09, PM1-10, PM1-11 |
| Low | 9 | PM1-12 to PM1-20 |

Notes on calibration:
- PM1-01 was considered for Critical ("silently wrong results"). It is rated High because the product itself is correct and the error is audible against demo 16.
- PM1-03 is High under the rubric's "a frozen test proves invalid" clause. Its likelihood is low.
- PM1-10 would be Critical-class if coverage reporting were enabled. The plan as written does not enable it, so it is rated Medium.
- Risks already declared and handled in `plan/RISKS.md` were not re-reported: trust on first use of the Rack SDK, R-011's intermittent state restoration, R-012's untested Rack runtime, R-006 aliasing, R-003 patterns and R-004 references.

CRITICAL: 0 HIGH: 2 MEDIUM: 9 LOW: 9

---

## Planner's disposition (added after the round; the text above is the reviewer's report, unedited)

| Item | Severity | Disposition | Where |
|---|---|---|---|
| PM1-01 | High | Fixed: the demo renderer drives the effect through `BlockRunner` as a mono or stereo host would, so mono demos of the filter bank carry all eight filters; self-check added. | S-LISTEN |
| PM1-02 | Medium | Fixed: `--random-seed`; the pass rule checks the seed line of the log. | D-007, S-021 |
| PM1-03 | High | Fixed in the frozen test: T-304 counts cycles for the 250 Hz case. Re-frozen; red and green re-verified. | D-063, `research/spikes/red-verification/RESULT.md` |
| PM1-04 | Medium | Fixed with new frozen tests T-410 and T-708; the reviewer's variant now fails exactly those two. | D-063 |
| PM1-05 | Medium | Fixed with new frozen test T-110. | D-063 |
| PM1-06 | Medium | Fixed: deployment target, stated floors, install notes, `minos` check. | D-071, S-021, S-035, S-040 |
| PM1-07 | Medium | Accepted as a risk with a pre-decided response. | R-016 |
| PM1-08 | Medium | Fixed: meters cross threads through atomics; the stress test opens the editor. | D-016, S-021, S-ADVERSARIAL |
| PM1-09 | Medium | Fixed: log commits carry `[skip ci]`; the not-applicable check uses its own commit. | PLAN rules, S-MUTATE |
| PM1-10 | Medium | Fixed: score rule counts only killed statuses, fails on others, and is cross-checked against Mull's own score. | S-MUTATE |
| PM1-11 | Medium | Accepted as an upkeep risk with a pre-decided response; reported to the owner. | R-017 |
| PM1-12 | Low | Fixed: `.gitignore`. | S-001 |
| PM1-13 | Low | Fixed: the floor has its own file. | S-MUTATE |
| PM1-14, PM1-15 | Low | Accepted. | R-019 |
| PM1-16 | Low | Fixed: the two rows rewritten. | `research/PRIOR_KNOWLEDGE.md` |
| PM1-17 | Low | Accepted. | R-018 |
| PM1-18 | Low | Fixed: issue de-duplication; note for the owner. | S-MUTATE, S-FINAL |
| PM1-19 | Low | Fixed: `rack-macos` also runs the core tests with Rack's flags. | S-033 |
| PM1-20 | Low | Recorded as a design choice. | D-027 item 12 |

Result of round 1: 0 Critical, 2 High, both fixed. Because the plan and the frozen tests changed, a cold read of the changed parts and a second pre-mortem round follow (protocol Phase 4, items 4 and 5).
