# Pre-mortem, round 3: mufferfluffer plan (branch `gen-20261111T151412Z-mufferfluffer-vst-vcv`, head `736de5a`)

Reviewer: fresh-context agent (Claude Opus 5.5). I did not write the plan. Date: 2026-11-13. I edited, committed and pushed nothing in `/home/claude/pm3`. All builds and variants were made in `/tmp/pm3work`.

## 1. Method and what was executed

I read `HANDOFF.md`, `plan/ASSUMPTIONS.md`, `plan/DECISIONS.md`, `plan/ENVIRONMENT.md`, `plan/PLAN.md` (all steps), `plan/RISKS.md`, the relevant rows of `plan/TRACEABILITY.md`, and the four frozen headers. I read every new or amended frozen test: T-308, T-411, T-508, T-509, T-857 and `tests/py/test_repo.py` in full. I also read the neighbouring tests for ring modulator, oscillator, tap tempo, CV, bypass, smoothing, state and the Rack bridge (T-010, T-012 to T-014, T-022, T-201 to T-206, T-406, T-407, T-506, T-602 to T-606, T-706, T-801, T-802, T-811, T-812, T-852 to T-854). Beyond those I read the fixture's tables, the oracle core and the VST3 oracle shell, `research/spikes/red-verification/RESULT.md`, `research/PRIOR_KNOWLEDGE.md`, the claims named below, and the disposition tables of rounds 1 and 2.

| # | What was run | Result |
|---|---|---|
| Y1 | Oracle core and the frozen tests, Release, GCC 13.3 | `All tests passed (299647 assertions in 87 test cases)` |
| Y2 | Oracle built with Rack's x64 flags (`-O3 -march=nehalem -funsafe-math-optimizations -fno-omit-frame-pointer`) | 87 of 87 pass |
| Y3 | Per-tag case counts read from the `TEST_CASE` lines | lowpass 10, ringmod 6, phaser 8, delay 11, chorus 8, filterbank 11 (T-023, T-024, T-501..T-509), oscbox 7, total 87; VST3 7; T-011 is the one case left out under `MUFF_NO_ALLOC_HOOK`, so the sanitizer build has 86 |
| Y4 | `python3 -m unittest discover -s tests/py` on the planning tree | `Ran 9 tests`, `FAILED (failures=9)`, as S-000 expects. The S-035 `-k` selection counts 8 tests, as stated |
| Y5 | `python3 tools/check_frozen.py` | `frozen files intact: 21 files` |
| Y6 | `git ls-remote` of every pinned tag (peeled where annotated) | All nine pins equal their tag's commit (table in section 2) |
| Y7 | Download of the three pluginval 1.0.4 archives from GitHub release assets, `sha256sum`, archive listing | All three hashes equal `plan/ENVIRONMENT.md`. Layout: Linux `pluginval`; Windows `pluginval.exe`; macOS `pluginval.app/Contents/MacOS/pluginval`. No symlinks, so Python's `zipfile` unpacks it intact |
| Y8 | Rack SDK archive URLs | 403 from this sandbox's proxy, so not checked. `compile.mk` at Rack tag v2.6.6 read: `-march=armv8-a+fp+simd` on arm64, `-march=nehalem` on x64, `-mmacosx-version-min=10.9` |
| Y9 | Variant `floatlfo`: LFO phase accumulator in `float` instead of `double` (D-015 says "phase accumulator" and does not specify precision) | 86 of 87. **T-308 fails** at `REQUIRE (lfo[top] > 0.5)` with `-0.55999893f > 0.5`. No depth assertion is reached |
| Y10 | Variant `rateperturb2`: Phaser LFO rate × (1 − 3e-7), i.e. about five float ulps | `[phaser]` 7 of 8: **T-308 fails** the same way. The variant × (1 + 3e-7) passes |
| Y11 | Variant `ratefloat`: Phaser rate computed in `float` (`0.01f * powf (250.0f, n)`) | passes |
| Y12 | Variant `lfopost`: every LFO returns its value before advancing the phase (first sample at phase 0) | 87 of 87 |
| Y13 | Variant `ph1oct` (Phaser LFO depth ±1 octave) and `fb02` (FilterBank depth 0.2 octave) | fails exactly T-308 and T-508 respectively, as `RESULT.md` states |
| Y14 | Variant `fbdur`: FilterBank attack read as "reach 1 in `attack` steps from the present value" (slope `(1 − v0)/attack`) instead of the oracle's fixed slope `1/attack` | `[filterbank]` 11 of 11: both readings pass |
| Y15 | T-308 with `top` chosen as the signed maximum of `lfo_out` instead of `argPeak` (which takes the maximum of the absolute value), run against Y9, Y10, `ph1oct` and the oracle | `floatlfo` and `rateperturb2` pass; `ph1oct` still fails; the oracle passes |
| Y16 | Shard test times, oracle: Release and `-O0` | `[filterbank]` 9.7 s and 19.3 s; `[phaser]` 3.8 s and 6.6 s; `[oscbox]` 2.7 s and 3.9 s; `[delay]` 1.1 s and 2.8 s |
| Y17 | `git ls-remote --tags` of `mull-project/mull`; sparse clone of tag 0.38.0 (`CHANGELOG.md`, `.github/workflows`) | 0.38.0 was released on 2026-11-12, one day after planning. Its Ubuntu 24.04 CI matrix still builds LLVM 19. It adds a second package per LLVM version, `mull-extras-N` |
| Y18 | `grep` of the tracked product files for T-901's forbidden pattern | no hits |
| Y19 | Newest tag of every pinned repository | all at their newest tags, except pluginval: `v2.0.0-beta3` exists, a pre-release, so not newer for the pin |

