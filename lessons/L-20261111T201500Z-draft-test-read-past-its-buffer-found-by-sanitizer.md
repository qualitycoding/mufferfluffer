---
id: L-20261111T201500Z-draft-test-read-past-its-buffer-found-by-sanitizer
title: A draft frozen test read past the end of its rendered buffer; only the sanitizer run showed it
status: active
supersedes: []
recurrence_of: null
occurrences: 1
severity: Medium
tags: [domain:audio, tech:cpp, phase:planning, kind:verification-gap]
recorded_by: planner
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-11T20:15:00Z
---
## Trigger
A C++ test renders N samples into a vector and then measures windows whose positions are computed from loop variables.
## What went wrong
T-503 rendered 17 sequencer steps and measured a window ending at step 22.7. Without a sanitizer the read returned neighbouring heap memory and the test passed against the oracle.
## Correction
The test now renders 24 steps; the whole suite was re-run under AddressSanitizer and UndefinedBehaviourSanitizer and is clean.
## Prevention rule
Before freezing C++ tests, run them once under AddressSanitizer and UndefinedBehaviourSanitizer against the oracle, and treat a sanitizer report in test code as a test defect.
## Detection check
The spike record lists a sanitizer run of the frozen tests with zero reports.
## Evidence
research/spikes/red-verification/RESULT.md, section 'Defects this spike found before the freeze'.
