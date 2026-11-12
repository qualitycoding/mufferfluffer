# HANDOFF — mufferfluffer: seven analogue-style effects as VST3 plug-ins and a VCV Rack 2 plug-in

Generation branch: `gen-20261111T151412Z-mufferfluffer-vst-vcv`. Protocol: `qualitycoding/skills` `protocols/planning-protocol.md` v3.15, copied to `plan/protocol/planning-protocol.md` together with the three skills the plan uses (`plan/protocol/README.md`). You are the implementer. Everything you need is on this branch; ask the owner nothing except at the three gates of section 5.

## 1. Purpose
Build an original software reimplementation of the behaviour of seven analogue effect pedals (a low-pass filter, a ring modulator, a phaser, a delay, a resonant filter bank with a pattern sequencer, an oscillator box and a chorus/flanger) as one shared C++ DSP core, delivered as seven VST3 plug-ins (JUCE) and one VCV Rack 2 plug-in with seven modules, under this project's own names and panel design. Scope, success criteria SC-1 to SC-10 and the owner's choices are in `plan/ASSUMPTIONS.md`.

## 2. Active profiles
`software` with the `audio` mode flag; `software.deploys` is false. Sections and artefacts that do not apply are listed in `plan/PROFILE.md`; do not create them.

## 3. Reading order
1. This file.
2. `plan/ASSUMPTIONS.md` (what was asked and agreed).
3. `plan/DECISIONS.md` (every design decision: project and build D-001 to D-018, effect behaviour D-020 to D-027, algorithms D-030 to D-037, gates and quality D-040, D-041, D-050, D-060 to D-062, versions D-070; the numbers in between are unused; D-027 lists the choices made where the hardware is undocumented).
4. `plan/ENVIRONMENT.md` (pins and exact commands).
5. `plan/PLAN.md` (the steps, in order).
6. `plan/GATES.md`, `plan/RISKS.md`, `plan/TRACEABILITY.md`.
7. The frozen interfaces: `core/include/muff/core.hpp`, `core/include/muff/blocks.hpp`, `adapters/include/muff/adapters.hpp`, `vst3/include/muff/vst3_shell.hpp`; the fixture `tests/fixtures/effects.json`.
8. `research/PRIOR_KNOWLEDGE.md` (lessons from earlier runs, mapped to steps) when a step's "Lessons applied" points there.
9. Section 9 of this file (working conventions) before S-000.
Reference only: `research/` (claims, sources, spikes, the planner's own tools in `research/tools/`), `premortem/` and `reviews/COLD_READ.md` (the planner's review records), `plan/protocol/` (consult where a step cites it). You add your own review records to `reviews/`.

## 4. Environment, tests and the freeze
- Setup and every command: `plan/ENVIRONMENT.md`. First step: S-000.
- The frozen suite: `muff_core_tests` (83 Catch2 cases: `tests/core/`), `muff_vst3_tests` (6 cases: `tests/vst3/`), `tests/py/test_repo.py` (9 `unittest` cases), plus the benchmark `muff_bench` (`tests/perf/`). How to run each: `plan/ENVIRONMENT.md`, section "Commands".
- The freeze: `python3 tools/check_frozen.py` verifies `tests/FROZEN_MANIFEST.sha256` (tests, fixtures, the four public headers and four tools: the three generators and `tools/check_frozen.py` itself). It must exit 0 before every commit. Never edit a frozen file, never add a file under `tests/`, never change a tolerance. The only routes to a test change are section 6 (a test looks wrong) and section 9.4 (a test must be added).
- Stubs: `core/src/stubs.cpp`, `adapters/src/stubs.cpp` and `vst3/src/stubs.cpp` make the tests build and fail; steps S-010, S-018 and S-020 delete them.
- The oracle: `research/spikes/oracle/`, `research/spikes/oracle_vst3/` and `research/spikes/oracle_repo/` are the planner's proof that the tests can pass (`research/spikes/red-verification/RESULT.md`). They are a reference for what the tests accept, not the deliverable and not a quality bar: build the structure the steps describe, and do not ship files from `research/`.

