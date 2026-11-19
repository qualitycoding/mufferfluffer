# Mutation testing (S-MUTATE)

Tool and set-up: `.github/workflows/mutation.yml`, `mull.yml`, `tools/mull_shards.py` (scope and shard matrix), `tools/mull_release.py`
(chooses, records and verifies the two Mull release packages), `tools/mull_score.py` (score from the SQLite reports),
`tools/mull_include_paths.py`. Decision D-060; scope `core/src/*.cpp` and `adapters/src/*.cpp`.

## Installed Mull

Release **0.39.0** of the Mull project, for LLVM 19 on `ubuntu-24.04` (`clang-19` and `libclang-rt-19-dev` from the Ubuntu archive). The
release listing offered, among others, the two packages below; `tools/mull_release.py` chose them by exact file-name pattern and
recorded them under `mull` in `deps.lock.json` (trust on first use, run 44441090332); every later run verifies them against that record.

| Role | Package | SHA-256 |
|---|---|---|
| runner | `Mull-0.39.0-ubuntu-amd64-24.04.deb` | `dfae322fcb6363741de1b3fa2b3e948796892accda63da3ffc29f8f43334722a` |
| IR plug-in for LLVM 19 | `Mull-19-0.39.0-LLVM-19.1.1-ubuntu-amd64-24.04.deb` | `39097dc27184b894c668141deac2fb364b938eea8ab7d5031685524188231fb5` |

After installation the runner is `/usr/bin/mull-runner` (also `mull-runner-19`) and the plug-in `/usr/lib/mull-ir-frontend-19`. Mull 0.39 prints
no score line: its console output ends with `Survived mutants (N/TOTAL)`, `Mutation score: NN%` (rounded) and `Surviving mutants: N`, and a progress bar
whose last figure is the mutant count. `tools/mull_score.py --compare-log` therefore compares the surviving count, the total and the derived score. Its
first version required a score line and failed all eleven shards of the first run (`FAIL: ... no mutation score or surviving-mutant count found`); that was my
unverified assumption about the log format, and the fix is in commit db2ef4a.

## Measuring run (sub-step 1)

Runs: https://github.com/qualitycoding/mufferfluffer/actions/runs/44441090332 (first full run, every shard failed only the log comparison above) and
https://github.com/qualitycoding/mufferfluffer/actions/runs/44447284584 (second, gate green). Figures below are from the second. Build: Debug, `-O0`, with the Mull plug-in;
four workers; per-mutant timeout 10 times the shard's baseline, at least 30 s.

| Shard | Files | Mutants | Killed | Survived | Score | Baseline test time | Per-mutant timeout | Mull run | Job time |
|---|---|---|---|---|---|---|---|---|---|
| lowpass | lowpass.cpp | 42 | 29 | 13 | 69.0 % | 10.4 s | 103.5 s | 2 min 36 s | 4 min |
| ringmod | ringmod.cpp | 21 | 19 | 2 | 90.5 % | 1.0 s | 30 s | 8 s | 8 min |
| phaser | phaser.cpp | 43 | 25 | 18 | 58.1 % | 13.6 s | 136.1 s | 3 min 35 s | 28 min |
| delay | delay.cpp | 39 | 22 | 17 | 56.4 % | 19.5 s | 194.9 s | 8 min 58 s | 10 min |
| chorus | chorus.cpp | 36 | 26 | 10 | 72.2 % | 3.3 s | 33.2 s | 52 s | 24 min |
| filterbank | filterbank.cpp | 66 | 52 | 14 | 78.8 % | 64.0 s | 639.7 s | 30 min 40 s | 56 min |
| oscbox | oscbox.cpp | 60 | 45 | 15 | 75.0 % | 9.8 s | 97.6 s | 3 min 14 s | 29 min |
| clocked_line | clocked_line.cpp | 28 | 17 | 11 | 60.7 % | 13.9 s | 139.4 s | 3 min 52 s | 27 min |
| dsp_common | dsp_common.cpp | 32 | 25 | 7 | 78.1 % | 103.4 s | 1034.2 s | 22 min 45 s | 50 min |
| shared | tables.cpp, blocks.cpp, effect_base.cpp, create.cpp | 166 | 106 | 60 | 63.9 % | 24.0 s | 239.9 s | 34 min 33 s | 36 min |
| adapters | adapters.cpp | 39 | 33 | 6 | 84.6 % | 2.6 s | 30 s | 38 s | 2 min |

