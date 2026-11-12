#!/usr/bin/env python3
"""Planner tool: merges research/agents/*-claims.json into research/claims.json (stable IDs by family) and
research/agents/*-sources.md into research/SOURCES.md. Run from the repository root."""
import json, pathlib, re
FAM = [("pedals-a", "C-PEDA", 1), ("pedals-b", "C-PEDB", 1), ("pedals-c", "C-PEDC", 1), ("dsp", "C-DSP", 1), ("legal", "C-LEGAL", 1),
       ("references", "C-REF", 1), ("planner", "C-PLANNER", 1), ("round2", "C-R2", 2), ("round3", "C-R3", 3)]
out = []
for name, prefix, rnd in FAM:
    p = pathlib.Path(f"research/agents/{name}-claims.json")
    if not p.exists(): continue
    for i, c in enumerate(json.loads(p.read_text()), 1):
        conf = c.get("confidence", "single-source")
        status = "verified" if conf == "verified" else ("unverified" if conf == "inferred" or c["claim"].startswith("UNKNOWN") else conf)
        out.append({"id": f"{prefix}-{i:03d}", "claim": c["claim"], "kind": c.get("kind", "software"), "load_bearing": bool(c.get("load_bearing")),
                    "sources": c.get("sources", []), "publisher_count": c.get("publisher_count", len({s.get("publisher") for s in c.get("sources", [])})),
                    "status": status, "round": rnd, "topic": c.get("topic", ""), "contradicted_by": c.get("contradicted_by", []),
                    "spike": c.get("spike", ""), "verdict": c.get("verdict", ""), "checks": c.get("checks", ""), "depends_on": []})
pathlib.Path("research/claims.json").write_text(json.dumps(out, indent=1, ensure_ascii=False) + "\n")
md = ["# Sources (every source opened or attempted)", "",
      "One table per research strand, as written by the agent that did the reading; rejected and unreachable sources are included. "
      "Reading method: unless a row says otherwise, pages were read through a summarising fetch tool (direct download was refused by the egress proxy), "
      "so page and equation locators are approximate (R-001).", ""]
for name, prefix, rnd in FAM:
    p = pathlib.Path(f"research/agents/{name}-sources.md")
    if p.exists(): md += [f"## Strand `{name}` (claims {prefix}-nnn, round {rnd})", "", p.read_text().strip(), ""]
pathlib.Path("research/SOURCES.md").write_text("\n".join(md) + "\n")
from collections import Counter
lb = [c for c in out if c["load_bearing"]]
print(len(out), "claims;", len(lb), "load-bearing:", dict(Counter(c["status"] for c in lb)))
