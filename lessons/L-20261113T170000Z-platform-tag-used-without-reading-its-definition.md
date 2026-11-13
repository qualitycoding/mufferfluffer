---
id: L-20261113T170000Z-platform-tag-used-without-reading-its-definition
title: A platform tag was required by a frozen test without reading the platform's definition of it
status: active
supersedes: []
severity: High
tags: [domain:audio, tech:vcv-rack, phase:research, kind:wrong-assumption]
recurrence_of: null
occurrences: 1
recorded_by: planner
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-13T17:00:00Z
---
## Trigger
Choosing a category, tag or label that a platform defines (a plug-in library tag, a store category, a licence identifier) because its name sounds right.
## What went wrong
The plan required the Rack tag "Hardware clone" on all seven modules and froze a test for it. The evidence was that the tag exists in Rack's tag list. The platform's guide defines the tag for modules that copy a hardware panel and layout with the owner's permission, which is the opposite of this project's position. Research, an adversarial pass, four cold reads and four pre-mortem rounds did not question it; the assumption audit did, by reading the definition.
## Correction
Tag removed; the test now forbids it (D-072).
## Prevention rule
For every platform-defined label a plan uses, record a claim that quotes where the platform defines its meaning and conditions, not merely that the label is valid.
## Detection check
For each tag or category in a manifest test: is there a claim whose source is the platform's definition page?
## Evidence
`premortem/ASSUMPTION_AUDIT.md` item F-2; decision D-072 item 1; `tests/py/test_repo.py` class `T902RackManifest`.