## 5. Steps at a glance and human gates
S-000 environment; S-001 build skeleton and CI; S-010 core foundation; S-011 to S-017 the seven effects; S-018 adaptors and the whole core suite; S-020 and S-021 VST3; S-030 to S-033 Rack; S-034 dependency scan; S-035 documentation; S-REVIEW; S-ADVERSARIAL; S-MUTATE; S-040 (gate G-002); S-LISTEN (gate G-LISTEN); S-FINAL; S-RETRO; S-KNOW (gate G-003). The plan begins with S-000 and ends with the closing steps S-RETRO and S-KNOW.
Gates (`plan/GATES.md`): **G-002** before any built binary is uploaded to the public repository; **G-LISTEN** the owner listens to the demo bundle (at most three iterate rounds unless the owner grants more); **G-003** before lessons and knowledge are pushed to the stores. While halted at a gate, continue any step that does not depend on its outcome; never take an external or irreversible action without its gate.

## 6. Halting, deviations and test challenges
- **Default rule for anything the plan did not anticipate:** choose the most reversible option that does not expand scope, log it in `DEVIATIONS.md` with the reason, and continue, unless it touches frozen tests, security, data integrity, a public interface (the four frozen headers, parameter ids, the Rack manifest slugs) or research integrity, in which case halt and write `BLOCKED.md`. To make that judgement mechanical: *halt* covers any edit to a file in `tests/FROZEN_MANIFEST.sha256`, any change to `tests/fixtures/effects.json`-derived names, ids, ranges or defaults, to the state format of `vst3_shell.hpp`, to the pins of `plan/ENVIRONMENT.md` or the hashes in `deps.lock.json` (other than the recording steps S-030 and S-MUTATE.1), to workflow permissions or triggers beyond what a step states, and anything that would publish a binary or third-party material. *Continue with a `DEVIATIONS.md` row* covers everything else, for example: Rack panel and editor layout details, `rack/Makefile` and CMake wiring, private helper files and their names, tool command-line details, timeouts, package names, runner paths.
- **`BLOCKED.md`** states: the step, what was attempted (commands and output), what is missing or contradictory, and the options you see. Commit, push, stop that line of work.
- **A frozen test looks wrong** (it contradicts a decision or cannot be met): do not touch it. Write `TEST_CHALLENGE.md` with the test id, the evidence (measured values, the decision text it contradicts) and the proposed change. A fresh-context agent of at least Opus tier that did not write the code (section 9.1) decides, after re-reading the decision and the claims behind the test (`plan/TRACEABILITY.md` maps test to decision to claim): if it upholds the challenge, that agent (not you) edits the test, shows the corrected test red against a deliberately wrong build (the code with the behaviour under test disabled or reverted) and green against the fix, runs `python3 tools/check_frozen.py --write`, and commits with the message prefix `REFREEZE:` naming the test ids; the change is logged in `DEVIATIONS.md` and as a lesson. If it rejects the challenge, implement to the test.
- **Amendment route** (cited by `plan/RISKS.md` where a response needs a frozen value, a fixture entry or a decision changed, for example a hardware figure found wrong or the owner supplying patterns or recordings): the same as a test challenge. Write `TEST_CHALLENGE.md` naming the fixture entries, decisions and tests affected (found through `plan/TRACEABILITY.md`) and the evidence; the fresh-context agent described above makes the change to fixture, generated files and tests together, re-verifies red and green, re-freezes with a `REFREEZE:` commit, and records the amendment as a new decision at the end of `plan/DECISIONS.md` (next free number after D-070). You then implement to the amended tests.
- Every substitution (a version, a package, a timeout, a configuration file not named in the plan) is a `DEVIATIONS.md` row.
- `EXECUTION_LOG.md`: one line per step attempt (UTC time, step, attempt number, `pass`/`fail`/`blocked`, commit, note).