Not executed: JUCE builds (T-857 was judged by reading it and `RESULT.md`); ThreadSanitizer (PM3-02 is judged by reading the code); Rack SDK archives and Mull release assets (blocked by the proxy).

## 2. Verification of round 2 fixes

| Item | Disposition | Check made | Holds? |
|---|---|---|---|
| PM2-01 msys2 pin | Fixed; `tools/check_pins.py` | Y6: `msys2/setup-msys2` v2.33.0 = `ec48f7c5447b…`. The other eight pins also match, using the peeled commit for the annotated tags of pluginval and Catch2. S-001 3a queries both `refs/tags/<tag>` and `^{}` and accepts either commit, which is right for annotated tags. | Yes. The knowledge item was corrected in place rather than superseded (PM3-09, Low). |
| PM2-02 Phaser depth | Fixed, T-308 | Y13: the one-octave variant fails T-308. **But** T-308 finds the LFO's top with `argPeak`, which takes the maximum of the absolute value. With 7 s of a 6.32 s triangle, the run holds a +1 peak at about 3.16 s and a −1 trough at about 6.32 s. Which one `argPeak` picks depends on sub-sample phase: 0.3 against 0.4 of a sample for the oracle, which is 0.033 samples, or 2.2e-7 in relative rate, from flipping. Y9 and Y10 flip it. | **No**: PM3-01 (High). |
| PM2-03 header DSP outside mutation | Fixed | S-014 now requires `clocked_line.cpp` and declaration-only private headers. S-MUTATE adds a `clocked_line.cpp` shard with `"[delay],[chorus]"`, and its `scope` job selects `full` on any header change. | Yes |
| PM2-04 arm64 Rack flags | Fixed | Y8: the flags in S-033 equal `compile.mk` at v2.6.6 for both architectures. | Yes |
| PM2-05 `[skip ci]` heads | Fixed | S-FINAL pushes an empty `final:` commit without `[skip ci]`, and S-KNOW's last commit carries none. **But** S-FINAL's Done-when still demands `mutation-gate` in `full` or `incremental` mode "on that commit", the empty head commit, where the push-triggered `scope` reports `not-applicable`. | Partly: PM3-03 (Medium). |
| PM2-06 other laws | Fixed, T-411, T-508, T-509 | T-411: by reading, the bursts at eight offsets cover a whole 2 s cycle, so the result does not depend on the LFO's start phase. I worked through the clocked line: the arrival time lies between 63 + 0.75s ms and 252 − 3s ms for transits that span an edge (s is the time from entry to the edge), so the extremes are the settled values. T-508: the run covers 13 s of a 12.5 s cycle, so it also does not depend on start phase; it accepts depths of about 0.41 to 0.57 octave. T-509 is robust to Y12. It does not decide between two readings of the rise law (Y14; PM3-05, Low). | Yes |
| PM2-07 VST3 mono | Fixed, T-857 | Read: it compares mono→mono with `BlockRunner` at `numOut = 1` for all seven effects, within 1e-6, and requires energy above 1e-3. `RESULT.md` records the two-output shell failing it. | Yes (comment line 1 of `test_vst3.cpp` still says "T-851 .. T-856": cosmetic) |
| PM2-08 tail across threads | Fixed, atomic | D-016 now sets the atomic from `effect.tailSeconds()` at the end of `setStateInformation` as well. That function runs on the message thread, while `processBlock` writes the effect's parameters on the audio thread. In the oracle, `tailSeconds()` of four effects reads those parameters. | **Partly**: the race moved; PM3-02 (Medium). |
| PM2-09 Time CV | Recorded | C-PEDB-020 `contradicted_by` filled; D-027 item 13 present. | Yes |
| PM2-10, -13, -14 | Accepted, R-019 | R-019 names all three. | Yes |
| PM2-11 `aggregate` | Fixed in T-905 | The block regex captures lines indented four or more spaces up to the next two-space key, and requires a line `    if: always()` at exactly job level. A step-level `if:` (six or more spaces) or a comment cannot satisfy it. It rejects `${{ always() }}` and a trailing comment, which is stricter than needed but fails visibly. | Yes |
| PM2-12 citations | Fixed | S-016 cites C-PEDB-031 to C-PEDB-033; D-027 item 14 present. | Yes |

