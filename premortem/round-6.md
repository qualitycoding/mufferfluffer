# Pre-mortem round 6 (closing round: verification of round 5's dispositions)

Reviewer: fresh context; I did not write the plan. Branch clone `/home/claude/pm6` at `63b4314`, read only: nothing was edited, committed or pushed there. Scratch work was done in `/tmp/pm6work`. Date 2026-11-13.

## Method

What I read:
- `premortem/round-5.md`, including the planner's disposition table, and the full diff of `63b4314` (the round 5 fixes). I also read `b05d2bb..3155c43`, the cold-read commit that came between round 5's base and the fixes.
- `HANDOFF.md` in full.
- In `plan/DECISIONS.md`: D-002, D-004, D-012, D-027 (items 12 to 17) and D-072.
- `plan/GATES.md` in full.
- In `plan/PLAN.md`: S-018, S-032, S-034, S-035, S-REVIEW, S-MUTATE, S-LISTEN, S-FINAL, S-RETRO and S-KNOW.
- A-007, `plan/RISKS.md` (R-019, R-020), `plan/ENVIRONMENT.md`, `plan/TRACEABILITY.md` (T-9xx rows) and `premortem/RISK_REGISTER.md`.
- `premortem/ASSUMPTION_AUDIT.md`, from its disposition table onwards.
- `core/include/muff/core.hpp` (`PortKind`, `PortSpec`) and `tests/CMakeLists.txt`.
- `tests/py/test_repo.py`: T-902 and T-905.
- Every lesson dated 2026-11-12 and 2026-11-13, and every knowledge item dated 2026-11-13, plus `c-planner-004`, `-012` and `-015`.
- Protocol Appendices A, B and C.
- The example Mull workflow, lines 85 to 112.

What I ran (in `/tmp/pm6work`):
- **E1.** I extracted the Appendix C schema check verbatim (the Python block after "Schema check.") into `/tmp/pm6work/schema_check.py` and ran it from a full copy of the clone. Output: `all entries valid`, exit 0 (11 lessons, 377 knowledge items). The knowledge store's own `scripts/check_knowledge_schema.py` is the same script and gives the same result.
- **E2.** `python3 tools/check_frozen.py` in the clone: `frozen files intact: 21 files`, exit 0. `git diff --stat b05d2bb HEAD -- tests tools core adapters vst3 research/spikes/oracle_repo` is empty, so no frozen file or oracle changed after round 5.
- **E3.** I built the synthetic tree with `research/spikes/oracle_repo/make_tree.py` and ran the frozen Python suite against it (`MUFF_REPO_ROOT=<tree>`): `Ran 9 tests … OK`.
- **E4.** Test counts. `^TEST_CASE` gives 87 in `tests/core/*.cpp` (5 + 24 + 16 + 23 + 19) and 7 in `tests/vst3/`. There are 9 `def test_` in `tests/py/test_repo.py`. The sanitizer build defines `MUFF_NO_ALLOC_HOOK`, which compiles out exactly one case (T-011, `test_common.cpp:19` and `:260-284`), leaving 86.
- **E5.** Traceability: 94 C++ test ids and T-901 to T-907 (the 9 Python methods) all have a row in `plan/TRACEABILITY.md`; none is missing.
- **E6.** Read-only shallow clones of `qualitycoding/knowledge` and `qualitycoding/Lessons`. Both `supersedes` store ids exist: `knowledge/mutation-testing-cpp/K-20261027T123200Z-mull-mutation-testing.md` and `knowledge/vcv-plugin-build/K-20261009T234214Z-rack-compile-mk-flags-and-cpp17.md`, both `status: current` today. I compared every tag on the branch with each store's `TAXONOMY.md` and with `knowledge/TAXONOMY-ADDITIONS.md`.
- **E7.** I dumped the fixture's input ports (kind, depth, target) to check H-2 and the CV workaround. Every `cv_normalised` input has depth 1.0, except OscBox `cv_wave` (0.6). OscBox `cv_frequency` is `cv_volt_per_oct`.
- **E8.** Searched every planning document for leftover wording of the old CV-law route: "may reverse", "re-freezes tests", "centred response of the hardware", "gives the hardware", and every L-8 reference.