## 7. Rules restated verbatim from the protocol
Rule 9 (integrity):

> 9. **Integrity** `[All]`: No step may fabricate, cherry-pick without disclosure, or manually alter data, test results, benchmarks, or figures. In addition:
>    * `[computational, publication]` Every reported number is generated from committed results, not transcribed by hand.
>    * `[publication]` Generative-AI images are never used as data figures. AI assistance is disclosed according to the venue's policy, as recorded in `plan/ASSUMPTIONS.md`.

Rule 6 (everything is committed, including the push cadence). The owner added on 2026-11-11: "Push the plan and artefacts as each phase is finalised so we don't lose work" (A-015).

> 6. **Everything Is Committed** `[All]`: No finding, decision, proof, figure spec, or artifact exists unless it is committed to the generation branch.
>     * **Push cadence** `[All]`, on **every** platform (stable and ephemeral): after every successful chunk — a plan step turning green, a frozen-test amendment round closed, a review or gate record committed — immediately push the working branch to the remote, then verify with `ls-remote` that the remote matches local HEAD. Retry transient failures with backoff (on flaky networks prefer HTTP/1.1 with a raised `http.postBuffer`); report any remote/local mismatch instead of assuming the push landed. On ephemeral platforms this is the primary defense against losing work; on stable platforms it is cheap insurance and keeps collaborators and CI current.

Rule 11 (lessons):

> 11. **Lessons** `[All]`: Whenever the executing agent corrects its own execution, it records a lesson before continuing. The *executing agent* is the planning agent while it runs this protocol, and the implementing agent while it runs the plan. A *correction* is any of:
>     * redoing or reverting something the agent did;
>     * a command, assumption, or edit that failed and was fixed by taking a different approach;
>     * a check (test, cold read, proof review, pre-mortem, CI) that caught the agent's own mistake;
>     * a correction from the human.
>
>     Retrying the same action after a transient failure is not a correction, and neither is a failure the protocol plans for (such as tests failing red against stubs).
>     * Each lesson is one file, `lessons/L-<YYYYMMDDTHHMMSSZ>-<slug>.md`, on the working branch, using the template in Appendix A. It is committed before work continues.
>     * A lesson carries the *identifying information* that lets a future agent recognise the situation before repeating the mistake: the trigger (the circumstances and observable signals), the mistake, the correction, a prevention rule written as one imperative, checkable instruction, and a detection check.
>     * Every lesson is classified with tags from the store taxonomy (Appendix C), so lessons can be read as a group: for example, all `tech:java` lessons, or all `domain:audio` lessons.
>     * If the mistake repeats a lesson loaded in R0, the new lesson names it in `recurrence_of` and strengthens its prevention rule; a recurrence means the earlier rule did not work.
>     * If the same mistake recurs within the run, increase `occurrences` on the run's existing lesson and add the new evidence, instead of writing a second file.

Rule 12 (knowledge base):

> 12. **Knowledge Base** `[All]`: All supporting research and derived knowledge is recorded as knowledge items, so later work can reuse it instead of researching it again. Whenever either agent discovers a new fact about the task's Subject (its technologies, formats, APIs, platforms, tools, mathematical objects, or venues), it adds an item before continuing.
>     * Each item is one file, `knowledge/K-<YYYYMMDDTHHMMSSZ>-<slug>.md`, on the working branch, using the template in Appendix B. It records the fact, its source and locator, the version it applies to, its confidence, and the claim it derives from (`C-###`), if any.
>     * Every item is classified with tags from the store taxonomy (Appendix C), including at least one `subject:` tag, so it can be found again.
>     * A fact that turns out wrong or outdated is never edited away: a new item `supersedes` it, so the history remains.
>     * The sources behind the facts are recorded too. At the end of the research rounds (R7), one **source register** item lists every external source checked during the run, with its content, licence and terms of use (Appendix B). A source that is not in the register has not been checked, as far as a later run is concerned.