Project score **399 of 572 killed (69.8 %)**; the first run gave 400 of 572 (69.9 %): one mutant of the 572 flipped between runs (cause not investigated; it is within the 2-point per-file tolerance of the baseline). The longest job took 56 min (filterbank), so no shard needs splitting (limit 90 min). No shard has fewer than 5 mutants,
so nothing is merged into `shared`; `create.cpp` has no mutants of its own (its report rows are absent). Per-file scores, lowest first:

| File | Killed / total | Survived | Score |
|---|---|---|---|
| effect_base.cpp | 46 / 85 | 39 | 54.1 % |
| delay.cpp | 22 / 39 | 17 | 56.4 % |
| phaser.cpp | 25 / 43 | 18 | 58.1 % |
| clocked_line.cpp | 17 / 28 | 11 | 60.7 % |
| tables.cpp | 29 / 47 | 18 | 61.7 % |
| lowpass.cpp | 29 / 42 | 13 | 69.0 % |
| chorus.cpp | 26 / 36 | 10 | 72.2 % |
| oscbox.cpp | 45 / 60 | 15 | 75.0 % |
| dsp_common.cpp | 25 / 32 | 7 | 78.1 % |
| filterbank.cpp | 52 / 66 | 14 | 78.8 % |
| adapters.cpp | 33 / 39 | 6 | 84.6 % |
| ringmod.cpp | 19 / 21 | 2 | 90.5 % |
| blocks.cpp | 31 / 34 | 3 | 91.2 % |

## Failing gate and agreement with Mull (sub-step 2)

Agreement: for every shard of run 44447284584 the log check printed `log check: agrees within 0.5 points`: the report's surviving count, mutant total and score
equal those of Mull's own output (adapters: Mull `Survived mutants (6/39)`, `Mutation score: 84%`; script 6 survivors, 33/39 = 84.62 %).

The `--floor 99` run and the not-covered report are produced by the step "Show that the scoring gate can fail" of `mutation.yml` on the real adapters report; the output is recorded
below once the run that introduces that step has finished.

## Tests omitted from each shard (D-060)

Each shard runs only the tests whose tags it names, so a mutant can only be killed by a test in its shard's filter. `muff_core_tests --list-tests "<filter>"` gives (98 cases in all):

| Shard | Filter | Tests run | Tests omitted | Tests run (ids) |
|---|---|---|---|---|
| lowpass | `[lowpass]` | 10 | 88 | T-107 T-105 T-110 T-106 T-102 T-109 T-103 T-104 T-108 T-101 |
| ringmod | `[ringmod]` | 6 | 92 | T-202 T-201 T-206 T-204 T-205 T-203 |
| phaser | `[phaser]` | 9 | 89 | T-306 T-308 T-304 T-305 T-309 T-302 T-303 T-301 T-307 |
| delay | `[delay]` | 15 | 83 | T-405 T-415 T-403 T-411 T-404 T-412 T-408 T-410 T-406 T-409 T-414 T-401 T-413 T-402 T-407 |
| chorus | `[chorus]` | 8 | 90 | T-703 T-701 T-705 T-706 T-704 T-702 T-707 T-708 |
| filterbank | `[filterbank]` | 11 | 87 | T-506 T-509 T-024 T-502 T-501 T-023 T-504 T-508 T-503 T-507 T-505 |
| oscbox | `[oscbox]` | 7 | 91 | T-602 T-603 T-604 T-606 T-605 T-607 T-601 |
| clocked_line | `[delay],[chorus]` | 23 | 75 | T-707 T-413 T-409 T-401 T-708 T-404 T-706 T-408 T-703 T-414 T-402 T-705 T-702 T-411 T-701 T-405 T-407 T-704 T-415 T-412 T-410 T-403 T-406 |
| adapters | `[adapters]` | 5 | 93 | T-802 T-801 T-811 T-821 T-812 |
| shared | `[tables],[params],[blocks],[lifecycle],[cv],[bypass],[smoothing],[meters]` | 25 | 73 | T-004 T-014 T-015 T-013 T-001 T-002 T-012 T-031 T-407 T-028 T-024 T-020 T-019 T-030 T-022 T-003 T-008 T-021 T-023 T-006 T-602 T-029 T-010 T-027 T-005 |