**Counts.** 87 core cases, 86 under the sanitizer build, 7 VST3 and 9 Python are consistent in `HANDOFF.md` §4, PLAN line 3, S-000, S-018, S-020, S-REVIEW, S-FINAL, D-006 and `RESULT.md`. The per-tag counts in S-011 to S-017 match Y3. `[filterbank]`'s 11 includes T-023 and T-024, as S-016 says.

**Pins.** All nine equal their tags (Y6). The three pluginval hashes are now verified against real downloads for the first time (Y7).

## 3. Incident reports

### PM3-01: T-308 rejects conforming Phaser implementations (High)
*Six months on.* At S-013 the implementer wrote the Phaser LFO with a `float` phase accumulator, a common audio idiom. D-015 says only "phase accumulator". T-308 failed with `-0.56 > 0.5`, an assertion about locating the LFO's peak, not about depth. The implementer spent attempts on a correct depth law. Then either a `TEST_CHALLENGE.md` with a re-freeze, or a switch to `double` "because the oracle does". A second implementation computed the rate through a `float` log taper and passed on Linux but not on one other libm: the margin is about five float ulps (Y10), so the platform matters.

*Root cause.* `ts::argPeak` returns the index of the largest magnitude. The test needs the largest signed value. The run holds both a +1 peak and a −1 trough of nearly equal magnitude. Which wins depends on where the samples fall relative to each, which depends on the implementation's arithmetic. This is the same class as PM1-03 (T-304 passing only on the oracle's sampling grid), now in a round 2 addition.

*Evidence.* Y9, Y10 (fail); Y11, Y12 (pass); Y15 (the fix).

*Mitigation.* Re-freeze T-308 with the signed maximum: `long top = lead; for (i = lead; i < n - len; ++i) if (lfo[i] > lfo[top]) top = i;`. With that change, Y9 and Y10 pass and the depth-defect variant still fails (Y15). Record a recurrence of the PM1-03 lesson: a frozen test must not choose between two extremes of nearly equal size by `argPeak`.