Not re-opened, because earlier rounds, D-027 or the risk register already handle them: Mull estimates (R-010), the absence of reference recordings (R-004), the unverifiable DSP claims (R-020, apart from its labels), the macOS floor (D-071), the choice of the additive law itself, and the decision not to add a VST3 side-chain.

## Verification of round 5's dispositions

| Item | Disposition claimed | Holds? | What I checked |
|---|---|---|---|
| PM5-05 (High) | The centred law is not implemented under any gate answer; L-8 is for information; a request becomes follow-up planning | **Holds** | See "Check (a)" below. The documents agree, and none of them still offers the route. Two cross-references are thin (PM6-06, Low). |
| PM5-06 (Medium) | The open point on the delay's Time-input direction is carried forward | **Holds** | D-012 now says "without claiming that this reproduces the hardware exactly (… D-072 item 3)". D-027 item 13 says "was not settled", and so does D-072 item 3's last sentence. The S-035 guide sentence was softened ("gives a centred response; the guide does not claim this matches the hardware exactly"). `K-20261113T170200Z` records the open point, though it states it twice (PM6-03). The arithmetic of the workaround is sound: `CvNormalised` accepts negative CV, limited to ±16 before use and clamped only after the sum (`core.hpp:43`, D-012). |
| PM5-10 (Medium) | Lessons on the template; `subject:` tags; schema check passes | **Holds** | E1: exit 0. All seven lessons dated 11-12 and 11-13 have `supersedes`, `recurrence_of`, `occurrences` and the six `## ` headings, including `## Evidence`. `domain:ci` is gone. One lesson still uses tags from outside both taxonomies; the schema check does not catch this (PM6-04, Low). |
| PM5-11 (Medium) | Wrong items superseded; `supersedes` set; S-KNOW marks the store items | **Holds** | `c-planner-012` and `c-planner-015` are `status: superseded`. K-170100Z supersedes `[c-planner-012, K-20261027T123200Z-mull-mutation-testing]` and K-170400Z supersedes `[c-planner-015, K-20261009T234214Z-rack-compile-mk-flags-and-cpp17]`. Both store ids exist (E6). S-KNOW now names both ids and says to set `status: superseded` on them in the store, "in the same commit" in effect. Side effects: one or two still-valid facts lose their only current carrier, and S-KNOW's "Done when" does not check the change (PM6-08, Low). |
| PM5-01, PM5-15 (Low) | `modularGridUrl` banned in S-032; brief 1 checks it and the C++17 library limits | **Holds** | S-032 action 1 says "no `modularGridUrl` property". S-REVIEW brief 1 names both checks. D-002 points to brief 1. A small inaccuracy remains (PM6-09). |
| PM5-02, PM5-04 (Low) | Assets listed in CI over REST; `digest` compared | **Holds** | S-MUTATE.1 now uses `gh api repos/mull-project/mull/releases --jq …` inside a CI job with `GITHUB_TOKEN`, and requires the computed SHA-256 to equal `digest` where one is given. |
| PM5-03 (Low) | The step overrides the example workflow | **Mostly holds** | The overrides for Cloudsmith, `lld-19` and `mull-runner-19` are stated. They do not say to drop the example's `-DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld-19"` (example line 99). If that line is copied, the link fails loudly (PM6-09). |
| PM5-07 (Low) | H-2 names the knob position | **Holds, with a residue** | H-2 now says "with a control's knob at its minimum". The "full range" answer is still untrue for OscBox Wave and Frequency (PM6-07). |
| PM5-08 (Low) | Stated as the project's preference | **Holds** | D-072 item 3: "(a preference of this project, not a rule of Rack)". |
| PM5-09 (Low) | One list, in D-027 item 17 | **Partly** | A-007 now points to item 17. Item 17 says "A-007 and `HANDOFF.md` section 8 point to it", but §8 does not, and §8 still lists neither divergence (PM6-02). |
| PM5-12 (Low) | Sources made exact; tags from the taxonomy | **Does not hold in full** | K-170100Z and K-170400Z now have exact URLs. However: K-170000Z still uses the run's own `subject:plugin-licensing-and-marks`, which round 5 called a near-duplicate of the store's `subject:vcv-plugin-licensing`, and the store's `subject:vcv-plugin-manifest` fits exactly. K-170200Z's URL is now the ManualsLib *brand index*, not the copies that were read. K-170300Z still has `derived_from: []`, and its new URL is a path on the planning branch that will not resolve once the item is in the store (PM6-03). |
| PM5-13 (Low) | The pin lesson's Evidence records the Mull recurrence | **Partly** | Evidence cites F-1 and the tag `tech:mull` was added. `occurrences` is still 2, and the prevention rule was not widened to tool facts. Rule 11 asks for both when a mistake recurs within the run (PM6-05). |
| PM5-14 (Low) | `page_token` named; bound of 20 pages | **Holds** | S-034: "repeated with `"page_token": <that value>` added … at most 20 pages, after which the tool fails". |
| PM5-16 (Low) | "Fixed" | **Partly** | Fixed: `premortem/RISK_REGISTER.md` says R-001 to R-020 (done in `3155c43`); the ENVIRONMENT pluginval cell now cites round 3; `HANDOFF.md` §6 gives D-072 as the highest number. Not fixed: D-072 still sits between D-071 and D-070 in `plan/DECISIONS.md`; R-020 still labels the three claims "filter-bank sweep behaviour, oscillator sync and anti-aliasing details", whereas C-DSP-029 is stability under cutoff modulation, C-DSP-053 the polyBLEP residual and C-DSP-055 alias-free limits; `HANDOFF.md` §3 item 3 still calls D-027 "the choices made where the hardware is undocumented", while item 9 of the same list says "D-027 lists every choice the owner is shown" (PM6-01). |