Tests that no shard runs: T-007 (finite and bounded outputs, any parameters), T-009 (silence in, silence out), T-011 (no heap allocation in the audio path), T-016 (latency),
T-017 (`tailSeconds` is an upper bound) and T-018 (same behaviour at 44.1 and 192 kHz). They exercise whole effects across parameters, which no single shard owns; they run in the
core test job of `ci.yml` on every push. Consequence for the survivors below: a mutant they alone would kill still survives in Mull's report. Round 1 therefore adds
exact tail-time tests tagged with the shard's own tag (T-309 for the Phaser, T-412 for the Delay) instead of relying on T-017.

## Survivors (sub-step 3)

Round 1 (survivors of run 44447284584). Three files score below 60 %: effect_base.cpp, delay.cpp and phaser.cpp, 74 survivors in all. A fresh-context Opus-tier agent that did not write the
code (HANDOFF 9.4) dealt with each: **40 are killed** by 11 new tests (T-019, T-027 to T-031 in `test_common.cpp`, T-309 in `test_m1.cpp`, T-412 to T-415 in `test_m2.cpp`; each shown red against its
mutant applied by hand, green against the code) and **34 are justified** below. No bug was found. Per file: effect_base.cpp 13 killed, 26 justified; delay.cpp 13 and 4; phaser.cpp 14 and 4.
Survivors of files at or above 60 % are listed by count only: clocked_line.cpp 11, tables.cpp 18, lowpass.cpp 13, chorus.cpp 10, oscbox.cpp 15, dsp_common.cpp 7, filterbank.cpp 14, adapters.cpp 6, ringmod.cpp 2, blocks.cpp 3.

### Kill table

