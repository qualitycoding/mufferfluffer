#!/usr/bin/env python3
"""Planner tool: writes one knowledge item per research claim (protocol Rule 12) into knowledge/, in the
knowledge store's item format. usage: make_knowledge.py <run id>. Idempotent: ids are derived from claim ids."""
import json, pathlib, re, sys
run = sys.argv[1]
SUBJ = {"C-PEDA": ("moogerfooger-pedals", ["domain:audio", "domain:dsp"]), "C-PEDB": ("moogerfooger-pedals", ["domain:audio", "domain:dsp"]),
        "C-PEDC": ("moogerfooger-pedals", ["domain:audio", "domain:dsp"]), "C-R2": ("moogerfooger-pedals", ["domain:audio", "domain:dsp"]),
        "C-R3": ("moogerfooger-pedals", ["domain:audio", "domain:dsp"]), "C-DSP": ("analogue-modelling-dsp", ["domain:audio", "domain:dsp", "tech:cpp"]),
        "C-LEGAL": ("plugin-licensing-and-marks", ["domain:audio", "tech:juce", "tech:vst3", "tech:vcv-rack"]),
        "C-REF": ("moogerfooger-reference-recordings", ["domain:audio"]),
        "C-PLANNER": ("mufferfluffer-build-and-ci", ["tech:juce", "tech:vcv-rack", "tech:cmake", "tech:github-actions", "tech:mull"])}
out = pathlib.Path("knowledge"); out.mkdir(exist_ok=True)
for old in out.glob("K-*.md"): old.unlink()
n = 0
for c in json.loads(pathlib.Path("research/claims.json").read_text()):
    fam = c["id"].rsplit("-", 1)[0]; subject, tags = SUBJ[fam]
    kid = f"K-20261112T120000Z-{c['id'].lower()}"
    conf = {"verified": "verified", "corroborated": "corroborated", "single-source": "single-source", "unverified": "inferred"}[c["status"]]
    src = c["sources"][0] if c["sources"] else {"url": "", "doi": "", "title": "no source: negative result or inference recorded by the research agent", "locator": "", "accessed": "2026-11-12", "tier": 4}
    src = {k: src.get(k, "") for k in ("url", "doi", "title", "locator", "accessed", "tier")}
    ver = (c["sources"][0].get("version", "") if c["sources"] else "") or "as of 2026-11"
    body = []
    if len(c["sources"]) > 1: body.append("Further sources: " + "; ".join(f"{s.get('title','')} ({s.get('publisher','')}, {s.get('locator','')}) {s.get('url','')}" for s in c["sources"][1:]))
    if c.get("contradicted_by"): body.append("Contradicting or qualifying evidence: " + " | ".join(c["contradicted_by"]))
    if c.get("verdict"): body.append(f"Re-verification verdict (round {c['round']}): {c['verdict']}.")
    if c.get("spike"): body.append(f"Checked by: {c['spike']}.")
    body.append("Reading method: unless the source is this run's own spike or a git tag list, the page was read through a summarising fetch tool, so locators are approximate.")
    fm = ["---", f"id: {kid}", "statement: " + json.dumps(c["claim"], ensure_ascii=False), "status: current", "supersedes: []",
          f"tags: [{', '.join(tags + ['subject:' + subject, 'phase:research'])}]", "applies_to_version: " + json.dumps(str(ver)[:60], ensure_ascii=False),
          "source: " + json.dumps(src, ensure_ascii=False), f"confidence: {conf}", f"derived_from: [{c['id']}]", "discovered_by: planner",
          f"run: {run}", "recorded_at: 2026-11-12T12:00:00Z", "---"]
    (out / f"{kid}.md").write_text("\n".join(fm) + "\n" + "\n\n".join(body) + "\n"); n += 1
(out / "TAXONOMY-ADDITIONS.md").write_text("# Tags this run adds to the knowledge store's TAXONOMY.md\n\n## subject:\n\n"
    "- `subject:moogerfooger-pedals`: documented behaviour of the Moog Moogerfooger analogue effect pedals (MF-101 to MF-108M)\n"
    "- `subject:analogue-modelling-dsp`: published digital models of analogue audio circuits (ladder filters, bucket-brigade delays, phasers, band-limited oscillators)\n"
    "- `subject:plugin-licensing-and-marks`: licences and trade-mark rules that bind an open-source audio plug-in (JUCE, VST3, VCV Rack, manufacturer marks)\n"
    "- `subject:moogerfooger-reference-recordings`: availability and licences of recordings of Moogerfooger hardware\n"
    "- `subject:mufferfluffer-build-and-ci`: pins, build measurements and CI facts established for the mufferfluffer project\n")
print(n, "knowledge items")
