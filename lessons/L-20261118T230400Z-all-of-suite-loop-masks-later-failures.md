---
id: L-20261118T230400Z-all-of-suite-loop-masks-later-failures
title: A Catch2 test that loops over every effect stops at its first failure and hides failures in the others
status: active
supersedes: []
severity: Medium
tags: [tech:catch2, domain:audio, phase:implementation, kind:verification-gap]
recurrence_of: null
occurrences: 1
recorded_by: implementer
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-18T23:04:00Z
---
## Trigger
Integrating one effect at a time while the other effects are stand-ins: a frozen `[common]` test iterates over all seven effects and a failure in a stand-in stops the loop before it reaches the effect under work.
## What went wrong
Sub-agents reported that the shared tests were red because of the stand-ins and that their own effect's result could not be seen in the full run. A real defect in the effect under work could have been hidden behind a stand-in's failure.
## Correction
Each agent was told to check its own effect alone (a filter on the effect's tag or a scratch test that creates only that effect) before declaring it done, and the integration step ran the whole suite after each merge.
## Prevention rule
When a test loops over a list in which some members are stand-ins, test the member under work on its own before reading the loop's result.
## Detection check
Check that the run's failure output names the effect under work, or that a single-effect run passes.
## Evidence
Agent reports for S-011 to S-017; briefs in /home/claude/briefs.