| Mutant | Operator | Outcome | Evidence |
|---|---|---|---|
| effect_base.cpp:53:23 | cxx_lt_to_le | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:58:23 | cxx_lt_to_le | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:61:63 | cxx_ge_to_gt | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:76:23 | cxx_lt_to_le | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:85:23 | cxx_lt_to_le | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:89:32 | cxx_ge_to_gt | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:89:32 | cxx_ge_to_lt | KILLED by T-027 / T-029 | `FAIL (killed, exit 42)` by T-027 / T-029; `tests/core/test_common.cpp:450: FAILED:`; `tests/core/test_common.cpp:493: FAILED:` |
| effect_base.cpp:91:30 | cxx_ge_to_gt | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:91:30 | cxx_ge_to_lt | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:93:22 | cxx_ge_to_gt | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:93:22 | cxx_ge_to_lt | KILLED by T-027 | `FAIL (killed, exit 42)` by T-027; `tests/core/test_common.cpp:448: FAILED:` |
| effect_base.cpp:98:28 | cxx_ge_to_gt | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:107:32 | cxx_ge_to_gt | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:111:29 | cxx_lt_to_le | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:115:32 | cxx_ge_to_gt | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:123:18 | cxx_ge_to_gt | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:123:18 | cxx_ge_to_lt | KILLED by T-030 | `FAIL (killed, exit 42)` by T-030; `tests/core/test_common.cpp:513: FAILED:` |
| effect_base.cpp:123:32 | cxx_lt_to_ge | KILLED by T-030 | `FAIL (killed, exit 42)` by T-030; `tests/core/test_common.cpp:513: FAILED:` |
| effect_base.cpp:123:32 | cxx_lt_to_le | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:135:15 | cxx_ge_to_gt | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:135:15 | cxx_ge_to_lt | KILLED by T-031 | `FAIL (killed, exit 42)` by T-031; `tests/core/test_common.cpp:522: FAILED:` |
| effect_base.cpp:135:29 | cxx_lt_to_ge | KILLED by T-031 | `FAIL (killed, exit 42)` by T-031; `tests/core/test_common.cpp:522: FAILED:` |
| effect_base.cpp:135:29 | cxx_lt_to_le | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:139:23 | cxx_lt_to_le | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:142:25 | cxx_ge_to_gt | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:145:54 | cxx_ge_to_gt | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:146:22 | cxx_ge_to_gt | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:146:22 | cxx_ge_to_lt | KILLED by T-027 | `FAIL (killed, exit 42)` by T-027; `tests/core/test_common.cpp:455: FAILED:` |
| effect_base.cpp:147:21 | cxx_ge_to_gt | KILLED by T-028 | `FAIL (killed, exit 42)` by T-028; `tests/core/test_common.cpp:479: FAILED:` |
| effect_base.cpp:147:21 | cxx_ge_to_lt | KILLED by T-028 | `FAIL (killed, exit 42)` by T-028; `tests/core/test_common.cpp:479: FAILED:` |
| effect_base.cpp:152:27 | cxx_lt_to_le | KILLED by T-019 | `FAIL (killed, exit 42)` by T-019; `tests/core/test_common.cpp:422: FAILED:` |
| effect_base.cpp:157:23 | cxx_lt_to_le | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:162:23 | cxx_lt_to_ge | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:162:23 | cxx_lt_to_le | KILLED by T-019 | `FAIL (killed, exit 42)` by T-019; `tests/core/test_common.cpp:435: FAILED:` |
| effect_base.cpp:165:27 | cxx_lt_to_ge | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:165:27 | cxx_lt_to_le | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:165:47 | cxx_pre_inc_to_pre_dec | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| effect_base.cpp:168:23 | cxx_lt_to_ge | KILLED by T-029 | `FAIL (killed, exit 42)` by T-029; `tests/core/test_common.cpp:493: FAILED:` |
| effect_base.cpp:168:23 | cxx_lt_to_le | KILLED by T-019 | `FAIL (killed, exit 42)` by T-019; `tests/core/test_common.cpp:436: FAILED:` |
| delay.cpp:85:47 | cxx_div_to_mul | KILLED by T-412 | `FAIL (killed, exit 42)` by T-412; `tests/core/test_m2.cpp:236: FAILED:` |
| delay.cpp:86:11 | cxx_ge_to_gt | KILLED by T-412 | `FAIL (killed, exit 42)` by T-412; `tests/core/test_m2.cpp:242: FAILED:` |
| delay.cpp:86:11 | cxx_ge_to_lt | KILLED by T-412 | `FAIL (killed, exit 42)` by T-412; `tests/core/test_m2.cpp:236: FAILED:` |
| delay.cpp:87:11 | cxx_le_to_gt | KILLED by T-412 | `FAIL (killed, exit 42)` by T-412; `tests/core/test_m2.cpp:236: FAILED:` |
| delay.cpp:87:11 | cxx_le_to_lt | KILLED by T-412 | `FAIL (killed, exit 42)` by T-412; `tests/core/test_m2.cpp:241: FAILED:` |
| delay.cpp:88:47 | cxx_add_to_sub | KILLED by T-412 | `FAIL (killed, exit 42)` by T-412; `tests/core/test_m2.cpp:236: FAILED:` |
| delay.cpp:88:68 | cxx_div_to_mul | KILLED by T-412 | `FAIL (killed, exit 42)` by T-412; `tests/core/test_m2.cpp:236: FAILED:` |
| delay.cpp:88:70 | cxx_minus_to_noop | KILLED by T-412 | `FAIL (killed, exit 42)` by T-412; `tests/core/test_m2.cpp:236: FAILED:` |
| delay.cpp:89:14 | cxx_gt_to_ge | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| delay.cpp:89:14 | cxx_gt_to_le | KILLED by T-412 | `FAIL (killed, exit 42)` by T-412; `tests/core/test_m2.cpp:236: FAILED:` |
| delay.cpp:94:64 | cxx_add_to_sub | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| delay.cpp:124:52 | cxx_gt_to_ge | KILLED by T-415 | `FAIL (killed, exit 42)` by T-415; `tests/core/test_m2.cpp:304: FAILED:` |
| delay.cpp:129:40 | cxx_lt_to_ge | JUSTIFIED (behaviour no decision fixes) | survives the shard filter (`PASS (survived)`); see justifications.md |
| delay.cpp:129:40 | cxx_lt_to_le | JUSTIFIED (behaviour no decision fixes) | survives the shard filter (`PASS (survived)`); see justifications.md |
| delay.cpp:139:37 | cxx_mul_to_div | KILLED by T-414 | `FAIL (killed, exit 42)` by T-414; `tests/core/test_m2.cpp:284: FAILED:` |
| delay.cpp:158:39 | cxx_sub_to_add | KILLED by T-413 | `FAIL (killed, exit 42)` by T-413; `tests/core/test_m2.cpp:264: FAILED:` |
| delay.cpp:158:81 | cxx_sub_to_add | KILLED by T-413 | `FAIL (killed, exit 42)` by T-413; `tests/core/test_m2.cpp:257: FAILED:` |
| phaser.cpp:51:68 | cxx_lt_to_le | JUSTIFIED (behaviour no decision fixes) | survives the shard filter (`PASS (survived)`); see justifications.md |
| phaser.cpp:161:31 | cxx_mul_to_div | JUSTIFIED (behaviour no decision fixes) | survives the shard filter (`PASS (survived)`); see justifications.md |
| phaser.cpp:161:39 | cxx_add_to_sub | JUSTIFIED (behaviour no decision fixes) | survives the shard filter (`PASS (survived)`); see justifications.md |
| phaser.cpp:167:61 | cxx_mul_to_div | KILLED by T-309 | `FAIL (killed, exit 42)` by T-309; `tests/core/test_m1.cpp:458: FAILED:` |
| phaser.cpp:168:67 | cxx_mul_to_div | KILLED by T-309 | `FAIL (killed, exit 42)` by T-309; `tests/core/test_m1.cpp:458: FAILED:` |
| phaser.cpp:168:142 | cxx_div_to_mul | KILLED by T-309 | `FAIL (killed, exit 42)` by T-309; `tests/core/test_m1.cpp:458: FAILED:` |
| phaser.cpp:168:149 | cxx_sub_to_add | KILLED by T-309 | `FAIL (killed, exit 42)` by T-309; `tests/core/test_m1.cpp:458: FAILED:` |
| phaser.cpp:169:93 | cxx_mul_to_div | KILLED by T-309 | `FAIL (killed, exit 42)` by T-309; `tests/core/test_m1.cpp:458: FAILED:` |
| phaser.cpp:170:72 | cxx_ge_to_gt | KILLED by T-309 | `FAIL (killed, exit 42)` by T-309; `tests/core/test_m1.cpp:458: FAILED:` |
| phaser.cpp:170:72 | cxx_ge_to_lt | KILLED by T-309 | `FAIL (killed, exit 42)` by T-309; `tests/core/test_m1.cpp:458: FAILED:` |
| phaser.cpp:171:33 | cxx_div_to_mul | KILLED by T-309 | `FAIL (killed, exit 42)` by T-309; `tests/core/test_m1.cpp:458: FAILED:` |
| phaser.cpp:171:40 | cxx_mul_to_div | KILLED by T-309 | `FAIL (killed, exit 42)` by T-309; `tests/core/test_m1.cpp:458: FAILED:` |
| phaser.cpp:173:11 | cxx_le_to_gt | KILLED by T-309 | `FAIL (killed, exit 42)` by T-309; `tests/core/test_m1.cpp:458: FAILED:` |
| phaser.cpp:173:11 | cxx_le_to_lt | JUSTIFIED (equivalent mutant) | survives the shard filter (`PASS (survived)`); see justifications.md |
| phaser.cpp:174:70 | cxx_mul_to_div | KILLED by T-309 | `FAIL (killed, exit 42)` by T-309; `tests/core/test_m1.cpp:458: FAILED:` |
| phaser.cpp:174:85 | cxx_add_to_sub | KILLED by T-309 | `FAIL (killed, exit 42)` by T-309; `tests/core/test_m1.cpp:458: FAILED:` |
| phaser.cpp:174:107 | cxx_div_to_mul | KILLED by T-309 | `FAIL (killed, exit 42)` by T-309; `tests/core/test_m1.cpp:458: FAILED:` |
| phaser.cpp:174:109 | cxx_minus_to_noop | KILLED by T-309 | `FAIL (killed, exit 42)` by T-309; `tests/core/test_m1.cpp:458: FAILED:` |