### PM3-02: the tail-length atomic is written from `effect.tailSeconds()` on the message thread (Medium)
*Six months on.* At S-ADVERSARIAL, ThreadSanitizer reports a race between `setStateInformation` (stress thread) and `processBlock` (audio thread), with frames in `vst3/src/processor.cpp`. To fix it, the implementer must depart from the text of D-016. A host that restores state during playback reads a torn or stale value. In practice that is harmless numerically, but it is undefined behaviour, and it is exactly what PM2-08 set out to remove.

*Root cause.* PM2-08's fix moved the call instead of removing it. `tailSeconds()` depends on parameter values that `processBlock` writes through `Effect::setParam`, and D-016 states that the `Effect` belongs to the audio thread.

*Evidence.* D-016 text. The oracle's `tailSeconds()` reads the effect's parameter targets for Lowpass, Phaser, Delay and Chorus. S-ADVERSARIAL's stress calls `setStateInformation` concurrently with `processBlock`.

*Mitigation.* D-016: set the atomic only at the end of `prepareToPlay` (no audio running) and of every `processBlock`. `setStateInformation` does not touch the effect. T-852 is unaffected.

### PM3-03: S-FINAL's Done-when cannot be met as written (Medium)
*Six months on.* The `final:` commit is empty. Its push-triggered `mutation.yml` run reports `mode: not-applicable`, but the Done-when line requires `full` or `incremental` "on that commit". The step's own Actions say to read the newest commit whose mode was not `not-applicable`. The two lines disagree. The only way to satisfy the Done-when literally is the `pull_request` run that the new PR starts. Its `scope` compares against a base that the plan never defines. If the implementer diffs the PR against `main`, every core file "changed", so each later push to the PR (the final log commit, S-KNOW's last commit) starts a full mutation run of up to 6 h, and S-FINAL waits for one.

*Root cause.* PM2-05's fix changed the head commit to an empty one without revisiting the mutation clause, and `scope` semantics for `pull_request` events are unstated.

*Mitigation.* Change S-FINAL's Done-when to "`mutation-gate` green in `full` or `incremental` mode on the newest commit that changed a file under `core/`, `adapters/`, `tests/` or the mutation set-up, and green (any mode) on the head commit". State in S-MUTATE that `scope` diffs `github.event.before..github.sha` on push and the PR's head against its previous head on `pull_request` (or skip `pull_request` in `mutation.yml` while the PR is open).

### PM3-04: a documented tap function is dropped without appearing on the owner-visible list (Medium)
On the Delay pedal, holding the tap switch retargets tap tempo from delay time to LFO rate (C-PEDB-017, corroborated). A-007 puts every documented front-panel control in scope. D-014 gives the Delay's tap one target only, and D-027 item 11 ("not modelled anywhere") does not mention the retarget. At G-LISTEN the owner, who knows the hardware, finds that tapping cannot set the Delay's LFO; nothing in the Phase 5 report warned them.

*Mitigation.* Either add it to D-027 item 11 (documentation; workaround: the LFO Rate knob and CV), or model it (a held `tap` over 1 s toggles the target), which is a new decision and test.

### PM3-05 to PM3-09 (Low)
See the risk table.

## 4. Lens-by-lens notes

