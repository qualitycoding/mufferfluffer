---
id: L-20261111T200000Z-eight-draft-test-defects-found-by-oracle
title: Eight of 77 draft tests were wrong and were found only by running them against an oracle implementation
status: active
supersedes: []
recurrence_of: L-20261009T234507Z-draft-test-unsatisfiable-found-by-oracle-t-021
occurrences: 2
severity: Medium
tags: [domain:audio, domain:dsp, tech:cpp, phase:planning, kind:verification-gap]
recorded_by: planner
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-11T20:00:00Z
---
## Trigger
Behavioural DSP tests are drafted from decision formulas before any implementation exists.
## What went wrong
The first oracle run failed 10 of 77 tests; 8 were test defects: measuring while a smoothed control was still moving, giving clock-relative sources absolute indices, exact float equality after smoothing, a tap sequence ignoring the five-second idle rule, an analysis window shorter than one cycle of the tone, and a threshold above the analytic value.
## Correction
Each test was corrected and the suite re-run green against the oracle and red against the stubs; two failures were real gaps in decisions and were fixed there.
## Prevention rule
Write the oracle before the freeze and budget for a second pass: for every test that drives a smoothed parameter, settle before measuring; for every source function, state whether its index is absolute or relative to the rig clock.
## Detection check
The red-verification record shows a green oracle run and names the defects found.
## Evidence
research/spikes/red-verification/RESULT.md.
