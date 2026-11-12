#!/usr/bin/env python3
# Wraps plan/SOURCE_REGISTER.md (the master copy) as the knowledge item tagged kind:source-register (protocol Appendix B).
# Run from the repository root after any change to the register:  python3 research/tools/make_source_register_item.py
import json, pathlib, re
src = pathlib.Path("plan/SOURCE_REGISTER.md").read_text()
rows = [l for l in src.split("\n") if re.match(r"^\| \d+ \|", l)]
def count(word): return sum(1 for r in rows if r.split("|")[7].strip().lower().startswith(word))
n = len(rows)
nolic = int(re.search(r"Rows whose licence is `unknown` or `none stated`: (\d+)", src).group(1))
assert n == int(re.search(r"Sources listed: (\d+)", src).group(1))
claims = sorted({c["id"] for c in json.loads(pathlib.Path("research/claims.json").read_text()) if "id" in c})
stmt = (f"Source register for the mufferfluffer planning run (gen-20261111T151412Z-mufferfluffer-vst-vcv): {n} external sources checked, "
        f"{count('used')} used, {count('corroborating')} corroborating, {count('rejected')} rejected, {count('unreachable')} unreachable, {count('not')} not relevant; "
        f"{nolic} have an unknown or unstated licence.")
head = f"""---
id: K-20261112T130000Z-source-register-mufferfluffer
statement: "{stmt}"
status: current
supersedes: []
tags: [subject:moogerfooger-pedals, subject:analogue-modelling-dsp, subject:plugin-licensing-and-marks, domain:audio, phase:research, kind:source-register]
applies_to_version: version-independent
source: {{"url": "", "doi": "", "title": "Source register, run gen-20261111T151412Z-mufferfluffer-vst-vcv", "locator": "table below", "accessed": "2026-11-12", "tier": "n/a"}}
confidence: verified
derived_from: [{", ".join(claims)}]
discovered_by: planner
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-12T13:00:00Z
---
"""
body = src.split("\n", 1)[1].lstrip("\n")
pathlib.Path("knowledge/K-20261112T130000Z-source-register-mufferfluffer.md").write_text(head + body)
print(stmt)