**(a) Technical correctness.**
- New tests: T-411, T-508, T-509 and T-857 are sound and do not depend on the reference's start phase or LFO timing (Y12; reasoning in section 2). T-308 does depend on them (PM3-01).
- Areas not probed in earlier rounds:
  - Ring modulator: T-201..T-206 pin the sideband levels, both ranges, the LFO range and shape, the peak-to-peak law, the external carrier and the dry path at Mix 0. D-021 gives no Mix law between the ends (PM3-07).
  - Oscillator: tracking, envelope, FM, sync and band-limiting are pinned (T-602..T-607). D-017's smoothing wording does not say whether the volt-per-octave input is smoothed (PM3-07).
  - Tap tempo: T-022, T-406, T-506 and T-706 cover three taps, refinement, idle reset, knob takeover and range limits. In the VST3, a tap is a parameter read once per block, so a click shorter than one block can be missed. Ordinary clicks of 80 ms or more are not affected; not raised.
  - CV and trigger: T-012 and T-013 pin the additive law, clamping, the Wave depth of 0.6 and disconnected inputs; T-812 pins the trigger levels.
  - Bypass and spillover: T-010 and T-407 cover them. The oracle keeps recirculating echoes in bypass with spillover off, which is consistent with D-018.
  - Smoothing: T-014. Drive and output: T-010, T-014, T-108.
  - State: T-853's hostile cases (empty, junk, truncated, wrong tag, out-of-range, unknown id, non-numeric) match the contract.
  - Rack bridge: T-811 and T-812 pin scaling, the mono-sum rule and limits.
  - Python tests: T-901 has no false positives on the current tree (Y18). The T-905 regexes behave as section 2 says. The test counts per `-k` selection are right.

**(b) Supply chain.**
- Pins verified (Y6) and pluginval hashes verified (Y7).
- Mull 0.38.0 appeared on 2026-11-12; the plan's "newest release with an LLVM 19 asset" rule handles that. It now ships two `.deb` files per LLVM version, and S-MUTATE speaks of "an asset" (PM3-08).
- Rack SDK archives remain trust on first use (R-018), unchanged.

**(c) Research assumptions.**
- Phaser Amount = ±3 octaves rests on the one-source phrase "full sweep range" (C-PEDA-041, single-source) and is now frozen by T-308.
- RingMod Amount as peak-to-peak rests on C-PEDA-028 (single-source).
- The FilterBank retrigger law rests on no hardware source.
- None of the three is on D-027's owner-visible list (PM3-06).
- The tap retarget of C-PEDB-017 is dropped silently (PM3-04).

**(d) Implementer misinterpretation.** The D-024 rise law (PM3-05); volt-per-octave smoothing and the RingMod Mix law (PM3-07); S-FINAL's Done-when (PM3-03).

**(e) Integrity.**
- The red and green variants in `RESULT.md` reproduce (Y13).
- The claim in D-063 that each new test fails "against no other variant" holds only for the planner's own variant set; Y9 is a conforming variant that T-308 fails.
- The knowledge item for C-PLANNER-009 was rewritten in place (PM3-09).

**(f) Past lessons.**
- PM3-01 recurs PM1-03's class: a test whose verdict depends on the reference's sampling lattice.
- PM3-02 and PM3-03 recur the pattern noted in round 2, where a fix to one place leaves its neighbours unrevisited (`L-20261004T150500Z-cross-library-float-fix-regressed-passing-cases` is the nearest stored lesson).

**(g) Performance and CI time.**
- The `[filterbank]` shard takes 9.7 s Release and 19 s at `-O0` (Y16), above the plan's estimate of 1 to 4 s Release. At about 150 mutants, 25 s each and 4 workers, that is roughly 16 min, inside the 90 min budget. Not raised; sub-step 1 measures it.
- After the PR opens, `ci.yml` runs twice per push (push and `pull_request`). That costs time only.
- PM3-03 notes the risk of repeated full mutation runs.

**(h) Security.**
- No new exposure. `check_pins.py` runs `git ls-remote` with arguments taken from committed workflow files, under a read-only token.
- PM3-02 is a data race (undefined behaviour) but is not attacker-reachable beyond state loading, and TSan would catch it.

**(i) Operations after hand-over.**
- PM3-03 (repeated full mutation runs on PR pushes).
- Otherwise as R-017, R-018 and R-019.

**(j) CI platform coverage.**
- Every job maps to an A-006 platform, and the Rack flags per architecture are now right.
- Rack's `compile.mk` sets `-mmacosx-version-min=10.9` on x64. The core must then avoid C++17 library features that Apple gates above 10.9: `std::optional::value`, `std::variant`, aligned `new`, `std::filesystem`. The frozen headers use none of them. An implementation that did would fail `rack-macos-intel` visibly, and S-033's fix-forward covers that. Not raised.

