---
name: mutation-testing-in-ci
description: Use when planning, adding or fixing mutation testing (Mull, Stryker, PIT, mutmut or similar) in CI - deciding when the job runs (path-scoped incremental runs on pull requests, scheduled full runs, on-demand runs), keeping the score gate honest (computed from the report, cannot be skipped silently), controlling runtime, and handling survivors. Also use when a plan includes an S-MUTATE step or a mutation job is slow, flaky, always green or always skipped.
---

# Mutation testing in CI

Mutation testing re-runs the tests once per mutant, so its cost is `mutants x test time / workers`, plus a build. That cost, not the idea, decides how it is wired into CI. The wiring below keeps the score gate real without paying the full price on every push.

## 1. Price it before choosing anything

Measure, do not guess, and write the numbers into the plan:

1. **Baseline test time** of the exact executable and test list the tool will run, built the way the tool needs it (instrumented, usually unoptimised). Reference point (guitar-amp-vst, 2026-11): the full suite took 12.5 minutes per run instrumented, versus about 2 minutes in a normal Release build; the tool's own warm-up run failed at its default 3-second timeout.
2. **Mutant count** for the in-scope files (reference: 154 to 174 mutants over 9 files).
3. **Estimate**: `mutants x per-mutant test time / workers`, plus the instrumented build time. If it exceeds the runtime budget, shrink the test list (fast tests only), the scope (fewer files) or move the job off the per-push path. Reference: 66 to 150 minutes for the 154 to 174 mutants on 4 workers with a fast-test subset, and about 10 minutes of that was the build.
4. **Floor**: choose the score floor before the first run and read the first real score before declaring the job done. Reference: the first real run scored 37.7 %; a job that had been green for several runs had never executed a mutant.

## 2. Three tiers, one required check

| Tier | Trigger | Scope | Purpose |
|---|---|---|---|
| 1 | pull request, and push to the default branch | only changed mutable source files (incremental) | fast feedback on the code being changed; the floor applies to those files |
| 2 | schedule (daily or weekly) on the default branch | all in-scope files | backstop: catches erosion across many small changes |
| 3 | manual dispatch | full or incremental, caller's choice | before a release, a protocol gate, or after changing the test list |

Rules that make the tiers trustworthy:

- **Do not path-filter the workflow itself** if its status is required. A workflow filtered by `paths:` does not run at all when no listed file changed, so a required check from it stays pending. Run always; let a scope step decide `incremental`, `full` or `not-applicable`.
- **One aggregator job is the only required status.** It runs with `if: always()`, depends on the mutation job, prints the mode, and fails unless the mutation job succeeded. `not-applicable` (no mutable file changed) is a success that says so in the log; a cancelled, skipped-by-error or failed job is never a success.
- **Scheduled and manual runs must have an owner.** A scheduled failure opens an issue (or notifies a named channel); a scheduled job nobody reads is the same hole as swallowed errors.
- Enforcing "required" is a repository setting (branch protection or rulesets). Check it from the repository's settings; a workflow file cannot make a check required.
- **Widen to a full run** when the change touches the tests that kill mutants, the mutation config, or the scripts of this job: any of those can change every file's score.
- **Gate an incremental run per file against a recorded baseline**, not against the project floor. Some files legitimately score under the floor (their survivors are justified), so a floor applied to a single changed file would fail every change to it. Record the per-file scores from a full run (`mull_score.py ... --write-baseline`), commit the file, and gate with `--baseline` (each changed file may fall at most `--tolerance` points below its recorded score; a file missing from the baseline must reach the floor). The full run keeps the project floor. Refresh the baseline from a full run when scores legitimately move; raising a baseline is free, lowering one is a reviewed decision.
- Incremental scope is generated, not hand-edited: `scripts/mull_include_paths.py` turns `git diff --name-only BASE...HEAD` into a tool config that mutates only those files, and exits 3 when none are mutable.

