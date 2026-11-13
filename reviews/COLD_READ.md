# Cold-read gate (protocol 3.6): record of passes

Each pass used two new agents with no knowledge of the planning conversation, given only a fresh clone of the branch: a Sonnet agent that dry-ran `HANDOFF.md` and `plan/PLAN.md` without executing anything and listed every question, undefined term, missing input, ambiguous instruction, unstated credential and judgement call; and a Haiku agent that checked mechanically that every referenced path, command and id exists or is created by an earlier step, and the structural requirements of 3.6 item 2. Every item was resolved by amending the plan, never by an answer outside it.

| Pass | Commit read | Dry-run items | Mechanical failures | Resolution |
|---|---|---|---|---|
| 1 | `4e4aab9` | 92 | 4 | All amended in commit `3f32f27` (see below). |
| 2 | `02dc5ef` | 8 | 2 (neither a defect: a gate id mentioned only as not applicable; American spellings only inside verbatim source titles) | All amended in commit `430bf47` (see below). |
| 3 | `430bf47` | 8 | 1 (two citations of `HANDOFF.md` section 9.2 for things it did not state) | All amended in commit `2795122` (see below). |
| 4 | `2795122` | 0 | 1 (`rack/plugin.so` used in a Done-when without being named a build product; a note was added) and 1 not a defect ("sanitizer" is the tools' own name) | Gate passed: the dry run returned zero items. |

## Pass 1: what the items were and what changed
- Protocol and skills were cited but not on the branch (items 1 to 5): copied to `plan/protocol/` with their source commit; `premortem/` and `reviews/` now exist; "Lessons applied" lines cite lesson ids, not row names that did not exist.
- Access and environment (6, 7, 14 to 17): S-000 now probes workflow-file permission, log reading and the hosts later steps need, and separates blocking checks from recorded ones; the implementation branch name is literal.
- Process (8 to 13, 92): `HANDOFF.md` section 9 added: agents and tiers, gate answers and waiting, root records and the naming test, adding a frozen test, licence headers, commit markers, checkpoint schema; step commit and log commit distinguished; counts defined after added tests; the default rule made mechanical.
- Wrong or vacuous commands (18, 27, 33, 41, 42, 58): the Catch2 filter of S-001 (a bracket begins a tag, so `T-00[1-3]*` matched nothing) replaced by explicit names; `[filterbank]` selects 9 cases; `unittest -k` used with repeated options and an expected test count; pluginval tools given real arguments; `ctest -V`.
- Contradictions between documents (11, 21, 35, 39, 43, 63, 74, 76, 78, 84): section references, stub and CMake comments, the 45-minute rule, D-062 against S-REVIEW, dispatch against push markers, the response file's two parts, the round limit, job-level write permission.
- Under-specified work (19, 20, 22 to 26, 28 to 32, 40, 44 to 57, 59 to 62, 64 to 73, 75, 77, 79 to 83, 85 to 91): container shell; literal `deps.lock.json`; per-platform command lines; pattern-table accessors; effect registration; tail-time bounds that hold; the delay line's owner; the editor before S-021; Rack SDK recording without hand-typed hashes; a complete panel layout rule; module descriptions and the template design; the licence file in the Rack package; the advisory tool's options; README and user-guide wording; review briefs and pin-currency method; thread-sanitizer recipe; where the mutation floor lives, the job id `mutate`, the survivor stopping rule; package contents; marker commits that cannot fire by accident; the five dry inputs and how demo lines become event files; which CI run each Done-when reads.
- Planner tools that named the hardware sat in `tools/`, where test T-901 would have failed on them at S-035 (59): moved to `research/tools/`.
- Mechanical failures: undefined id ranges written as "D-001 to D-070", "D-011 to D-037", "T-101 to T-707" now name the real blocks; `[filterbank]` count; missing directories; American spellings were found only inside the verbatim protocol quotations, which stay as quoted.

## Pass 2: what the items were and what changed
1. S-021 passed bundle paths containing a space unquoted: now NUL-separated, and the size check counts seven.
2. S-LISTEN told the implementer to apply the oversampling remedy of R-006 first, though it is not pre-authorised: now only R-007's internal remedy is applied; R-006 needs the owner's request and the amendment route.
3. "Phase 2 re-opening" and "amendment route" were cited but not defined: `HANDOFF.md` section 6 now defines the amendment route and every citation uses that name.
4. `research/PRIOR_KNOWLEDGE.md` quoted the old level rule of the demo renderer: aligned with S-LISTEN.
5. The source register was not a knowledge item: `knowledge/K-20261112T130000Z-source-register-mufferfluffer.md` is now generated from the master `plan/SOURCE_REGISTER.md`; S-KNOW merges the taxonomy additions.
6. S-034 had no response for a failing or rejecting advisory service: added.
7. S-040 required the checksum file to exceed 100 kB: corrected.
8. `cancel-in-progress` would cancel the run of a step or marker commit when a log commit followed: one run per commit, never cancelled.
Mechanical note: the gates file's mention of the protocol's headline-result gate was reworded so that it is not read as a reference to a gate of this plan.

## Pass 3: what the items were and what changed
1. The demo renderer's right channel had two conflicting rules, one of which could put a control signal in a demo: the renderer now takes the right output as a required argument and the demo script passes `copy` except for the four stereo demos.
2. The limit of three full mutation runs clashed with full runs that start by themselves when tests change: the limit now counts rounds of survivor work.
3. A mutation shard for a file with almost no mutants would fail the minimum-mutant rule with no response: the four shared files form one shard, and small shards are merged.
4. The Done-when of S-REVIEW and S-ADVERSARIAL could pass with an empty review: each now requires a populated section per brief and a row per attack with its observed result.
5. No check that a pull request can be opened: S-FINAL has a fallback (compare link for the owner).
6. S-018 told the sanitizer job to run `ctest` in a tree it does not build: it keeps its own tree and must show 79 cases.
7. The Rack package check had no platform argument: each job passes its platform literally.
8. The panel label rule depended on an unstated letter width: the advance is given, with a two-line fallback.
Mechanical: `HANDOFF.md` section 9.2 now states the `## Record` rule and what to report at G-003 without an answer.

## Pass 4
The dry-run agent reported zero items and listed five minor points it rejected as covered by the default rule of `HANDOFF.md` section 6 (the count of non-container jobs in S-001, since clarified; how the inputs of demos 03 and 22 are joined; where a two-line label starts shrinking; the order in which S-MUTATE creates its tools; the unmeasured 5 MB floor of S-030, which the step already labels as the planner's choice). The cold-read gate is closed after four of the five passes allowed.

## Pass 5 (after the changes of pre-mortem round 1; commit read `432a8de`)
Protocol Phase 4 item 4 requires the cold read to run again after a plan change. A new dry-run agent, told which parts had changed, reported 5 items, all amended: D-060 still named the old file for the mutation floor; the Rack-flags core test on arm64 macOS used an x86 flag; the score cross-check had no input at the gate (now made per shard, where Mull's log exists); the macOS deployment-target check ran only in the gated package workflow (now also in the two macOS VST3 jobs of `ci.yml`); a header-only change was `not-applicable` to the mutation scope (now `full`). Test counts were confirmed against the files (83 core cases; 6; 9).

## Pass 6 (after the changes of pre-mortem round 2; commit read `736de5a`)
A new dry-run agent reported 3 items, all amended: `HANDOFF.md` still described the decision numbers as they were before D-063 and D-071; S-FINAL asked for a mutation result on the empty hand-over commit; the Lowpass tail rule returned 60 s where the true bound is longer (now computed, and reported as endless above 60 s as the frozen header requires). Counts confirmed: 87 core (86 under the sanitizers), 7 VST3, 9 Python; every test id has a traceability row.
