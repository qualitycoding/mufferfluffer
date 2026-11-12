---
name: mutation-sharding
description: Use when a mutation-testing job exceeds its CI runtime or memory budget and must be split into deterministic shards - especially a per-file or per-module matrix with CPU and memory caps, bounded tool-install retries, per-shard report gates and one aggregate required check. Also use when a mutation workflow needs a step limit, a shard timeout or protection against flaky package installs.
---

# Mutation sharding

Use this skill with `mutation-testing-in-ci`. That skill decides **when** mutation testing runs and how the score gate behaves; this skill decides how to **divide one expensive run** so the whole suite stays inside a stated CPU, memory and wall-clock budget.

The default shape is a generated **per-file matrix**: one CI job per mutable source file, with small files packed into deterministic groups only when measurements say that is safe. Do not hand-write the matrix in the workflow.

## 1. Decide whether sharding is needed

Record these values in the plan before writing the workflow:

1. `T`: instrumented baseline test time for the exact test executable and fast-test list the mutation tool will run.
2. `M`: mutant count for the in-scope source files.
3. `W`: measured worker count used by the tool.
4. `B`: job runtime budget in minutes, including build, install and report upload.
5. `C`: CPU cores available to one shard job.
6. `R`: RAM available to one shard job, in MiB.
7. `I`: measured install time on the pinned runner image.

Estimate the unsharded runtime as `ceil(M x T / W)` minutes plus build and install time. Shard when either:

- the estimate exceeds `B`; or
- the observed job exceeds `B` on a reference run; or
- the job is killed by the runner for memory, leaves no report, or needs an unbounded retry to get through its setup.

Also shard an incremental run when the changed-file scope can contain several files: a pull request touching five independently slow files must not silently become five times the single-file budget.

## 2. Build a deterministic shard plan

Generate the shard plan in a setup job and pass it to the matrix as JSON.

1. Enumerate every mutable source file from the tool's include/exclude configuration, not from the pull-request diff alone.
2. Attach each file's expected mutant count and measured or estimated runtime. Use the SQLite report, the tool's list command, or a measured mapping committed at the last full run.
3. Sort paths lexicographically so the same input always produces the same matrix.
4. Make every file with a runtime above `B / 2` its own shard.
5. Pack the remaining files into groups whose summed expected runtime is at most `B / 2` and whose summed expected mutants is at most the planned per-shard maximum. Never split one source file across shards unless the tool supports mutant-level sharding natively.
6. Give every shard a stable ID, for example `001`, `002`, and the exact include-path list it must run.
7. Emit an empty matrix with mode `not-applicable` when no in-scope file changed. Emit the full matrix when a change can alter every score: tests that kill mutants, mutation configuration, shard-plan generator, install/build scripts, or the report gate.

The workflow must fail closed if matrix generation fails, emits an empty object unexpectedly, or drops a file between enumeration and matrix output. A partial matrix is not a success.

## 3. Cap CPU and memory per shard

Set caps explicitly; do not let the tool discover the whole runner.

- **CPU cap.** Set the shard job's worker count to `min(configured cores, C, floor(R / per-worker memory estimate))`. For a tool without a memory estimate, start with 1 worker per 2 GiB of `R`, capped by `C`, then adjust from a measured run. On a GitHub-hosted Linux runner without a measured baseline, treat 4 vCPU and 16 GiB as the conservative planning assumption, not as a guarantee.
- **Memory cap.** State `R` in the workflow and enforce what the platform permits. For self-hosted runners, use a labelled runner whose inventory records cores and RAM; use `ulimit -v` or cgroup limits where available. For containerised shards, set container memory and CPU limits rather than relying only on tool flags.
- **Parallelism cap.** Set `strategy.max-parallel` from the budget, not from the number of shards. For example, allow at most 4 shard jobs to run at once on a 16-core self-hosted pool even if the matrix has 40 entries.
- **Step limit.** Give the install step a hard command timeout (for example 10 minutes per attempt), give the build/run step a hard command timeout below the job timeout, and set the job `timeout-minutes` below the CI budget. On GitHub Actions, `timeout-minutes` is a job limit, so long steps also need `timeout` in the shell.
- **Leave headroom.** The runtime budget must include checkout, install, build, report gate, artifact upload and the aggregate job. A shard whose measured runtime is 90% of `B` is too large.

## 4. Treat slow tool installation as flaky network

A mutation-tool install that normally takes under one minute but takes tens of minutes is an install-path problem, not a queueing success. Retry it only inside a bounded step:

1. Allow exactly three install attempts.
2. Bound each attempt with `timeout 10m` or a smaller measured command-specific bound.
3. Sleep between attempts: 2 seconds after attempt 1 and 4 seconds after attempt 2.
4. Print the attempt number, start time, end time and exit status.
5. After attempt 3 fails, stop the whole shard. Do not continue to the build with a partially installed tool.
6. Prefer a cache only when the cache key includes the tool version, OS image family, compiler major version and plugin major version. Validate the installed version after restore; a cache hit with the wrong plugin is a failed install.

