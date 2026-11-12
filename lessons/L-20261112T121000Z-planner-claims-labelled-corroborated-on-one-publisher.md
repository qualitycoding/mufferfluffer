---
id: L-20261112T121000Z-planner-claims-labelled-corroborated-on-one-publisher
title: Planner claims backed by a tag list plus a knowledge-store item were labelled corroborated although both trace to one publisher
status: active
supersedes: []
recurrence_of: L-20261009T123005Z-corroborated-label-without-independent-sources
occurrences: 2
severity: Low
tags: [phase:research, kind:process]
recorded_by: planner
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-12T12:10:00Z
---
## Trigger
A claim's second source is a knowledge-store item or a second page of the same organisation.
## What went wrong
Five planner claims (Rack tag, pluginval tag, action commits, runner labels, Windows image) carried the label corroborated; their sources were one upstream publisher each.
## Correction
Tag facts read by git ls-remote are now labelled verified (direct probe); the runner-label claims are single-source with a note that S-001 exercises them.
## Prevention rule
Count publishers, not URLs: a knowledge-store item inherits the publisher of the source it was derived from.
## Detection check
For every claim with status corroborated, publisher_count is at least 2 and the publishers are different organisations.
## Evidence
research/agents/planner-claims.json history.
