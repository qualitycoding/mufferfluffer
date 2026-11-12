---
id: L-20261112T121500Z-sub-agent-research-lost-to-rate-limits
title: Two research sub-agents were cut off by a rate limit before writing their output files
status: active
supersedes: []
recurrence_of: null
occurrences: 1
severity: Low
tags: [phase:research, kind:process]
recorded_by: planner
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-12T12:15:00Z
---
## Trigger
A long research or review task is delegated to a sub-agent that writes its files only at the end.
## What went wrong
The reference-recording strand and the adversarial pass were each interrupted by a rate limit with nothing on disk; both had to be resumed, and round 2 lost six figures to a fetch quota.
## Correction
Both agents were resumed by message and completed; the six figures were re-read in round 3.
## Prevention rule
Tell each sub-agent to write its output file after every few findings (append mode) and to keep a list of what remains, so an interruption loses minutes, not the task.
## Detection check
Agent briefs contain the sentence requiring incremental writes.
## Evidence
research/rounds/round-1.md and round-2.md.
