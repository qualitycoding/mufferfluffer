---
id: L-20261111T152000Z-licence-constraint-stated-at-intake-before-r0
title: The intake batch said the stack forces a GPL-family repository licence before the knowledge store had been read
status: active
supersedes: []
recurrence_of: null
occurrences: 1
severity: Low
tags: [tech:juce, tech:vcv-rack, phase:intake, kind:wrong-assumption]
recorded_by: planner
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-11T15:20:00Z
---
## Trigger
An intake batch proposes a licence default for a project that links a copyleft framework (JUCE under its open-source option) while the knowledge store holds items on that framework's licensing.
## What went wrong
Intake item 5 told the owner that the stack "forces a GPL-family repo licence ... not your usual Apache-2.0". The knowledge store (read only afterwards, in R0) records that binaries linking JUCE 9 under the open option are AGPLv3 works as a whole while the project's own sources may stay Apache-2.0, and that a free Rack plug-in may use any licence. The stated constraint was stronger than the facts.
## Correction
D-001 keeps AGPL-3.0-only (the adopted default, and the simplest single-licence position) but records the Apache-2.0-sources alternative and reports it to the owner in the Phase 5 report so the choice is informed.
## Prevention rule
Read the store items whose subject is a licence (`grep -ril licen knowledge/`) before the intake batch states any licence constraint, and word a default as a proposal with its alternative, not as a necessity.
## Detection check
The intake licence item cites a K- item or a source opened in the same turn.
## Evidence
Intake message of 2026-11-11; knowledge items K-20261007T160137Z (vst3-sdk) and K-20261009T234234Z (vcv-plugin-licensing).