### Check (a): the centred control-voltage law

- **D-072 item 3** says: "**This plan does not implement option (a) under any gate answer.**" A request is "recorded as a request for follow-up planning (a new plan amendment by a planner, because it changes a frozen public header), listed in `COMPLETION.md`, and does not stop the gate from being answered `proceed`". The fixture fields, tests and route it names are no longer wrong: it names the `core.hpp` port kind, the rule for unpatched inputs and new tests for three effects, all as things that do not exist.
- **D-012** keeps one additive law on purpose and gives the workaround, without a fidelity claim.
- **D-027 item 13** keeps the law, gives the workaround as "a response centred on the knob", and says the Time-input direction was not settled.
- **D-027 item 17 and A-007** are consistent with each other.
- **GATES L-8** is "for information". It says changing the law "is not part of this plan (D-072 item 3)", that a request is recorded in `GATE-G-LISTEN.RESPONSE.md` and `COMPLETION.md`, and that it "does not affect the answer to this gate".
- **GATES H-2** names the knob at minimum.
- **S-035** describes the centred response with no claim of hardware fidelity.
- **E8** found no leftover "may reverse", "re-freezes tests" or "gives the hardware's response" anywhere outside the round files.

Can the implementer still be forced to guess at G-LISTEN?
- **Allowed responses.** No keyword is needed for L-8, because L-8 explicitly does not affect the answer. `proceed` with a CV remark is therefore fully specified.
- **`iterate` with a CV remark.** D-072's "under any gate answer" settles it. S-LISTEN action 5 ("one targeted change per reported difference") and its "On failure" ("… offer `proceed-with-rescope` or a test challenge") do not cite D-072. An implementer who reads only S-LISTEN could offer a test challenge, which D-072 rules out. That path ends loudly, not silently: the frozen header `core.hpp` is halt-class under `HANDOFF.md` §6, so the challenge would halt. The plan also does not say whether such a remark uses up an `iterate` round. Both points are cross-reference gaps with a decisive rule elsewhere, so they are Low (PM6-06).
- **S-FINAL's list for `COMPLETION.md`** omits the L-8 request, although GATES and D-072 both say it is listed there. Also Low (PM6-06).
- **`HANDOFF.md` §6 and §9.2** contain nothing that contradicts D-072. §6's amendment route gives as examples only patterns or recordings the owner supplies.

Verdict on (a): consistent. No High remains.

### Check (b): lessons, knowledge items and S-KNOW

