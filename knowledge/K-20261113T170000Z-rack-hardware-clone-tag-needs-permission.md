---
id: K-20261113T170000Z-rack-hardware-clone-tag-needs-permission
statement: "VCV Rack's manifest guide reserves the module tag 'Hardware clone' for modules that copy a commercial hardware module's function, panel design and layout, asks that hardware be cloned only with the owner's permission, and ties the modularGridUrl property to that tag."
status: current
supersedes: []
tags: [tech:vcv-rack, subject:plugin-licensing-and-marks, phase:research]
applies_to_version: "Rack 2 manifest guide as published 2026-11-13"
source: {"url": "https://vcvrack.com/manual/Manifest", "doi": "", "title": "VCV Rack manual: Plugin Manifest", "locator": "section on module tags, entry Hardware clone; property modularGridUrl", "accessed": "2026-11-13", "tier": 1}
confidence: verified
derived_from: []
discovered_by: assumption audit (fresh-context reviewer), confirmed by planner
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-13T17:00:00Z
---
Checked by: page fetched twice on 2026-11-13 (auditor and planner). Consequence: a module that models documented behaviour under its own name and panel, without the hardware owner's permission, should not carry the tag. The tag exists in Rack's tag list (src/tag.cpp), which says only that it is valid, not when it applies.
