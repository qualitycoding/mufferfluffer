---
id: L-20261113T093000Z-action-pin-recorded-as-verified-was-not-the-tag-commit
title: An action pin recorded as "verified" was not the commit of its tag
status: active
supersedes: []
severity: High
tags: [tech:github-actions, tech:mull, phase:research, kind:integrity]
recurrence_of: L-20261028T200000Z-pinned-tool-version-does-not-exist
occurrences: 2
recorded_by: planner
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-13T09:30:00Z
---
## Trigger
Writing a commit hash for a pinned action or dependency into a plan, claim or lock file.
## What went wrong
Five action pins were recorded in one claim with the status "verified" and the note that each tag had been read with `git ls-remote`. Four were right. The fifth (`msys2/setup-msys2` v2.33.0) was a hash that no ref of the repository reaches; it had come from the planner's own working notes, not from the command (neither the lesson store nor the knowledge store contains that hash, so no stored item needs superseding). A frozen test checked only that pins have the form of a full hash. Two review passes and four cold reads did not notice; a pre-mortem reviewer who re-ran the command did.
## Correction
The pin was corrected, every pin was re-read, and the plan now has a tool, run in CI, that compares each pinned commit with the commit of the tag named beside it.
## Prevention rule
A pin is "verified" only if the command that read it and its output are stored beside the claim; a claim covering several pins needs one stored output per pin. Give the implementer a check that compares pins with tags, so that a wrong pin fails in CI instead of blocking a later step.
## Detection check
`git ls-remote <repository> refs/tags/<tag> 'refs/tags/<tag>^{}'` for every pin before freezing.
## Evidence
`premortem/round-2.md` item PM2-01; claim C-PLANNER-009 in `research/claims.json`; `plan/PLAN.md` step S-001 action 3a. The same class of error recurred in this run for the Mull install recipe, written from an older release's layout without installing it (`premortem/ASSUMPTION_AUDIT.md` item F-1; knowledge item `K-20261113T170100Z-mull-two-packages-since-0-36`).
