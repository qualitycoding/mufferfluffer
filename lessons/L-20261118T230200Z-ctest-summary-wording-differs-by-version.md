---
id: L-20261118T230200Z-ctest-summary-wording-differs-by-version
title: A grep for the ctest summary line was written from one ctest version and failed on another
status: active
supersedes: []
severity: Low
tags: [tech:cmake, tech:github-actions, phase:implementation, kind:wrong-assumption]
recurrence_of: null
occurrences: 1
recorded_by: implementer
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-18T23:02:00Z
---
## Trigger
A CI step greps the last line of `ctest` output (`100% tests passed, 0 tests failed out of 87`) on several platforms, and one platform's ctest prints `100% tests passed out of 87`.
## What went wrong
The grep took the Linux wording as universal. The Intel macOS job failed although all 87 tests passed.
## Correction
Grep for the fixed part that every version prints (`out of 87`) together with the failure count line, and record the change in DEVIATIONS.md.
## Prevention rule
Match tool summary lines on the part that carries the fact (a count), not on the whole sentence, unless the sentence has been seen on every platform of the matrix.
## Detection check
Before committing a grep on tool output, run the tool on each platform of the matrix once, or use the fixed-string fragment with the number only.
## Evidence
CI runs for S-018 on core-macos-intel; DEVIATIONS.md row for S-018.