- **Schema check output (E1):** `all entries valid`, exit status 0.
- **`supersedes`:** every id named exists, either on the branch (`c-planner-012`, `c-planner-015`) or in the store (E6).
- **S-KNOW** tells the implementer to set `status: superseded` on the two store items when copying into the store.
- **Remaining deviations from Appendices A to C**, none caught by the schema check:
  - `L-20261112T205000Z` uses `domain:dsp`, `phase:test-design` and `kind:test-validity`. None is in the lesson store's `TAXONOMY.md` or in the additions file. The last two are near-duplicates of `phase:specification` ("specification and tests") and `kind:verification-gap` (PM6-04).
  - Five items have `discovered_by: assumption audit (fresh-context reviewer), confirmed by planner`, outside the template's `planner | implementer`.
  - The details of PM6-03 and PM6-05.

### Check (c): S-MUTATE, S-034, S-032 and S-REVIEW brief 1

The texts are present and correct, apart from the small points in PM6-09. Those points:
- The `-fuse-ld=lld-19` flag of the copied example is not overridden.
- The `digest` comparison may need the algorithm prefix removed: if GitHub returns the field as `sha256:<hex>`, a bare comparison never matches. That would be a loud failure.
- D-002's "would also fail to link" should say "fail to compile". With a deployment target of 10.9, Apple's availability markings reject `std::filesystem` and `std::optional::value` at compile time.

## Consistency sweep

- **Highest decision number.** D-072 is highest wherever a number is stated: `HANDOFF.md` §3 item 3 lists it and §6 says "D-072 at hand-over". The only other mentions, "D-001 to D-070" and "D-071 as the highest" in `reviews/COLD_READ.md` lines 20 and 54, are dated historical records. The file order is D-071, D-072, D-070 (PM6-01).
- **Risk range.** R-001 to R-020 in `plan/RISKS.md` (20 rows, last R-020) and in `premortem/RISK_REGISTER.md`. No stale "R-019" remains.
- **Test counts (E4).**

  | Suite | Count | Where stated |
  |---|---|---|
  | Core | 87 | `HANDOFF.md` §4, PLAN line 3, S-018, S-REVIEW, S-FINAL |
  | Sanitizer build | 86 | PLAN line 3, S-018 action 2 and Done-when; mechanism confirmed |
  | VST3 | 7 | `HANDOFF.md` §4, S-REVIEW, S-FINAL |
  | Python | 9 | `HANDOFF.md` §4, S-FINAL |

  S-035's "Ran 8 tests" is right: T-901 has 2 methods, T-902 to T-904, T-906 and T-907 one each, plus `test_dependency_lock`.
- **Traceability (E5):** every test id has a row.
- **`check_frozen.py` (E2):** exit 0.
- **Done-when lines:** no step's Done-when names a file, job or count that another document contradicts. Two gaps:
  - S-FINAL's `COMPLETION.md` contents omit the L-8 request (PM6-06).
  - S-KNOW's Done-when does not cover the new supersession edit (PM6-08).
- **Minor document points:**
  - D-027 lists its items in the order 12, 13, 17, 14, 15, 16 (PM6-01).
  - `L-20261113T170500Z`'s Correction still says "a question put to the owner at the listening gate", although L-8 is now for information only (PM6-09).

## Lens notes (parts changed since round 4)