### Justified survivors, round 1 (CI run 44447284584)

Thirty-four of the 74 survivors in the three files below 60 % are justified here; the other 40 are killed by T-019, T-027 to T-031, T-309 and T-412 to T-415 (table: `kill_table.md`). Every justified mutant was applied by hand and still survives its shard's filter with the new tests in place. Facts about the effect tables come from `tests/fixtures/effects.json`, which T-001 freezes: in every effect parameter 0 is `drive` and `active` is the last parameter (index 7 to 18); input 0 is the audio input `in`; no `CvNormalised` input acts on `drive`; every output port has `param` -1; meters are `level` (0), the effect's own meter `env`, `lfo` or `rate` (1) and `active` (2); `kMaxParams` = 24 > 19, `kMaxPorts` = 16 > 8 and `kMaxMeters` = 4 > 3, so a slot one past the used range is still inside each member array.

#### Equivalent mutants (29)

**One extra loop pass or one extra accepted index, into a slot that nothing reads** (effect_base.cpp 53:23, 58:23, 76:23, 85:23, 98:28, 107:32, 111:29, 115:32, 123:32 `<=`, 135:29 `<=`, 139:23, 157:23). Each mutant lets the code touch index `numParams`, `numInputs` or `numMeters` of a member array. Every reader of those arrays either loops below the count or checks the index first (`getParam`, `getMeter`, `connected`, the loops of `reset()`, `advanceControls()` and `process()`), and the effects only pass the fixed, valid indices they looked up in their constructors. So the extra slot is written but never read, or read but always holds its zero initial value: `target_[numParams]` and `meters_[numMeters]` are written by nothing in the unmutated code (getParam and getMeter at the count therefore still return 0, as T-004 checks), `connected_[numInputs]` stays false (so 157:23 short-circuits before reading `in[numInputs]`), and the setters at 111:29 and 135:29 are never called with the count by any effect. Four of these mutants (53:23, 58:23, 98:28, 139:23) also read one element past the end of a static description table: undefined behaviour at source level, but in the built library each table is followed by the next one (`nm`: every `k<Effect>Inputs` lies directly before `k<Effect>Outputs`, whose first entry is an audio port with `param` -1, so 58:23's `CvNormalised && param >= 0` test is false), and the values read only reach the unread slots above.

**A comparison at a value the variable never takes.** 61:63 (`port.param > 0`): differs only for a `CvNormalised` input acting on parameter 0, `drive`, and no effect has one. 89:32 `>` and 145:54 `>`: differ only when `activeParam_` is 0; it is 7 or more. 93:22 `>` and 146:22 `>`: differ only when `activeMeter_` is 0; it is 2. 123:18 `>`: `connected()` differs only for input 0, and it is called only with `fb_return`, `carrier_in` or `sweep_in` (index 1) and `step_in` (index 6). 135:15 `>`: `setMeter()` differs only for meter 0, and every effect calls it with meter 1. 142:25 `>`: `cvInput_[p]` is -1 or the index of a `CvNormalised` input, never 0, the audio input.

**A value that is overwritten before it is used** (effect_base.cpp 91:30 `>` and `<`). `reset()` sets `driveGain_`, but `process()` calls `advanceControls()`, which recomputes `driveGain_` from the Drive position, before `tick()`; `drive()`, the only reader, is called only from `tick()`. The `reset()` inside the non-finite branch of `process()` is followed by `return`, so the next frame recomputes it as well.

**The non-finite branch of `process()`** (effect_base.cpp 162:23 `>=`, 165:27 `>=`, 165:27 `<=`, 165:47 `--`). These mutants change only what happens after `tick()` has produced a non-finite output (D-012: reset and output 0 for that sample). We argue, but have not proved, that no input admitted by the public interface reaches that branch: inputs are made finite and limited to +/-16, parameters to their ranges, the sample rate to [8 kHz, 768 kHz], the feedback loops of the Lowpass, Phaser, Delay and Chorus each contain a saturator (`softClip` or `tanh`: D-030, D-022, D-023, D-026), and the remaining filters and oscillators are driven by bounded signals. Evidence: with each of the four applied, the whole suite passes (98 cases, including T-007's 1.9 million frames of random corner settings and hostile inputs at six sample rates), and 165:47 would loop downwards through memory and crash the first time the branch ran. A test that reaches the branch would need an effect whose `tick()` returns NaN, that is, a test-only class derived from the private `EffectBase`. The frozen test wiring does not allow that (`core/src` is a private include directory of `muffcore`), and the frozen tests use public headers only (`plan/TRACEABILITY.md`, self-audit).

**A boundary that no float input reaches.** delay.cpp 89:14 (`t >= 60` for `t > 60`) differs only when the computed bound is exactly 60.0f. Every float Feedback value in [0, 10] was run through the same single-precision expression with the reference platform's `logf` (Linux x86-64, the platform Mull runs on). 612 values give a bound within 0.01 s of 60 and none gives exactly 60.0f (`/home/claude/work/survive_tools/enum.cpp`). phaser.cpp 173:11 (`g < 0.001` for `g <= 0.001`) differs only when `g = (double) 0.09f x (double) resonance` equals `(double) 0.001f`. The product of two floats is exact in double precision, so the enumeration over every float Resonance value in [0, 10] (no hit) gives the same answer on every platform.

**A buffer larger than any delay can use.** delay.cpp 94:64 sizes the line to `0.85 fs - 8` samples instead of `0.85 fs + 8`. `delaySeconds()` limits every delay (knob, LFO and tapped period) to at most 0.8 s, and a sample written at clock phase p is read when the phase reaches p + 1, so no read needs a sample more than `0.8 fs + 1` samples old. `0.85 fs - 8 >= 0.8 fs + 1` holds for every fs of at least 180 Hz, and `prepare()` rejects anything below 8 kHz (6 792 slots against at most 6 401 needed at 8 kHz). Neither the clamp in `ClockedLine::write()` nor the oldest-sample rule of `read()` is ever reached.

#### Only an omitted test kills it (0)

None. Each such mutant was either killed by a test carrying the shard's tag or is listed above.

#### Behaviour no decision fixes (5)

- delay.cpp 129:40 `>=` and `<=` (the Delay's `rate` meter lit for the second half of the LFO cycle, or also at phase exactly 0.5). D-023 gives the Delay no meter law. The common rule under "Effects (D-020 to D-026)" fixes only `level` and `active`, and the law "meter `rate` = 1 while the LFO phase is below 0.5" is written in D-026 for the Chorus alone. The fixture gives the Delay a `rate` meter without a law. (The Chorus law itself is now tested in the shared shard by T-031.)
- phaser.cpp 161:31 and 161:39 (the Phaser's `lfo` meter as `0.5 / (1 + lfo)` or `0.5 (1 - lfo)`). D-022 fixes `lfo_out` = 0.56 x LFO but no law for the `lfo` meter, and neither does the common rule. Both mutants stay within [0, 1] after `getMeter()`'s clamp, which is all that `core.hpp` requires of a meter.
- phaser.cpp 51:68 (`|v| <= 1e-20` flushed instead of `< 1e-20`). D-012 requires only that the core does not slow down on decaying signals: it may set flush-to-zero or flush its states itself. The 1e-20 threshold, and which side of it a state of exactly 1e-20 falls on, is an implementation choice that no decision or claim fixes. The difference is one all-pass state of 1e-20 (-400 dB) left in place or set to zero.

## Recommendation

The owner should mark `mutation-gate` (this workflow) and `aggregate` (`ci.yml`) as required status checks; that is a repository
setting and is not changed from here. Two planning observations for later: the effect-specific meters (`env`, `lfo`, `rate`) have no decided law
except the Chorus `rate` meter (D-026), so their mutants cannot be killed by a decision-backed test; and the shared shard's tag list did not originally
cover base-class behaviour (connection flags, meter writes, drive refresh, output limit), which is why round 1 had to add tests tagged for it.
