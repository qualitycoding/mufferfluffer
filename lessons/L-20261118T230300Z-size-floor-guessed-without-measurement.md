---
id: L-20261118T230300Z-size-floor-guessed-without-measurement
title: Planner size floors for the Rack SDK archive and the package were guesses and rejected correct downloads
status: active
supersedes: []
severity: Low
tags: [tech:vcv-rack, tech:github-actions, phase:implementation, kind:verification-gap]
recurrence_of: null
occurrences: 1
recorded_by: implementer
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-18T23:03:00Z
---
## Trigger
A check fails a download or a build product for being smaller than a number the plan gave, and the first real artefact is below it: the win-x64 Rack SDK archive was 3.15 MB against a floor of 5 MB, and the Rack package was 97,548 bytes against 100 kB.
## What went wrong
A floor chosen without measuring any real artefact turned a correct result into a failure. The check's purpose is to catch an empty or truncated file, which a much lower floor does equally well.
## Correction
The floors were lowered to 1 MB and 50 kB and the package checker was extended to test its contents (library, manifest, licence, seven panels), which is the stronger check. Both are DEVIATIONS.md rows.
## Prevention rule
Set a size floor only after one real artefact has been measured, and also check the artefact's contents; use the size only to catch an empty file.
## Detection check
Compare each floor in a workflow with the size printed by the first green run.
## Evidence
DEVIATIONS.md rows for S-030 and S-032; CI run 44409074123.