| Lens | Remaining Critical or High? | What I checked |
|---|---|---|
| Technical | None | The CV workaround against `core.hpp` (negative CV accepted; clamp after the sum). The fixture's depths (E7). T-902 and the synthetic tree (E3). Frozen files unchanged (E2). |
| Supply chain | None | Mull: two packages, REST listing, digest comparison, hashes in `deps.lock.json` verified on later runs, T-905 reads only `PINS`. Residues are Low (PM6-09). |
| Research assumptions | None | D-072 item 3's statement that the centred law needs a new port kind holds (`PortSpec` has no offset field). The Time-direction point is carried forward as unsettled. The knowledge item's confidence is `single-source`, which is honest. |
| Implementer misinterpretation | None | G-LISTEN responses, S-LISTEN action 5 and On failure, `HANDOFF.md` §6 and §9.2, H-2, S-MUTATE's overrides. The worst residual path (a test challenge for the CV law) ends loudly at a halt-class header (PM6-06, PM6-07). |
| Integrity | None | No document still claims that the 2.5 V workaround reproduces the hardware. Supersessions are correct. Two "Fixed" dispositions in round 5 are overstated (PM5-12, PM5-16): a records-accuracy point, Low. |
| Past lessons | None | Rule 11 recurrence handling for F-1 (PM6-05). Lesson templates (E1). |
| Performance | None | Nothing that affects performance changed after round 4. |
| Security | None | The Appendix C secret regex passes on all entries (E1). By reading the 11-13 files I found no tokens or personal data beyond the public account name. The workflow listing uses `contents: read` and `GITHUB_TOKEN` only. |
| Operations | None | Mull asset listing moved into CI. S-KNOW gives the store branch (`main`). The stores are reachable from this sandbox for clone (E6). |
| CI platform coverage | None | Nothing changed in the matrix. The mutation runner stays `ubuntu-24.04` amd64, which matches Mull's packages. |

## Risk table

| id | description | lens | severity | likelihood | root cause | trace | proposed mitigation |
|---|---|---|---|---|---|---|---|
| PM6-01 | Parts of PM5-16 not done: (1) D-072 sits between D-071 and D-070; (2) R-020's labels for C-DSP-029, -053 and -055 do not match the claims; (3) `HANDOFF.md` §3 item 3 still calls D-027 "choices made where the hardware is undocumented", which disagrees with item 9; (4) D-027's items run 12, 13, 17, 14, 15, 16. | documentation | Low | n/a | disposition marked "Fixed" without re-reading each item | `plan/DECISIONS.md`, `plan/RISKS.md` R-020, `HANDOFF.md`:14 | Move D-072 to the end. Relabel R-020 as "stability under cutoff modulation, polyBLEP residual, alias-free limits". Reword item 3 as "design choices, including deliberate divergences (items 13, 17)". Move item 17 after 16. |
| PM6-02 | D-027 item 17 says `HANDOFF.md` §8 points to it, but §8 does not, and §8 lists neither the CV-law divergence nor the VST3 jack gap. | documentation | Low | n/a | half of PM5-09's fix applied | D-027 item 17, `HANDOFF.md` §8 | Add one §8 bullet: "The VST3 plug-ins lack the jacks of D-027 item 17; three modules use a CV law that differs from their manuals (D-027 item 13)." |
| PM6-03 | Parts of PM5-12 not done. K-170000Z still uses the run's own near-duplicate `subject:plugin-licensing-and-marks`, although `subject:vcv-plugin-manifest` exists. K-170200Z's URL is a brand index, not the copies read, and its body repeats the "not settled" sentence. K-170300Z has `derived_from: []` and a branch-relative URL that breaks in the store. Five items have `discovered_by` outside `planner \| implementer`. | process | Low | n/a | — | Appendices B and C; `knowledge/K-20261113T1700*`–`1703*` | Use `subject:vcv-plugin-manifest`; give the exact manual URLs; `derived_from: [C-DSP-064]`; a repository URL with the commit; `discovered_by: planner` with the auditor named in the body. |
| PM6-04 | `L-20261112T205000Z` has tags in neither taxonomy: `domain:dsp` (lesson store), `phase:test-design` and `kind:test-validity`. The last two are near-duplicates of `phase:specification` and `kind:verification-gap`. S-RETRO calls the file a worked example and requires taxonomy tags, and S-KNOW would merge the near-duplicates into the store. | process | Low | certain (as written) | the schema check tests only the facet prefix | Appendix C, S-RETRO, S-KNOW | Retag as `phase:specification` and `kind:verification-gap`. Keep `domain:dsp` and list it for the lesson store's `TAXONOMY.md`. |
| PM6-05 | PM5-13 partial: `L-20261113T093000Z` records the Mull recurrence in its Evidence, but `occurrences` stays 2 and the prevention rule was not widened, which Rule 11 requires. | past lessons | Low | n/a | — | Rule 11, `lessons/L-20261113T093000Z…` | Set `occurrences: 3`. Widen the trigger and rule to "a tool fact (pin, package layout, install recipe) recorded without running it". |
| PM6-06 | S-LISTEN action 5 and its On failure do not cite D-072 item 3, so a CV-law remark inside `iterate` could be offered as a test challenge, contrary to D-072. It is not stated whether such a remark uses up an `iterate` round. S-FINAL's list for `COMPLETION.md` omits the L-8 request that GATES and D-072 say is listed there. | misinterpretation | Low | low | the fix edited D-072 and L-8 but not the steps that act on the answer | S-LISTEN, S-FINAL, D-072 item 3, GATES L-8 | Add to S-LISTEN action 5: "A remark asking for the centred CV law (L-8) is not an iterate change: record it per D-072 item 3; it does not count as a round." Add "any L-8 request (D-072 item 3)" to S-FINAL's `COMPLETION.md` list. |
| PM6-07 | H-2's "0 to 5 V … sweep the control over its full range" is untrue for OscBox Wave (depth 0.6, by design) and does not apply to OscBox Frequency (1 V/oct). An owner who tests those could answer "no" and use up an `iterate` round. | misinterpretation | Low | low | wording generalised over all inputs | GATES H-2, D-012, fixture | Append to H-2: "(except the Fluff Osc Wave input, which sweeps 60 %, and its 1 V/oct Frequency input)". |
| PM6-08 | Superseding whole store items retires a few still-valid facts that no current item carries any more (from the Mull item: "Release builds must not be mutated"; from `c-planner-015`: "a stub libRack suffices for a compile check"). Most of the other facts survive in store items E6 found (`mull-runner-cli-and-config`, `rack-windows-build-needs-msys2…`, `oildrumkit-engine-bit-identical…`). S-KNOW's Done-when does not check the status change. | integrity (knowledge) | Low | high | supersession at item level, correction at fact level | Appendix B, S-KNOW, K-170100Z, K-170400Z | Restate the still-valid facts in the superseding items' bodies. Add to S-KNOW's Done-when: "the two named store items read `status: superseded`". |
| PM6-09 | Small text points: (1) S-MUTATE does not tell the implementer to drop `-fuse-ld=lld-19` from the copied configure line (a loud link failure if kept); (2) the `digest` comparison may need the algorithm prefix removed if GitHub returns `sha256:<hex>`; (3) D-002 says "fail to link" where availability errors are compile-time; (4) `L-20261113T170500Z`'s Correction still says "a question put to the owner". | documentation | Low | low | — | S-MUTATE, D-002, lesson | One clause each. |