## 5. Risk table

| ID | Description | Lens | Severity | Likelihood | Root cause | Trace | Evidence checked | Proposed mitigation |
|---|---|---|---|---|---|---|---|---|
| PM3-01 | T-308 locates the LFO's top with `argPeak` (maximum absolute value). The +1 peak and −1 trough in its window differ by a fraction of a sample, so a conforming Phaser fails `lfo[top] > 0.5`: a `float` phase accumulator does, and so does a rate 3e-7 low (about five float ulps). The test then never reaches its depth assertion. A frozen test that rejects implementations following D-015 and D-022 is invalid. | (a), (d), (f) | **High** | medium | Signed maximum needed, magnitude used. The oracle sits 0.033 samples from the flip. Recurrence of PM1-03's class. | T-308; D-015, D-022; S-013 | Y9, Y10 (fail); Y11, Y12 (pass) | Re-freeze with a signed argmax (Y15: legitimate variants pass, the depth defect still fails). Lesson recurrence. |
| PM3-02 | D-016 sets the tail atomic from `effect.tailSeconds()` at the end of `setStateInformation` (message thread) while `processBlock` writes the effect's parameters (audio thread): the PM2-08 race moved rather than removed. S-ADVERSARIAL's TSan stress will flag it and force a departure from D-016. | (a), (d), (h) | Medium | high (by text) | Fix applied to one call site; effect ownership rule not re-read. | D-016; S-ADVERSARIAL; `vst3_shell.hpp` | Oracle `tailSeconds()` reads parameter targets for 4 effects | Update the atomic only in `prepareToPlay` and `processBlock`. |
| PM3-03 | S-FINAL's Done-when requires `mutation-gate` in `full` or `incremental` mode on the empty head commit, where a push run is `not-applicable`. The step's Actions say otherwise. The `pull_request` run that could satisfy it has an undefined `scope` base. A PR-base diff makes every later push a full run of up to 6 h. | (d), (i) | Medium | high | PM2-05 fix not carried into the Done-when; `scope` for `pull_request` unspecified. | S-FINAL; S-MUTATE triggers | PLAN l.509, l.512, l.417 | Reword the Done-when as in section 3. Define `scope`'s diff base per event, or omit `pull_request` from `mutation.yml`. |
| PM3-04 | The Delay hardware's tap switch, held about 1 s, retargets tap from delay time to LFO rate (C-PEDB-017, corroborated). This is in scope by A-007, not modelled (D-014), and not in D-027 item 11. | (c) | Medium | high | Single target chosen; omission not listed. | D-014, D-027; A-007; C-PEDB-017 | claims.json | Add it to D-027 item 11 for the owner (workaround: LFO Rate knob and CV), or decide and test a hold-to-retarget. |
| PM3-05 | D-024 says the envelope rises from the present value "to 1 in `attack` steps" (fixed duration), but the oracle uses a fixed slope `1/attack`. T-509 and every other test accept both (Y14). They differ audibly with dense patterns and long attacks. | (d) | Low | medium | Ambiguous wording; test bounds wide. | D-024; T-509 | Y14 | Say "at the rate of a full rise in `attack` steps" in D-024. |
| PM3-06 | Inferred readings frozen by tests are missing from the owner-visible list: Phaser Amount 1 = ±3 octaves (C-PEDA-041 "full sweep range", single-source); RingMod Amount = peak-to-peak octaves (C-PEDA-028, single-source); the FilterBank retrigger law (no source). | (c), (e) | Low | high | D-027 lists constants, not interpretations. | D-021, D-022, D-024, D-027; T-204, T-308, T-509 | claims.json | Add three lines to D-027. |
| PM3-07 | Unspecified small laws: whether the OscBox volt-per-octave input is smoothed (D-017 speaks of normalised positions only; a 5 ms smoother would remove audio-rate exponential FM through the Rack jack and add glide), and the RingMod Mix law between 0 and 1 (D-021 has none; T-201 and T-206 pin only the ends; demo 05 uses 0.5). | (a), (d) | Low | low | Decision wording. | D-017, D-021, D-025; T-201, T-206, T-602 | Oracle uses raw V/oct and linear mix | One sentence each in D-017 and D-021. |
| PM3-08 | Mull 0.38.0 (2026-11-12) is now the newest release and ships `Mull-19` and `Mull-Extras-19` packages for Ubuntu 24.04. S-MUTATE.1 records "an asset", so a loose pattern may select the extras package, which lacks the runner. | (b) | Low | low | Upstream packaging changed after planning. | S-MUTATE.1; D-060; R-010 | Y17 | Name the selection rule: the asset whose name starts `Mull-19-` and contains `ubuntu-24.04` (not `Mull-Extras-`). |
| PM3-09 | The C-PLANNER-009 knowledge item was corrected in place, with a note and `supersedes: []`, rather than by a superseding item (Rule 12). The "earlier note" that supplied the wrong hash (lesson L-20261113T093000Z) is not identified. If it lives in the knowledge store, it stays uncorrected for the next run. | (e), (f) | Low | low | Correction made in the working copy only. | Rule 12; K-…-c-planner-009 | knowledge file l.5, l.21 | Identify the source note. If it is a store item, add a superseding item for S-KNOW to push. |

