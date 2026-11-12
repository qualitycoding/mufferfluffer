# HANDOFF — mufferfluffer: seven analogue-style effects as VST3 plug-ins and a VCV Rack 2 plug-in

Generation branch: `gen-20261111T151412Z-mufferfluffer-vst-vcv`. Protocol: `qualitycoding/skills` `protocols/planning-protocol.md` v3.15. You are the implementer. Everything you need is on this branch; ask the owner nothing except at the three gates of section 5.

## 1. Purpose
Build an original software reimplementation of the behaviour of seven analogue effect pedals (a low-pass filter, a ring modulator, a phaser, a delay, a resonant filter bank with a pattern sequencer, an oscillator box and a chorus/flanger) as one shared C++ DSP core, delivered as seven VST3 plug-ins (JUCE) and one VCV Rack 2 plug-in with seven modules, under this project's own names and panel design. Scope, success criteria SC-1 to SC-10 and the owner's choices are in `plan/ASSUMPTIONS.md`.

## 2. Active profiles
`software` with the `audio` mode flag; `software.deploys` is false. Sections and artefacts that do not apply are listed in `plan/PROFILE.md`; do not create them.

## 3. Reading order
1. This file.
2. `plan/ASSUMPTIONS.md` (what was asked and agreed).
3. `plan/DECISIONS.md` (every design decision, D-001 to D-070; D-027 lists the choices made where the hardware is undocumented).
4. `plan/ENVIRONMENT.md` (pins and exact commands).
5. `plan/PLAN.md` (the steps, in order).
6. `plan/GATES.md`, `plan/RISKS.md`, `plan/TRACEABILITY.md`.
7. The frozen interfaces: `core/include/muff/core.hpp`, `core/include/muff/blocks.hpp`, `adapters/include/muff/adapters.hpp`, `vst3/include/muff/vst3_shell.hpp`; the fixture `tests/fixtures/effects.json`.
8. `research/PRIOR_KNOWLEDGE.md` (lessons from earlier runs, mapped to steps) when a step's "Lessons applied" points there.
Reference only: `research/` (claims, sources, spikes), `premortem/`, `reviews/`.

## 4. Environment, tests and the freeze
- Setup and every command: `plan/ENVIRONMENT.md`. First step: S-000.
- The frozen suite: `muff_core_tests` (80 Catch2 cases: `tests/core/`), `muff_vst3_tests` (6 cases: `tests/vst3/`), `tests/py/test_repo.py` (9 `unittest` cases), plus the benchmark `muff_bench` (`tests/perf/`). How to run each: `plan/ENVIRONMENT.md`, section "Commands".
- The freeze: `python3 tools/check_frozen.py` verifies `tests/FROZEN_MANIFEST.sha256` (tests, fixtures, the four public headers and three generator tools). It must exit 0 before every commit. Never edit a frozen file, never add a file under `tests/`, never change a tolerance. The only route to a test change is section 6.
- Stubs: `core/src/stubs.cpp`, `adapters/src/stubs.cpp` and `vst3/src/stubs.cpp` make the tests build and fail; steps S-010, S-018 and S-020 delete them.
- The oracle: `research/spikes/oracle/`, `research/spikes/oracle_vst3/` and `research/spikes/oracle_repo/` are the planner's proof that the tests can pass (`research/spikes/red-verification/RESULT.md`). They are a reference for what the tests accept, not the deliverable and not a quality bar: build the structure the steps describe, and do not ship files from `research/`.

## 5. Steps at a glance and human gates
S-000 environment; S-001 build skeleton and CI; S-010 core foundation; S-011 to S-017 the seven effects; S-018 adaptors and the whole core suite; S-020 and S-021 VST3; S-030 to S-033 Rack; S-034 dependency scan; S-035 documentation; S-REVIEW; S-ADVERSARIAL; S-MUTATE; S-040 (gate G-002); S-LISTEN (gate G-LISTEN); S-FINAL; S-RETRO; S-KNOW (gate G-003). The plan begins with S-000 and ends with the closing steps S-RETRO and S-KNOW.
Gates (`plan/GATES.md`): **G-002** before any built binary is uploaded to the public repository; **G-LISTEN** the owner listens to the demo bundle (at most three iterate rounds); **G-003** before lessons and knowledge are pushed to the stores. While halted at a gate, continue any step that does not depend on its outcome; never take an external or irreversible action without its gate.

## 6. Halting, deviations and test challenges
- **Default rule for anything the plan did not anticipate:** choose the most reversible option that does not expand scope, log it in `DEVIATIONS.md` with the reason, and continue, unless it touches frozen tests, security, data integrity, a public interface (the four frozen headers, parameter ids, the Rack manifest slugs) or research integrity, in which case halt and write `BLOCKED.md`.
- **`BLOCKED.md`** states: the step, what was attempted (commands and output), what is missing or contradictory, and the options you see. Commit, push, stop that line of work.
- **A frozen test looks wrong** (it contradicts a decision, cannot be met, or a review needs a new test): do not touch it. Write `TEST_CHALLENGE.md` with the test id, the evidence (measured values, the decision text it contradicts) and the proposed change. A fresh-context agent of at least Opus tier that did not write the code decides: if it upholds the challenge, that agent (not you) edits the test, shows it red against the stubbed or reverted code path and green against the fix, runs `python3 tools/check_frozen.py --write`, and commits with the message prefix `REFREEZE:` naming the test ids; the change is logged in `DEVIATIONS.md` and as a lesson. If it rejects the challenge, implement to the test.
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

During the run, lessons accumulate in `lessons/` and knowledge items in `knowledge/` on your branch (the planner's are already there). At S-KNOW, after gate G-003, lessons go to the lesson store `qualitycoding/Lessons` and knowledge items, including the source register `plan/SOURCE_REGISTER.md`, go to the knowledge store `qualitycoding/knowledge`.

## 8. What the plan does not prove (read before promising anything)
- Fidelity to the hardware is judged by the owner's ear on demos, not by a blind comparison: no reference recording could be obtained (D-040, R-004).
- The filter bank's patterns are this project's own (D-024, R-003).
- Many constants are design choices because the hardware is undocumented (D-027).
- The Rack modules are built and packaged on four platforms but never run inside Rack by an automated test (R-012).
- pluginval, the Rack SDK download, Mull and the OSV scan were not executed during planning (hosts unreachable from the planning sandbox); their first runs are in S-021, S-030, S-MUTATE and S-034, each with a pre-decided response to failure.