Example install step:

```yaml
- name: Install mutation tool with bounded retries
  shell: bash
  run: |
    set -o pipefail
    for attempt in 1 2 3; do
      echo "::group::Install attempt ${attempt}"
      start=$(date -u +%s)
      if timeout 10m bash tools/ci/install_mutation_tool.sh; then
        end=$(date -u +%s)
        echo "install-attempt=${attempt} status=success seconds=$((end-start))"
        echo "install-attempt=${attempt} status=success seconds=$((end-start))" >> "$GITHUB_OUTPUT"
        exit 0
      fi
      status=$?
      end=$(date -u +%s)
      echo "install-attempt=${attempt} status=${status} seconds=$((end-start))"
      echo "::endgroup::"
      if [ "$attempt" -lt 3 ]; then sleep $((attempt * 2)); fi
    done
    exit 1
```

Record the install duration in the run log. If any successful attempt exceeds 10 minutes, mark the runner image or package mirror for investigation; do not raise the bound merely to make a slow mirror pass.

## 5. Per-file matrix workflow

Use `references/per-file-matrix.yml` as the structural template. The required jobs are:

1. **`shard-plan`**: checks out the repository, decides `full`, `incremental` or `not-applicable`, generates the matrix, and prints every shard with its include paths, expected mutants and caps. It runs on every trigger; there is no workflow-level `paths:` filter.
2. **`mutation-shard`**: runs once per matrix entry with `fail-fast: false`. Each shard installs with the bounded retry, builds with the same pinned compiler and tool versions, runs only its include paths, gates its own report, and uploads a uniquely named report.
3. **`mutation-aggregate`**: is the only required status check. It downloads every shard artifact, verifies that the number of reports equals the number of matrix entries, computes the full or incremental score from the reports, and fails when any shard failed, any report is missing, or the score floor is missed.

Per-shard rules:

- Give every report and log a shard-specific name, for example `mull-017.sqlite` and `mull-017.log`.
- Gate each shard from its report file, never from the mutation tool's exit status alone.
- Enforce a minimum mutant count per shard when the shard is expected to contain mutants. A shard that ran no mutant is a defect in scope generation, not a pass.
- Upload reports and logs with `if: always()` and `if-no-files-found: error`.
- Do not use `continue-on-error`, `|| true` or an unfiltered `always()` success path. `not-applicable` is emitted by the scope step and printed by the aggregate job; it is not inferred from a missing matrix.
- A change to tests, mutation config, shard-plan code, install/build scripts or report tooling widens the run to the full matrix. A change to documentation outside those paths does not.

## 6. Aggregate gate

The aggregate job must prove completeness before it computes a score:

1. Read the matrix entry count from `shard-plan`.
2. Download all shard artifacts.
3. Fail when any expected artifact is missing, empty, unreadable or has the wrong tool schema version.
4. Merge the per-shard reports with a checked-in merger appropriate to the tool; do not concatenate logs and treat the result as a database.
5. Compute the project score for a full run, or compare each changed file with its recorded baseline for an incremental run, using the same rules as `mutation-testing-in-ci`.
6. Print one table with shard ID, file count, mutant count, killed count, survived count and runtime.
7. Fail the build on any missing shard even if the merged score is above the floor. An incomplete numerator is not evidence of quality.

Keep the floor and tolerances in workflow inputs or constants near the gate, with the plan recording why those values were chosen.

## 7. Budget and review record

The plan or run record states:

- runner label, OS image family, CPU cores and RAM per shard;
- shard count, grouping rule and `max-parallel`;
- per-shard and total runtime budgets;
- install attempt bound, per-attempt bound and job timeout;
- tool, compiler, plugin and report-schema versions;
- fast-test omissions as deviations;
- the report merger and score-gate scripts with their pins;
- the measured reference timings used to choose the caps.

After the first sharded run, append the measured per-shard runtime, peak memory, install duration and score to `reviews/MUTATION.md` or the project's mutation review file. Resize or repack shards from those measurements; do not tune the matrix from a single outlier without marking it as an outlier.

## 8. Validation before relying on the workflow

Before making the aggregate check required:

1. Run a no-op documentation change and verify the aggregate prints `not-applicable` and passes.
2. Run a one-file change and verify that exactly one matrix entry runs and the run finishes inside the per-file budget.
3. Run the full matrix on a throwaway branch and verify that every file in scope appears in exactly one shard and every expected report exists.
4. Kill one shard deliberately and verify that the aggregate fails even when all other shard scores pass.
5. Run the score gate once against a known-below-floor report and verify a red result.
6. Simulate a failed third install attempt and verify that the shard stops before build or run.