## 6. Counts by severity

| Severity | Count | IDs |
|---|---|---|
| Critical | 0 | none |
| High | 1 | PM3-01 |
| Medium | 3 | PM3-02, PM3-03, PM3-04 |
| Low | 5 | PM3-05, PM3-06, PM3-07, PM3-08, PM3-09 |

**Calibration notes.**
- **PM3-01** is High under "a frozen test proves invalid": I built a conforming variant and it fails. Likelihood is medium, not high, because the oracle's `double` accumulator is the obvious model to copy.
- **PM3-03** is Medium rather than High: the step's Actions say what to read, so the implementer is not left to guess, only to log a deviation.
- **PM3-04** is Medium because the missing function has a workaround and the defect is chiefly that the owner is not told.

**Round 2 fixes that do not hold:**
- PM2-02 (T-308 is brittle: PM3-01).
- PM2-08 holds only in part (the race moved to `setStateInformation`: PM3-02).
- PM2-05 holds for CI on the head, but its Done-when contradicts its Actions (PM3-03).
- All others hold.

Diminishing returns: apart from PM3-01, nothing found in this round would make the delivered effects wrong. The remaining items are wording, owner-visible lists and one thread-safety slip that the planned TSan stress would catch.

CRITICAL: 0 HIGH: 1 MEDIUM: 3 LOW: 5

---

## Planner's disposition (added after the round; the text above is the reviewer's report, unedited)

| Item | Severity | Disposition | Where |
|---|---|---|---|
| PM3-01 | High | Fixed in the frozen test: T-308 uses the signed maximum. Re-frozen; the depth variant still fails it. | `tests/core/test_m1.cpp`, `research/spikes/red-verification/RESULT.md` |
| PM3-02 | Medium | Fixed: the tail value is refreshed only in `prepareToPlay` and `processBlock`. | D-016 |
| PM3-03 | Medium | Fixed: S-FINAL names the mutation commit; the mutation workflow has no `pull_request` trigger and its comparison base is stated. | S-FINAL, S-MUTATE |
| PM3-04 | Medium | Recorded as a design choice shown to the owner. | D-027 item 15 |
| PM3-05 | Low | Fixed: the rise law is stated (same slope). | D-024 |
| PM3-06 | Low | Recorded. | D-027 item 16 |
| PM3-07 | Low | Fixed: one sentence each. | D-017, D-021 |
| PM3-08 | Low | Fixed: asset name rule. | S-MUTATE |
| PM3-09 | Low | Checked: neither store holds the wrong hash; the lesson says so. The corrected knowledge item was never pushed in its wrong form, so nothing is superseded. | lesson `L-20261113T093000Z` |

Result of round 3: 0 Critical, 1 High, fixed. A fourth round follows because a High item was found.
