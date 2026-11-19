---
id: L-20261118T230100Z-unlimited-parallel-build-exhausted-runner-memory
title: A bare cmake --parallel under the Makefile generator ran out of memory on a 7 GiB runner
status: active
supersedes: []
severity: Medium
tags: [tech:cmake, tech:juce, tech:github-actions, phase:implementation, kind:environment]
recurrence_of: null
occurrences: 1
recorded_by: implementer
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-18T23:01:00Z
---
## Trigger
Building seven JUCE plug-in targets in CI on a 2 vCPU, 7 GiB Linux runner with the default Makefile generator and `cmake --build ... --parallel` with no number. The log ends with `c++: fatal error: Killed signal terminated program cc1plus` and `The runner has received a shutdown signal`.
## What went wrong
The build step was written as `--parallel` without a job count. With Makefiles that starts unlimited jobs, and several JUCE translation units compiled at once exhausted memory; two runs failed and others ran for more than 30 minutes while thrashing.
## Correction
Every VST3 build step passes an explicit count: `--parallel "$(nproc)"` on Linux and `--parallel "$(sysctl -n hw.ncpu)"` on macOS (the Visual Studio generator on Windows limits itself). Recorded in DEVIATIONS.md.
## Prevention rule
When a CI build step uses a Makefile generator, pass an explicit job count to `--parallel`; never a bare `--parallel`.
## Detection check
Search workflows for `--parallel` not followed by a number or a command substitution, and for any job log containing `Killed signal terminated program`.
## Evidence
CI runs 44409598565 and 44410358339 (vst3-linux, step Configure and build); run 44416330150 passed with the fix.