Rule 13 (stores):

> 13. **Lesson & Knowledge Stores** `[All]`: Lessons and knowledge items persist across tasks in separate stores recorded at intake (`A-###`; layout in Appendix C). The default lesson store is a repository named `Lessons`; the default knowledge store is a repository named `knowledge`, both owned by the same account as the target repository. Stores are read at the start of research (R0) and written only at the end of implementation, after gate `G-003` (3.7.2); lessons go to the lesson store and knowledge items go to the knowledge store. During a run, new lessons and items accumulate on the working branch. Lessons and items must never contain credentials, tokens, secrets, or personal data; redact them (for example, `github_pat_***`).

During the run, lessons accumulate in `lessons/` and knowledge items in `knowledge/` on your branch (the planner's are already there). At S-KNOW, after gate G-003, lessons go to the lesson store `qualitycoding/Lessons` and knowledge items, including the source register (the item `knowledge/K-20261112T130000Z-source-register-mufferfluffer.md`, generated from its master copy `plan/SOURCE_REGISTER.md`), go to the knowledge store `qualitycoding/knowledge`.

Rule 4 (severity rubric; the lines for the `software` profile and for all profiles), used by S-REVIEW, S-ADVERSARIAL and `plan/RISKS.md`:

> * **Critical:** `[All]` Data loss or corruption; silently wrong results; unrecoverable state; a verification that reported success without having run (swallowed failure, missing artifact, non-gating budget — Rule 14), which is treated as a silently wrong result. `[software]` Security exposure.
> * **High:** `[All]` The implementer is blocked or forced to guess; a frozen test proves invalid. `[software]` A feature fails with no workaround.
> * **Medium:** `[All]` Degraded behavior or presentation with a documented workaround.
> * **Low:** `[All]` Cosmetic, typographic, or documentation-only.

Rule 14 (honest verification) is applied by the plan's "Done when" lines; its full text, and every other rule, is in `plan/protocol/planning-protocol.md` under "Global Rules".

## 8. What the plan does not prove (read before promising anything)
- Fidelity to the hardware is judged by the owner's ear on demos, not by a blind comparison: no reference recording could be obtained (D-040, R-004).
- The filter bank's patterns are this project's own (D-024, R-003).
- Many constants are design choices because the hardware is undocumented (D-027).
- The Rack modules are built and packaged on four platforms but never run inside Rack by an automated test (R-012).
- pluginval, the Rack SDK download, Mull and the OSV scan were not executed during planning (hosts unreachable from the planning sandbox); their first runs are in S-021, S-030, S-MUTATE and S-034, each with a pre-decided response to failure in its step's "On failure" line.

## 9. Working conventions every step relies on

### 9.1 Agents and tiers
- A step's Tier (Haiku, Sonnet, Opus) is the least capable Claude model class that may carry it out. If you are at or above that tier, do the step yourself; if you are below it, hand the step to a sub-agent of that tier or, if you cannot, write `BLOCKED.md` naming the step and the tier.
- "Fresh-context agent" (S-REVIEW, S-ADVERSARIAL, S-RETRO, sections 6 and 9.4) means a sub-agent you start with whatever agent-spawning tool your environment has, given only a clone of the branch and the brief the step states, and nothing of your conversation. If your environment has no such tool, do not review your own work: write `BLOCKED.md` for that step asking the owner to run it in a new session with the step's brief, and continue with every step that does not depend on it. S-000 records which case applies.

### 9.2 Gates: asking, answers and waiting
- To ask: commit and push `GATE-<id>.md`, then put the gate's question, its allowed responses and the file's URL in your message to the owner in the session you were started from (that message is the notification; there is no other channel).
- An answer arrives either as the owner's reply in that session or as a file `GATE-<id>.RESPONSE.md` the owner commits to `impl-20261111T151412Z-mufferfluffer-vst-vcv` (run `git fetch` and look before concluding there is none). Copy a reply verbatim into `GATE-<id>.RESPONSE.md` under `## Answer`. Commit that file in the same commit as the first change that applies the answer. Facts that exist only afterwards (a run URL, checksums, store commit hashes, bundle paths) are appended to the same file under a second heading `## Record` in a later commit.
- Waiting: never wait idle. Continue every step that does not depend on the gate. When no such step is left, write an `EXECUTION_LOG.md` line with outcome `blocked` and the note `waiting at <gate>`, push, and end your turn with the question. Work resumes when the owner answers. "No response" in a gate's branches means exactly this state: nothing left to do but wait. G-002 alone never causes that state (its "no response" branch lets the work continue as `decline`). At G-003 the two `git bundle` files are written when the step halts; with no response, your final message reports their paths as well as the question.

### 9.3 Records at the repository root
`EXECUTION_LOG.md`, `DEVIATIONS.md`, `BLOCKED.md`, `TEST_CHALLENGE.md`, `GATE-<id>.md`, `GATE-<id>.RESPONSE.md`, `COMPLETION.md`, `RETROSPECTIVE.md`, `CHANGELOG.md` and everything under `docs/` and `demos/` are scanned by test T-901, which fails on the manufacturer's name, its product-family name or a model code. In those files refer to effects by this project's names and to planning records by id (for example "D-004", "R-013"); text that must name the hardware goes in a file under `reviews/` or `research/`.

### 9.4 Adding a frozen test during implementation
Needed when S-REVIEW, S-ADVERSARIAL or S-MUTATE calls for a new test. A fresh-context agent of at least Opus tier that did not write the code under test (9.1):
1. adds the test case to the existing file under `tests/` that holds its neighbours (same id scheme: the next free number in the effect's or area's block; same tags), changing no existing case;
2. shows it **red** against a build with the defect: for a bug, the commit before the fix; for a surviving mutant, the source with that mutant applied by hand in a scratch worktree; and **green** against the corrected code;
3. runs `python3 tools/check_frozen.py --write`, and commits tests, manifest and (for a bug) the fix with a subject starting `REFREEZE:` that names the new test ids;
4. adds a `DEVIATIONS.md` row with the new ids and the new totals (core cases, sanitizer-build cases, VST3 cases, Python cases), a row in `plan/TRACEABILITY.md`, and a lesson if the test exposes an implementer mistake.
From then on every count in `plan/PLAN.md` means the newest totals in `DEVIATIONS.md`. One agent may add several tests for one source file in one commit.

### 9.5 Licence headers
Every `.cpp`, `.hpp` and `.h` file you create under `core/`, `adapters/`, `vst3/` or `rack/src/` (other than generated files, whose names contain `generated`) carries `// SPDX-License-Identifier: AGPL-3.0-only` within its first six lines (T-906). Python tools and workflow files start with a comment line `# SPDX-License-Identifier: AGPL-3.0-only`.

### 9.6 Commit-message markers
Four workflows start only when the head commit's subject line **starts with** a marker: `[record-rack-sdk]` (S-030), `[mutation full]` (S-MUTATE), `[build-packages G-002-approved]` (S-040, only after the owner's `proceed`), `[gate-evidence G-LISTEN]` (S-LISTEN). The workflows test this with `startsWith(github.event.head_commit.message, '<marker>')`. No other commit subject may start with one of these strings; mentioning a marker later in a message is harmless. When several commits are pushed at once only the last is the head commit, so push a marker commit on its own.

### 9.7 `.checkpoints/state.json`
Keep every existing key. Set `"phase": "impl"`; set `"active_step_id"` to the step in hand; append each finished step id to the list `"completed_steps"` (create it); add an object `"implementer"` with the keys `os`, `compiler`, `cmake`, `python`, `nproc`, `can_spawn_agents` and `unreachable_hosts` (S-000); put the values a step's Checkpoint line names in an object `"step_facts"` keyed by step id. Files under `.checkpoints/snapshots/` are the planner's; you need not add any.
