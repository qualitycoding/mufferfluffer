# Round 2 (depth: independent re-reading of load-bearing figures; tooling)

## Re-verification of 40 pedal figures (`round2` strand, claims C-R2-nnn)
A fresh agent re-opened the manuals on mirrors the first pass had not used and looked for a non-manufacturer source for each figure.
- Confirmed against the manual: 33. Contradicted: 0. Not re-read: 7 (six filter-array figures lost to a fetch quota, sent to round 3; the delay's Drive end points are not in its manual: only a 35 dB span is, so D-027 item 1 now says so).
- Independent second publisher found for one figure only (2-pole/4-pole slopes). The manuals are the manufacturer's specification, so "single-source, Tier 1" is the realistic ceiling for these figures; residual risk R-001.
- Notes carried into decisions: cutoff range starts at 15 Hz, not 20 Hz (D-020); the phaser has no Mix control (D-022); the chorus manual lists its six LFO shapes in two different orders (the panel order is used, D-015).

## Tooling (planner)
- Runner labels confirmed from GitHub's documentation and the runner-images README (C-PLANNER-010, -011).
- OSV query-by-commit confirmed from the API reference (C-PLANNER-019); the API is not reachable from the sandbox, so D-061's script is first exercised in CI (step S-005).
- Build and satisfiability spikes: `research/spikes/juce-vst3-build`, `research/spikes/red-verification`.

## Status of the question tree after round 2
All leaves answered or closed by a decision. Remaining weak points go to the adversarial pass (R5) and to the risk register.