`references/example-mutation-workflow.yml` is the working workflow from the project this skill came from (one instance; its PROJECT-SPECIFIC inputs are listed in its header). Its not-applicable and incremental paths ran on 2026-11-01 (a documentation-only push finished in about 1 minute; a one-file change mutated only that file's 9 mutants in about 11 minutes, against about 150 minutes for the full run). The full-run path under that file, the nightly schedule and a failing gate in that workflow were unobserved at that date. Run the whole thing once on a throwaway branch and read the numbers before relying on it.

## 3. Keep the gate honest

A check that cannot fail is not a check. For mutation jobs specifically:

- **Compute the score from the report file**, not from the tool's exit status. In mull-runner-19 (2026-11), `--mutation-score-threshold 75` did not fail a 37.7 % run that also passed `--allow-surviving`. `scripts/mull_score.py` reads the SQLite report, prints a per-file table (worst file first), and exits non-zero below `--floor` (project score) or, with `--baseline`, when a file falls below its recorded score.
- **Post-conditions**: the report exists, is non-empty, and holds at least a minimum number of mutants (`--min-mutants`). A tool error that produced no report must fail, not skip.
- **No swallowed failures** on the runner command: no `|| true`, no `continue-on-error`. With a pipe into `tee`, set `pipefail` (the default shell of GitHub Actions `run` steps has none, observed 2026-11).
- **Verify the status code for "survived"** against a report with a known survivor: the count printed by the script must equal the tool's own "Surviving mutants" line.
- **Upload the report and log always** (`if: always()`, `if-no-files-found: error`), so a failed run can be diagnosed without re-running 2 hours.
- **Show it can fail**: once, run the gate against a report with a score below the floor, and read the red result.

## 4. Controlling runtime

- Run only the fast tests: pass test names to the runner and leave out multi-second tests (long audio renders, fuzzing that needs a generated corpus, anything that needs a fixture the tool does not set up). Record the omission as a deviation: mutants only the omitted tests would kill count as survivors, so the score is conservative.
- Set the tool's timeouts explicitly (warm-up and per-mutant) and `--workers` to the core count.
- Skip a separate dry run if it executes the baseline too: that doubles the baseline cost.
- Tests that sleep or poll threads dominate per-mutant time; shorten the waits or keep those tests out of the mutation list and cover them by other jobs.
- A normal-speed unit job (sanitizers, plain tests) should cover everything the mutation list leaves out.

## 5. Working the survivors

1. Group survivors by file and sort by file score, **worst file first**. A file at 0 to 10 % usually means a stage no test exercises, or a defect: in the reference project the lowest-scoring file hid a gate that never attenuated anything.
2. Write a behaviour test from the unit's documentation before reading individual mutants: a reference test that restates the intended behaviour independently, an exact-value test, or a state-transition test.
3. Classify what remains and justify per class in a review document: loop-bound off-by-one that only a sanitizer sees (cover it with a sanitizer job instead), exact-boundary comparisons on floating-point values, framework default state that makes a comparison constant, equivalent arithmetic, threaded timing. Treat the justifications as claims to be checked by a person, not as proof.
4. Re-run and read the new score; keep the floor a few points below the achieved score, not at it. Reference: a score of 74.1 % against a floor of 75 % was one test away from failing every run.

## 6. What a plan must record (S-MUTATE)

The plan states, before implementation: the tool and exact version (and the config file as part of the pin); the files in scope; the score floor; the fast-test list and what is omitted; the measured baseline and estimated job time with the runtime budget; the three tier triggers; the aggregator job as the required check; the owner of scheduled failures; and where the report and per-class justifications live. The planning protocol's `S-MUTATE` step and its cold-read check refer to this list.

## 7. Tool notes observed with Mull (mull-runner-19, 2026-11; re-verify for another version)

- Include and exclude paths live in `mull.yml` (`includePaths`, `excludePaths`, regular expressions), not on the runner command line, which rejects `--include-path`.
- The compiler plugin and the runner major version must agree with the compiler major version; the linker named in `-fuse-ld=` must be installed.
- The SQLite report has a `mutant` table with `filename`, `line_number`, `mutator`, `execution_status`; status 2 meant survived (killed was 1 or 3).
- The text summary prints no percentage; compute it.
- A mutant that makes a loop bound one too large reads one element past the end: a plain test rarely notices; a sanitizer job does.