## Counts

- **Frozen suite:** 87 core cases (86 in the sanitizer build), 7 VST3, 9 Python. The manifest (21 files) is intact, and the Python suite passes on the synthetic tree.
- **Decisions:** D-001 to D-072 (D-072 highest). **Risks:** R-001 to R-020.
- **Schema check of Appendix C:** `all entries valid`, exit 0.
- **Round 5 dispositions:**
  - Hold: 8 of the 14 rows, covering PM5-05, -06, -10, -11, -01 with -15, -02 with -04, -08 and -14.
  - Hold with a residue: 2 rows (PM5-03, PM5-07).
  - Hold only in part: PM5-09, PM5-12, PM5-13 and PM5-16. All four are Low.
- **Findings this round:** 9, all Low. No Critical, High or Medium failure mode remains in the parts changed since round 4.

CRITICAL: 0 HIGH: 0 MEDIUM: 0 LOW: 9

---

## Planner's disposition (added after the round; the text above is the reviewer's report, unedited)

Round 6: 0 Critical, 0 High, 0 Medium, 9 Low. The pre-mortem loop is closed. Low items PM6-01, PM6-02, PM6-04, PM6-05, PM6-06 (S-FINAL), PM6-07 and PM6-08 (S-KNOW's Done-when) were corrected in the commit that adds this file; PM6-03 and PM6-09 (a source URL that is an index page, an empty `derived_from`, small wording points) and the loss of two still-true facts when whole store items are superseded (PM6-08) are left as they are and will be seen by S-RETRO's coverage check.

Totals over six rounds: 0 Critical; 8 High, all fixed (2, 3, 1, 0, 1, 0).
