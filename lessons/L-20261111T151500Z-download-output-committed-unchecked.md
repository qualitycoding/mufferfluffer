---
id: L-20261111T151500Z-download-output-committed-unchecked
title: A failed download was committed and pushed as the LICENSE file because its output was never inspected
status: active
supersedes: []
recurrence_of: null
occurrences: 1
severity: Medium
tags: [tech:github, tech:bash, phase:planning, kind:verification-gap]
recorded_by: planner
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-11T15:15:00Z
---
## Trigger
A file is created by redirecting a network command (`gh api ... > FILE`, `curl -o`) and committed in the same shell command, in a session whose GitHub token is bound to the attached repositories.
## What went wrong
`gh api licenses/agpl-3.0 --jq .body > LICENSE` returned HTTP 403 (non-repository REST paths are not available to a repository-bound session). The redirect still created a 249-byte file holding the error JSON, the chained commands carried on, and the commit was pushed to `main`.
## Correction
The AGPL-3.0 text was fetched from `raw.githubusercontent.com/spdx/license-list-data` (34 020 bytes, sha256 d8a6cc31...), checked for the licence title, committed and pushed as a second commit on `main`.
## Prevention rule
Never commit a downloaded file in the command that downloads it: first assert the exit status, a minimum size and an expected marker string (for a licence, its title line), and only then `git add` it.
## Detection check
`test $(wc -c < LICENSE) -gt 30000 && grep -q "GNU AFFERO GENERAL PUBLIC LICENSE" LICENSE` exits 0.
## Evidence
`main` commits 2c4f46f (bad) and 570c132 (fix) of qualitycoding/mufferfluffer.
