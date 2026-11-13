---
id: L-20261112T205000Z-test-estimator-tighter-than-its-stated-tolerance
title: A frozen test's estimator was far tighter than its stated tolerance, and loop tests measured only where the loop filter is flat
status: active
supersedes: []
severity: High
tags: [domain:audio, phase:specification, kind:verification-gap, tech:cpp]
recurrence_of: null
occurrences: 1
recorded_by: planner
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-12T20:50:00Z
---
## Trigger
Writing a numeric test for a periodic signal with few samples per cycle (here a 250 Hz triangle at 8 kHz, 32 samples per cycle, measured by mean absolute slope), or a test of a feedback loop that contains a filter.
## What went wrong
The slope estimator passed only when the samples fell exactly on the triangle's corners, so a rate 0.02 % off failed a test whose stated tolerance was 2 %. Separately, every feedback-loop test used frequencies in the filter's pass-band, so a loop that skipped the filter passed all 80 tests. The oracle passed both, so the oracle run did not reveal either fault; a reviewer found them by building deliberately wrong variants.
## Correction
T-304 counts cycles for the fast case; T-410 and T-708 compare the per-repeat loss at a pass-band and a corner frequency; T-110 covers the 2-pole mode with resonance.
## Prevention rule
Before freezing a numeric test, run it against at least one variant of the reference implementation that is wrong by a small amount inside the stated tolerance (it must pass) and one that is wrong in the way the test is meant to catch (it must fail); record both results beside the red and green runs.
## Detection check
For each behavioural decision, name the test that fails if that behaviour is removed; a decision with no such test is a gap.
## Evidence
`premortem/round-1.md` items PM1-03, PM1-04, PM1-05; `research/spikes/red-verification/RESULT.md`, section "Re-freeze after pre-mortem round 1"; decision D-063.
