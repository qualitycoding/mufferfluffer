---
id: L-20261119T042700Z-changed-constant-not-searched-in-every-spelling
title: A test-count change was applied to the spellings I remembered, and two other copies in the workflow broke
status: active
supersedes: []
severity: Low
tags: [tech:github-actions, phase:implementation, kind:incomplete-change]
recurrence_of: null
occurrences: 1
recorded_by: implementer
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-19T04:27:00Z
---
## Trigger
Adding eleven frozen tests changed the core suite from 87 to 98 cases (86 to 97 in the sanitizer build). `ci.yml` checks the number in several forms: `out of 87`, `in 86 test cases` and `Total Tests: 87`.
## What went wrong
I replaced the two spellings I had in mind and searched for a third pattern that did not match the others. The `Total Tests: 87` checks in `core-oldest-toolchain` and `core-musl` were left, and both jobs failed on the REFREEZE commit (run 44457040776).
## Correction
Search the whole tree for the bare number (`grep -rn "\b87\b"`) and for the old total's neighbours, then change every hit that counts tests; commit the count change together with the tests.
## Prevention rule
When a constant changes, search for the bare value in the whole repository, not for the sentences in which you remember it, and read every hit.
## Detection check
After the edit, `grep -rnw "<old value>" .github tools tests/CMakeLists.txt README.md docs` must show no remaining count of tests; run it before committing.
## Evidence
Run 44457040776, jobs core-oldest-toolchain and core-musl; fix in the next commit.
