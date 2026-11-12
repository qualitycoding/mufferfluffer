---
id: L-20261112T120500Z-dispatch-and-schedule-triggers-planned-for-a-non-default-branch
title: Three plan steps relied on workflow_dispatch on the implementation branch, which GitHub only offers for workflows on the default branch
status: active
supersedes: []
recurrence_of: L-20261104T011000Z-the-plan-s-mitigation-for-workflow-dispatch-ne
occurrences: 2
severity: High
tags: [tech:github-actions, phase:planning, kind:wrong-assumption]
recorded_by: planner
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-12T12:05:00Z
---
## Trigger
A plan has the implementer run a manually dispatched or scheduled workflow from a feature branch.
## What went wrong
Steps S-030, S-MUTATE and S-040 were first written with workflow_dispatch runs on the implementation branch. A stored lesson on exactly this was loaded in R0 but its titles were re-read only after the steps were drafted.
## Correction
The three steps now use push triggers selected by a marker in the head commit message; dispatch and schedule are documented as live only after the owner merges the final pull request; a plan-wide rule states this.
## Prevention rule
Build the lesson-application table (R0) before drafting plan steps, and for every workflow a step runs, write its trigger and the branch it must exist on next to the step.
## Detection check
grep the plan for workflow_dispatch and schedule: each occurrence is accompanied by the default-branch condition.
## Evidence
plan/PLAN.md rules list; commit 0720f62 of the generation branch.
