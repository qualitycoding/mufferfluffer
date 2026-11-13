# Pre-mortem round 5 (changed parts after the assumption audit, D-072)

Reviewer: fresh context, did not write the plan. Branch clone `/home/claude/pm5` at `b05d2bb` (read only; nothing edited, committed or pushed). Date 2026-11-13.

## Method

What I read: `premortem/ASSUMPTION_AUDIT.md` (all tables, "Falsified items", the planner's disposition table), the full diff of `b05d2bb`, D-002, D-004, D-012, D-016, D-027, D-037, D-060, D-072, A-007, `plan/GATES.md` (G-LISTEN), S-032, S-034, S-035, S-MUTATE, S-REVIEW, S-RETRO and S-KNOW in `plan/PLAN.md`, `plan/RISKS.md`, `plan/ENVIRONMENT.md`, `plan/TRACEABILITY.md`, `HANDOFF.md`, `tests/py/test_repo.py`, `tests/core/test_common.cpp` (T-012), `core/include/muff/core.hpp` (`PortKind`, `PortSpec`), the fixture, `research/spikes/oracle_repo/make_tree.py`, the new lessons and knowledge items, protocol Appendices A to C and `knowledge/TAXONOMY-ADDITIONS.md`.

What I ran (in `/tmp/pm5work`):
- E1 `make_tree.py` on a copy of the branch, then the frozen Python suite against the synthetic tree: `Ran 9 tests … OK`.
- E2 Same tree plus a `mull` lock entry shaped as S-MUTATE specifies (`{"version", "assets": [{name, sha256}, {name, sha256}]}`) and an empty `advisory_allow`: `Ran 9 tests … OK` (T-905 checks only the keys in `PINS`).
- E3 Same tree with `"Hardware clone"` appended to one module's tags: `FAIL: test_manifest … 'Hardware clone' unexpectedly found`. T-902 is sound and satisfiable.
- E4 `tools/check_frozen.py`: `frozen files intact: 21 files`; `sha256sum tests/py/test_repo.py` equals the manifest line.
- E5 Counted `^TEST_CASE` in `tests/core/*.cpp` (87) and `tests/vst3/test_vst3.cpp` (7). I did not re-run the sanitizer build (86).
- E6 The schema check of protocol Appendix C, extracted verbatim and run over `lessons/` and `knowledge/`: exit 1 with six problems (PM5-10).
- E7 Mull: `git ls-remote --tags` (newest tag is 0.38.0). Shallow fetch of tag 0.38.0, where I read `docs/Installation.rst:55-80` and `mull_package.bzl:56-98, 208-215, 316, 324-332, 399, 406-411`. The asset listing could not be reached: `gh release view` uses GraphQL, which this sandbox blocks; REST `repos/mull-project/mull/releases` was refused (repository not enabled); and the release page returned 403.
- E8 Fetched the VCV manual Manifest page.
- E9 Fetched an MF-104M manual copy (ManualMachine) for the Time CV direction.
- E10 Fetched the OSV `POST /v1/query` documentation for pagination.
- E11 `git ls-remote` on `steinbergmedia/vst3sdk`: `v3.8.0_build_66` exists.
- E12 `git ls-remote` on `qualitycoding/Lessons` and `qualitycoding/knowledge`: HEAD is `main` for both, and `Lessons` also has `master`.
- E13 Shallow clones of both stores. I read their `TAXONOMY.md` files and the existing items on Rack C++ level and on Mull.

Lenses applied to the changed parts: (a) every area named in the brief; supply chain (Mull pins, hashes, example workflow); implementer misinterpretation (L-8 route, H-2 wording, S-MUTATE patterns); integrity (knowledge supersession, honesty of D-072 wording); past lessons (Rule 11 recording of F-1); security (new files scanned for secrets with the Appendix C regex and by reading: none, no private data beyond the public account name and the run id); operations (sandbox reachability of the Mull release API); CI platform coverage (Mull is amd64-only, which matches the `ubuntu-24.04` x64 mutation runner; no new platform gap).

Not re-reported (already handled): Mull estimates (R-010), the macOS deployment floor itself (D-071), the absence of reference recordings (R-004), the VST3 validator (R-016), and the earlier F-1 to F-6 facts as such.

## Findings

### Area 1: the Rack tag

T-902 now asserts `assertNotIn("Hardware clone", tags)` and still requires every tag to be in `RACK_TAGS`. The deprecated alias "Hardware" is not in `RACK_TAGS`, so it is also rejected. The test is sound and satisfiable (E1, E3). `make_tree.py`, S-032 and D-004 agree with it. No other test or fixture entry assumes the tag. The only other mentions are research records and the knowledge item for C-PLANNER-004 (see PM5-11).

The VCV manual (E8) attaches the permission request, the "Based on" description and `modularGridUrl` to modules that carry the tag. It does not require hardware-inspired modules to carry it. Removing the tag therefore creates no new exposure under VCV's rules, and it narrows the self-description that R-013 worries about. There is no Library submission (A-009).

- **PM5-01 `modularGridUrl` ban not enforced.** D-004 says the manifest has no `modularGridUrl`, but neither T-902 nor S-032's "Done when" checks it, and S-032's field list does not mention it.
  - Lens: implementer misinterpretation. Severity: Low. Likelihood: low.
  - Root cause: the D-004 sentence was added without a check.
  - Trace: D-004, S-032, T-902.
  - Evidence: `test_repo.py:85-103`.
  - Mitigation: add "and no `modularGridUrl` key" to S-032 action 1 and to S-REVIEW brief 1. No re-freeze is needed.

### Area 2: Mull

Verified against Mull source at tag 0.38.0 (E7):
- The runner package file name is `Mull-{MULL_VERSION}-{OS_NAME}-{OS_ARCH}-{OS_VERSION}`.
- The plug-in package is `Mull-{LLVM}-{MULL_VERSION}-LLVM-{FULL_LLVM}-{OS}-{ARCH}-{OS_VERSION}`.
- `Mull-Extras-…` is a separate package holding `mull-instrument`/`mull-rewriter`.
- The documentation's own example uses exactly the patterns `Mull-0.36.0-ubuntu-amd64-24.04.deb` and `Mull-19-0.36.0-*-ubuntu-amd64-24.04.deb`, and installs with `apt-get install ./Mull-*.deb`.
- The plug-in path `/usr/lib/mull-ir-frontend-19` still exists.

S-MUTATE's patterns, its two-file install and its exclusion of Extras therefore match the source. The real asset names of release 0.38.0 could not be listed (E7). That the release carries Ubuntu 24.04 amd64 files for LLVM 19 is inferred from the packaging rules and the CI workflow `ci-Ubuntu_24.04-x86_64.yml`, not observed.

T-905: the new `mull` entry passes `test_dependency_lock` (E2), because the test reads only `PINS` keys. S-034's rule: the entry has no `commit` key, so `check_advisories.py` prints `not queried: no commit pin` for it, which is correct for a build tool. R-010, D-060 and ENVIRONMENT are consistent with S-MUTATE.

- **PM5-02 Mull hashes are trust on first use.** The SHA-256 values that S-MUTATE.1 writes are computed from the first download, so a tampered or replaced asset at first use would be pinned as good.
  - Lens: supply chain. Severity: Low. Likelihood: low.
  - Root cause: no second source for the hash.
  - Trace: S-MUTATE "Tool and pin", D-060.
  - Mitigation: in sub-step 1, also record the `digest` field that the REST release-asset listing returns and require it to equal the computed SHA-256. Note it in `reviews/MUTATION.md`.
- **PM5-03 The starting-point workflow contradicts the new recipe.** S-MUTATE tells the implementer to "start the workflow from `references/example-mutation-workflow.yml`". That file still installs through Cloudsmith (`mull-19`), installs `lld-19`, links with `-fuse-ld=lld-19` and calls `mull-runner-19`. The plan overrides the source ("Cloudsmith repository is not used") and the runner name. But its apt line (`clang-19 libclang-rt-19-dev`) omits `lld-19`, so a faithful copy of the build step fails at link time. `mull-runner-19` survives only as a deprecated symlink.
  - Lens: implementer misinterpretation, supply chain. Severity: Low (it fails loudly and the fix is obvious). Likelihood: medium.
  - Root cause: the amendment changed the install text but not the list of differences from the example.
  - Trace: S-MUTATE Inputs and "Tool and pin"; example workflow lines 85-112.
  - Mitigation: add one sentence to S-MUTATE: "replace the example's install step entirely; add `lld-19` to the apt line or drop `-fuse-ld=lld-19`; call `mull-runner`, not `mull-runner-19`."
- **PM5-04 The listing commands need an API this kind of sandbox blocks.** `gh release view` goes through GraphQL. In an agent sandbox like this one, GraphQL is refused, and REST needs the repository to be attached (E7). The plan does say "when run in CI" with `GITHUB_TOKEN`, where both work.
  - Lens: operations. Severity: Low. Likelihood: medium if the implementer tries sub-step 1 locally first.
  - Trace: S-MUTATE sub-step 1.
  - Mitigation: state that the listing is done inside the `mutation.yml` run (or with `gh api repos/mull-project/mull/releases`), not from the agent's sandbox.

### Area 3: the control-voltage law

Presentation: D-012's new bullet, D-027 item 13, D-072 item 3, the S-035 user-guide sentence and L-8 state the divergence plainly, and the owner is asked. The Rack workaround (subtract 2.5 V) is arithmetically right for a centred law with depth 1.0. The success criteria (SC-1, SC-2), the README text of S-035 and the demos (no demo drives a CV input) make no claim about the hardware's CV response. No traceability row claims it either. The problems are in what happens after the owner answers, and in one unresolved fact.

- **PM5-05 The "change to the centred law" route is mis-stated and has no gate branch (High).**
  - Description. If the owner answers L-8 with "change", the plan does not say what to do, and the parts it does name are wrong:
    - G-LISTEN's allowed responses (`proceed`, `iterate`, `extend-rounds`, `proceed-with-rescope`, `stop`, `accept-own-patterns`) have no branch for an answer to L-8. `proceed` plus "change it" would end S-LISTEN with the request unhandled. `iterate` leads to S-LISTEN action 5 and its "On failure" rule: a change that breaks a frozen test is reverted and offered as a test challenge.
    - D-072 item 3 says the change touches "the fixture's port depths and offsets, T-012 and the CV tests of those three effects". The fixture has no offset field (E7-style grep: 0 matches in `effects.json`, `core.hpp`, `gen_tables.py`), and the depth stays 1.0 under the centred law. T-012 exercises only Lowpass and OscBox, which are unaffected. No CV test exists for Delay, FilterBank or Chorus: `grep 'connect ("cv_'` finds only Lowpass, Phaser and OscBox.
    - The real change touches the frozen header `core/include/muff/core.hpp`. Its `PortKind::CvNormalised` comment fixes the additive law, and `PortSpec` has no offset, so either a new `PortKind` or a new field is needed. `HANDOFF.md` §6 classes the four frozen headers as a halt-class public interface. The change also touches `tools/gen_tables.py`, `tools/make_effects_fixture.py`, `tests/core/expected_effects.inc` and T-001, and it needs new tests.
    - Nothing says how an unpatched input behaves under the centred law. The hardware behaves as "knob only", which needs `setInputConnected`, so that a disconnected input does not drag the setting down by half the range.
  - Lens: implementer misinterpretation. Severity: High (Rule 4: the implementer is forced to guess the mechanism, the affected artefacts and the gate branch). Likelihood: medium (it depends on the owner's answer, and the owner is a hobbyist asked about hardware fidelity).
  - Root cause: the option was costed from memory of the fixture, not from its schema.
  - Trace: D-072 item 3, GATES G-LISTEN, S-LISTEN action 5 and "On failure", `HANDOFF.md` §6, `core.hpp:41-55`.
  - Mitigation:
    1. Add a G-LISTEN branch: "`L-8: centred` → write `TEST_CHALLENGE.md` by the amendment route; S-LISTEN is not done until it is resolved or the owner withdraws."
    2. Rewrite D-072 item 3's cost list with the actual artefacts: the `core.hpp` `PortKind`/`PortSpec` change (halt-class, named as pre-approved by the owner's L-8 answer), `gen_tables.py`, `make_effects_fixture.py`, the fixture, `expected_effects.inc`, T-001, T-013 if the behaviour on disconnection changes, and new tests T-4xx/T-5xx/T-7xx for the three effects (one per effect: knob at 0.5, 2.5 V equals no change, 0 V and 5 V reach the ends).
    3. State the rule for unpatched inputs.
    4. Alternatively, pre-decide the mechanism now: a new `PortKind::CvCentred = 4` that applies `depth × (cv − 0.5)` only when connected.
- **PM5-06 Possible inversion of the Delay Time CV is not carried forward (Medium).**
  - Description. The audit (F-3, and row C-PEDB-020) said the MF-104M wording ("0 to +5 V … from longest to shortest") may mean the Time CV runs opposite to the knob, and that this "should be re-read in a manual before deciding". E9: the manual copy does not settle the point either way. The new knowledge item records "not settled". But D-012, D-027 item 13, L-8 and the user-guide sentence of S-035 all present the hardware difference as a pure offset, and they say "subtracting 2.5 V gives the centred response of the hardware". If the Time CV is inverted on the hardware, the workaround and the gate question are both wrong for Delay Time, and the owner decides L-8 on incomplete information.
  - Lens: integrity, misinterpretation. Severity: Medium (degraded fidelity of one control; a workaround, inverting the CV in Rack, exists but is not documented). Likelihood: unknown (a plausible reading of the manual).
  - Root cause: an open question in the audit was dropped in the amendment.
  - Trace: audit F-3 last sentence; C-PEDB-020; D-027 item 13; L-8; S-035.
  - Mitigation: add to D-027 item 13 and L-8: "the Delay manual may also mean that a rising Time CV shortens the delay; not settled". Soften the user-guide sentence to "subtracting 2.5 V centres the response on the knob, as those manuals describe". Add to L-8 a sub-question on direction, if the owner knows the hardware.
- **PM5-07 H-2 omits the knob position.** H-2 asks whether "the CV inputs sweep their controls over the full range with 0 to 5 V". Under the additive law that is true only with the knob fully anticlockwise. An owner who follows hardware practice (knob at noon) sees saturation half-way and may answer "no", which would use up an `iterate` round on intended behaviour.
  - Lens: misinterpretation. Severity: Low. Likelihood: medium.
  - Trace: GATES H-2, D-012.
  - Mitigation: "with the knob fully anticlockwise, does 0 to 5 V sweep the whole range (and with the knob at noon, does it saturate half-way, as the user guide says)?"
- **PM5-08 D-072 item 3 overstates a Rack rule.** "for consistency inside Rack, where an unpatched input must equal 0 V" presents a design preference as a constraint. The core knows connection state (`setInputConnected`, T-013), so a centred law that leaves unpatched inputs at the knob is implementable. D-012's wording ("an unpatched input and a patched 0 V then mean the same thing, which is what Rack users expect") is the accurate one.
  - Lens: integrity (honest option presentation). Severity: Low.
  - Mitigation: replace "must equal" with D-012's wording in D-072 item 3.
- **PM5-09 The lists of jacks without a VST3 counterpart disagree.**
  - A-007 lists "the Phaser's Sweep In, trigger and tap inputs" but omits audio-rate modulation.
  - D-027 item 17 lists audio-rate modulation but omits two others: the OscBox 1 V/oct input, which reaches 20 kHz (T-602) beyond the knob's 1.6 kHz, so automation cannot cover it; and the hardware CV outputs (LFO Out, Envelope Out), which D-016 does not expose.
  - Item 17 says tap inputs have no VST3 counterpart, while D-016 gives the VST3 a tap button.
  - `HANDOFF.md` §8 ("What the plan does not prove") lists neither divergence (CV law, VST3 jacks).
  - Lens: documentation. Severity: Low.
  - Mitigation: one list, in D-027 item 17, cited by A-007 and the user guide. Add a bullet to §8.

### Area 4: other disposition items, lessons and knowledge items

- R-016's remedy names `v3.8.0_build_66`, which exists (E11). R-020 is coherent; its pluginval sentence agrees with round 3 (Y7). The column wording is covered in PM5-16.
- S-034's page-token rule agrees with the OSV documentation (E10), apart from PM5-14.
- S-KNOW's branch is right (E12).
- D-002 and D-037 are corrected accurately, apart from PM5-15. The D-037 figures match the closed form −20·log10|cos(πf/fs)|: 2.0 dB at 10 kHz and 11.7 dB at 20 kHz.
- Secrets and private data: none found in the 7 new files or the amendment (the Appendix C secret regex passes; checked by reading).

- **PM5-10 The new records fail the protocol's own schema check, and S-RETRO's "Done when" requires it to pass (Medium).**
  - Description. E6 gives six problems:
    - `K-20261113T170100Z-mull-two-packages-since-0-36` and `K-20261113T170400Z-rack-2-6-6-builds-as-cpp11` have no `subject:` tag, so Appendix C has no folder to file them in.
    - Four lessons lack the `supersedes` field: the two new ones and two from earlier today, `L-20261112T205000Z…` and `L-20261113T093000Z…`.
    - The four lessons also replace the template's headings (`## Trigger`, `## What went wrong`, `## Correction`, `## Prevention rule`, `## Detection check`, `## Evidence`) with bold paragraphs, and none has `## Evidence`. By contrast, 325 of 327 lessons in the store have it (E13).
    - `domain:ci` (in K-170100Z and L-093000Z) exists in neither store's `TAXONOMY.md` nor in `knowledge/TAXONOMY-ADDITIONS.md`.

    S-RETRO's "Done when" says the schema check of Appendix C passes, and it calls the files in `lessons/` and `knowledge/` "worked examples". As written, S-RETRO cannot finish without editing planner records, and the examples teach the wrong format.
  - Lens: process, past lessons. Severity: Medium. A strict reading could call this High (blocked), but the fix is mechanical and the deviation route covers it. Likelihood: certain (the check fails today).
  - Root cause: the records were written without running the check the plan itself requires.
  - Trace: Appendix A, Appendix B and the Appendix C schema check; S-RETRO "Done when".
  - Mitigation:
    1. Add `supersedes: []` to the four lessons and convert their bodies to the template headings with an `## Evidence` line (commit, file).
    2. Give K-170100Z `subject:mull` and K-170400Z `subject:vcv-plugin-build`. Both subjects already exist in the store.
    3. Replace `domain:ci` with an existing tag (`tech:github-actions` is already on L-093000Z), or add it to the additions file.
    4. Run the check and record its output in `reviews/COLD_READ.md`.
- **PM5-11 Corrected facts leave the false items `current` (Medium).** K-170400Z corrects C-PLANNER-015 but has `supersedes: []`. Two items still say, as current, that the Rack engine needs C++17: this run's `knowledge/K-20261112T120000Z-c-planner-015.md` (`status: current`) and the store's `K-20261009T234214Z-rack-compile-mk-flags-and-cpp17` (E13). The audit's F-5 asked for exactly this supersession. Likewise K-170100Z does not supersede the store's `K-20261027T123200Z-mull-mutation-testing` ("mull-19 … mull-runner-19 then reports") or this run's `c-planner-012` ("newest tag is 0.37.0 … mull-runner-19 from … Cloudsmith"). This run's `c-planner-004` still says the tag list includes 'Hardware clone' "used by tests". That is true, but the item omits the restriction. Under the R0 matching rule, future runs will load both contradictory items as current.
  - Lens: integrity. Severity: Medium (future plans are silently misinformed, but each new item also states the correction). Likelihood: high.
  - Root cause: no step lists which stored items a correcting item replaces.
  - Trace: audit F-5 "Dependents"; Appendix B `supersedes`; S-KNOW.
  - Mitigation:
    1. Set `supersedes` on K-170400Z and K-170100Z to the ids above. Mark the run-local `c-planner-015` (and the partly stale `c-planner-012`) `status: superseded`.
    2. Add to S-KNOW: "for every `supersedes` id that lives in the store, set that file's `status: superseded` in the same commit".
- **PM5-12 Tag choice and source fields.** K-170000Z uses the run's new `subject:plugin-licensing-and-marks`, but the store already has `subject:vcv-plugin-manifest` (which fits exactly) and `subject:vcv-plugin-licensing`, a near-duplicate that Appendix C forbids adding. K-170200Z's `source.url` is the ManualsLib home page, not the copies read: ManualsLib for MF-104M, Gear4Music for MF-105M. K-170300Z has `derived_from: []` and an empty URL, although it corrects context recorded in C-DSP-064.
  - Lens: process. Severity: Low.
  - Mitigation: use `subject:vcv-plugin-manifest` first; give the exact URLs; set `derived_from: [C-DSP-064]`.
- **PM5-13 F-1 has no lesson in fact.** The disposition maps F-1 to `L-20261113T093000Z…`, an action-pin lesson. That lesson was not updated: still `occurrences: 2`, and nothing about tool packaging. D-072's "Lessons" line lists only the two new lessons. The audit asked for a `kind:wrong-assumption` lesson whose trigger is a tool's packaging changing between planning and use (Rule 11).
  - Lens: past lessons. Severity: Low.
  - Mitigation: either a new lesson, or raise L-093000Z's `occurrences` to 3, widen its trigger to "a tool fact recorded without running it", and cite F-1 in its evidence.
- **PM5-14 The page-token rule does not name the request field.** S-034 does not say that the token is sent back as `page_token` with the same query (E10), and it sets no bound on the loop.
  - Lens: misinterpretation. Severity: Low.
  - Mitigation: add "send it back as `page_token` with the same body; stop with an error after 20 pages."
- **PM5-15 Dangling cross-reference in D-002.** D-002 says "S-REVIEW brief 1 checks this" (no `std::filesystem`, `std::optional::value`, `std::variant` visiting or aligned `new`, because of Rack's macOS 10.9 floor). Brief 1 in S-REVIEW does not mention it. The `rack-macos-intel` CI job would fail at compile time on such a use, so the failure is not silent.
  - Lens: documentation. Severity: Low.
  - Mitigation: add the sentence to brief 1.

### Area 5: counts and names

Verified:
- 87 core cases (E5) and 7 VST3 cases.
- 9 Python cases (E1); `HANDOFF.md` §5 and S-FINAL agree.
- 86 in the sanitizer build appears in S-018 (not re-run).
- Frozen manifest intact (E4).
- R-020 is the last risk in `plan/RISKS.md`. D-072 is the highest decision number.

- **PM5-16 Stale ranges and labels.**
  - `HANDOFF.md` line 14 lists the decision groups up to "compatibility D-071". It omits D-072 and so does not say that D-072 is the highest.
  - It also still describes D-027 as "the choices made where the hardware is undocumented", although item 13 now records a deliberate divergence from documented behaviour and item 17 a format gap.
  - `premortem/RISK_REGISTER.md` says "R-001 to R-019".
  - `plan/ENVIRONMENT.md`'s pluginval row says the hashes are not verified, while R-020 says round 3 confirmed them against downloads.
  - In `plan/DECISIONS.md`, D-072 sits between D-071 and D-070.
  - R-020 calls C-DSP-053 "oscillator sync"; the claim is about the polyBLEP residual.
  - Lens: documentation. Severity: Low.
  - Mitigation: add "amendments after the audit D-072 (highest number)" to `HANDOFF.md` line 14. Change "R-019" to "R-020". Update the pluginval "Verified here" cell with a pointer to round 3 Y7. Move D-072 after D-071 and D-070. Correct the R-020 label.

## Risk table

| id | description (short) | lens | severity | likelihood | root cause | trace | proposed mitigation |
|---|---|---|---|---|---|---|---|
| PM5-01 | `modularGridUrl` ban not checked | misinterpretation | Low | low | sentence without check | D-004, S-032, T-902 | add to S-032 and the review brief |
| PM5-02 | Mull hashes are trust-on-first-use | supply chain | Low | low | single hash source | S-MUTATE, D-060 | compare with the REST asset `digest` |
| PM5-03 | example workflow still Cloudsmith, `lld-19`, `mull-runner-19` | misinterpretation | Low | medium | amendment did not list the example's differences | S-MUTATE Inputs | one sentence of overrides |
| PM5-04 | `gh release view` needs GraphQL, blocked in agent sandboxes | operations | Low | medium | environment | S-MUTATE.1 | list assets inside CI or with `gh api` |
| PM5-05 | L-8 "change" route: no gate branch; D-072 names absent fields and wrong tests; frozen header change and unpatched rule unstated | misinterpretation | High | medium | option costed without reading the fixture schema | D-072 item 3, G-LISTEN, `core.hpp:41-55`, `HANDOFF.md` §6 | gate branch, correct artefact list, `CvCentred` mechanism, rule for unpatched inputs |
| PM5-06 | possible Delay Time CV inversion dropped; workaround and L-8 may be wrong | integrity | Medium | unknown | open audit question not carried | F-3, C-PEDB-020, D-027 item 13, L-8, S-035 | record as open; soften the user-guide sentence; ask the owner |
| PM5-07 | H-2 omits the knob position | misinterpretation | Low | medium | wording | H-2, D-012 | specify the knob position |
| PM5-08 | D-072 item 3 states a preference as a Rack rule | integrity | Low | — | wording | D-072, D-012 | use D-012's wording |
| PM5-09 | A-007 and item 17 jack lists disagree; 1 V/oct range and CV outputs missing | documentation | Low | — | two lists | A-007, D-027 item 17, D-016, `HANDOFF.md` §8 | one list |
| PM5-10 | schema check fails (6); template headings and Evidence missing; `domain:ci` unknown; S-RETRO "Done when" unmeetable | process | Medium | certain | check not run | Appendices A to C, S-RETRO | fix fields, tags and headings; run the check |
| PM5-11 | corrected facts leave false items `current` (run and store) | integrity | Medium | high | no supersession step | F-5, Appendix B, S-KNOW | set `supersedes`; mark superseded |
| PM5-12 | near-duplicate subject tag; imprecise sources | process | Low | — | — | Appendix C, K-170000Z, K-170200Z, K-170300Z | existing subjects, exact URLs |
| PM5-13 | F-1 lesson not recorded or updated | past lessons | Low | — | — | disposition table, D-072 | new lesson or update L-093000Z |
| PM5-14 | `page_token` field and loop bound unstated | misinterpretation | Low | low | — | S-034 | name the field; add a bound |
| PM5-15 | D-002 points to a brief-1 check that does not exist | documentation | Low | — | — | D-002, S-REVIEW | add the sentence |
| PM5-16 | `HANDOFF.md` omits D-072; RISK_REGISTER says R-019; ENVIRONMENT pluginval cell; order; R-020 label | documentation | Low | — | — | `HANDOFF.md`:14, `premortem/RISK_REGISTER.md`, `plan/ENVIRONMENT.md` | update |

## Counts

- Frozen suite: 87 core cases (86 sanitizer, per PLAN), 7 VST3, 9 Python; manifest intact (21 files).
- Decisions D-001 to D-072 (D-072 highest; `HANDOFF.md` does not say so). Risks R-001 to R-020 (`premortem/RISK_REGISTER.md` says R-019).
- Findings this round: 16.

CRITICAL: 0 HIGH: 1 MEDIUM: 3 LOW: 12

---

## Planner's disposition (added after the round; the text above is the reviewer's report, unedited)

| Item | Severity | Disposition | Where |
|---|---|---|---|
| PM5-05 | High | Fixed by removing the unplanned option: this plan does not implement the centred law under any gate answer; L-8 is for information, and a request for the change is recorded as follow-up planning work. D-072 item 3 no longer names artefacts that do not exist. | D-072 item 3, `plan/GATES.md` L-8 |
| PM5-06 | Medium | Fixed: the open point about the delay's Time input is carried in D-072, D-027 item 13 and the knowledge item; no document claims the 2.5 V offset reproduces the hardware exactly. | D-012, D-027, S-035, `K-20261113T170200Z` |
| PM5-10 | Medium | Fixed: lessons rewritten to the template with `supersedes` and Evidence; knowledge items given `subject:` tags; unknown tag replaced; the protocol's schema check now prints "all entries valid". | `lessons/`, `knowledge/` |
| PM5-11 | Medium | Fixed: this run's two wrong items are `superseded`; the new items name them and the two store items in `supersedes`; S-KNOW marks the store items. | `knowledge/`, S-KNOW |
| PM5-01, PM5-15 | Low | Fixed: S-032 forbids `modularGridUrl`; S-REVIEW brief 1 checks it and the C++17 library limits. | S-032, S-REVIEW |
| PM5-02, PM5-04 | Low | Fixed: assets listed in CI through the REST interface; digest compared where given. | S-MUTATE |
| PM5-03 | Low | Fixed: the step states where it overrides the copied example. | S-MUTATE |
| PM5-07 | Low | Fixed: H-2 names the knob position. | `plan/GATES.md` |
| PM5-08 | Low | Fixed: stated as this project's preference. | D-072 |
| PM5-09 | Low | Fixed: one list, in D-027 item 17. | A-007, D-027 |
| PM5-12 | Low | Fixed: sources made exact, tags from the taxonomy. | `knowledge/` |
| PM5-13 | Low | Fixed: the pin lesson's Evidence records the Mull recurrence. | `L-20261113T093000Z` |
| PM5-14 | Low | Fixed: field named, bound of 20 pages. | S-034 |
| PM5-16 | Low | Fixed. | `HANDOFF.md`, `premortem/RISK_REGISTER.md`, `plan/ENVIRONMENT.md` |

Result of round 5: 0 Critical, 1 High, fixed. A short sixth round verifies the fix.
