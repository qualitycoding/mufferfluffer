#!/usr/bin/env python3
"""Planner tool: writes tests/fixtures/effects.json, the single frozen description of every effect's
parameters, ports and meters (decisions D-020..D-026). Run once at planning time; the JSON is frozen.
The implementer never edits the JSON; core tables are generated from it by tools/gen_tables.py."""
import json, pathlib

def P(id, name, unit, lo, hi, de, taper="linear", steps=None):
    d = {"id": id, "name": name, "unit": unit, "min": lo, "max": hi, "def": de, "taper": taper}
    if steps: d.update(taper="stepped", min=0, max=len(steps) - 1, steps=steps)
    return d
def sw(id, name, steps, de=0): return P(id, name, "", 0, 0, de, steps=steps)
def audio(id, name): return {"id": id, "name": name, "kind": "audio", "param": None, "depth": 0.0}
def cv(param, name, depth=1.0): return {"id": "cv_" + param, "name": name, "kind": "cv_normalised", "param": param, "depth": depth}
def voct(param, name): return {"id": "cv_" + param, "name": name, "kind": "cv_volt_per_oct", "param": param, "depth": 0.0}
def trig(id, name, param): return {"id": id, "name": name, "kind": "trigger", "param": param, "depth": 0.0}
def cvout(id, name): return {"id": id, "name": name, "kind": "cv_normalised", "param": None, "depth": 0.0}
def M(id, name): return {"id": id, "name": name}

ACTIVE = sw("active", "Active", ["Bypass", "On"], 1)
DRIVE = P("drive", "Drive", "dB", -7.0, 28.0, 0.0)
OUT = P("output", "Output", "dB", -24.0, 12.0, 0.0)
MIX = lambda de: P("mix", "Mix", "", 0.0, 1.0, de)
LFO6 = ["Sine", "Triangle", "Square", "Sawtooth", "Ramp", "Random step"]
LEVEL, ACT = M("level", "Level"), M("active", "Active")

effects = [
 {"id": 0, "enum": "Lowpass", "slug": "lowpass", "name": "Fluff Lowpass", "milestone": 1,
  "params": [DRIVE, P("cutoff", "Cutoff", "Hz", 15.0, 12000.0, 1000.0, "log"), P("resonance", "Resonance", "", 0.0, 10.0, 0.0),
             P("env_amount", "Env Amount", "oct", 0.0, 5.0, 0.0), sw("env_speed", "Env Speed", ["Smooth", "Fast"], 1),
             sw("poles", "Poles", ["2-pole", "4-pole"], 1), MIX(1.0), ACTIVE],
  "inputs": [audio("in", "Audio In"), cv("cutoff", "Cutoff CV"), cv("resonance", "Resonance CV"), cv("env_amount", "Amount CV"), cv("mix", "Mix CV")],
  "outputs": [audio("out", "Audio Out"), cvout("env_out", "Envelope Out")],
  "meters": [LEVEL, M("env", "Envelope"), ACT], "right_output": None},
 {"id": 1, "enum": "RingMod", "slug": "ringmod", "name": "Fluff Ring", "milestone": 1,
  "params": [DRIVE, P("frequency", "Frequency", "", 0.0, 1.0, 0.5), sw("range", "Range", ["Lo", "Hi"], 1),
             P("lfo_rate", "LFO Rate", "Hz", 0.1, 25.0, 1.0, "log"), P("lfo_amount", "LFO Amount", "oct", 0.0, 3.0, 0.0),
             sw("lfo_wave", "LFO Wave", ["Sine", "Square"], 0), MIX(1.0), ACTIVE],
  "inputs": [audio("in", "Audio In"), audio("carrier_in", "Carrier In"), cv("frequency", "Frequency CV"), cv("lfo_rate", "Rate CV"), cv("lfo_amount", "Amount CV"), cv("mix", "Mix CV")],
  "outputs": [audio("out", "Audio Out"), audio("carrier_out", "Carrier Out"), cvout("lfo_out", "LFO Out")],
  "meters": [LEVEL, M("lfo", "LFO"), ACT], "right_output": None},
 {"id": 2, "enum": "Phaser", "slug": "phaser", "name": "Fluff Phase", "milestone": 1,
  "params": [DRIVE, OUT, P("sweep", "Sweep", "", 0.0, 10.0, 5.0), P("resonance", "Resonance", "", 0.0, 10.0, 0.0),
             P("lfo_rate", "LFO Rate", "", 0.0, 1.0, 0.5), sw("lfo_range", "LFO Range", ["Lo", "Hi"], 0),
             P("lfo_amount", "LFO Amount", "", 0.0, 1.0, 0.5), sw("stages", "Stages", ["6-stage", "12-stage"], 0), ACTIVE],
  "inputs": [audio("in", "Audio In"), audio("sweep_in", "Sweep In"), cv("lfo_rate", "Rate CV"), cv("lfo_amount", "Amount CV"), cv("sweep", "Sweep CV"), cv("resonance", "Resonance CV")],
  "outputs": [audio("out", "Audio Out"), audio("aux_out", "Aux Out"), cvout("lfo_out", "LFO Out")],
  "meters": [LEVEL, M("lfo", "LFO"), ACT], "right_output": "aux_out"},
 {"id": 3, "enum": "Delay", "slug": "delay", "name": "Fluff Delay", "milestone": 2,
  "params": [DRIVE, OUT, P("time", "Time", "", 0.0, 1.0, 0.5), sw("range", "Range", ["Short", "Long"], 0),
             P("feedback", "Feedback", "", 0.0, 10.0, 3.0), P("lfo_rate", "LFO Rate", "Hz", 0.05, 50.0, 1.0, "log"),
             P("lfo_amount", "LFO Amount", "", 0.0, 10.0, 0.0), sw("lfo_wave", "LFO Wave", LFO6, 0), MIX(0.5),
             sw("spillover", "Spillover", ["Off", "On"], 0), sw("tap", "Tap", ["Up", "Down"], 0), ACTIVE],
  "inputs": [audio("in", "Audio In"), audio("fb_return", "Feedback Return"), cv("feedback", "Feedback CV"), cv("time", "Time CV"), cv("lfo_rate", "Rate CV"), cv("lfo_amount", "Amount CV"), cv("mix", "Mix CV"), trig("tap_in", "Tap", "tap")],
  "outputs": [audio("out", "Mix Out"), audio("delay_out", "Delay Out"), audio("fb_send", "Feedback Send")],
  "meters": [LEVEL, M("rate", "Rate"), ACT], "right_output": None},
 {"id": 4, "enum": "FilterBank", "slug": "filterbank", "name": "Fluff Bank", "milestone": 3,
  "params": [DRIVE, OUT] + [P(f"level{i}", f"Filter {i}", "", 0.0, 1.0, 1.0) for i in range(1, 9)] +
            [sw("voicing", "Voicing", ["Bass", "Mids"], 1), sw("pattern", "Pattern", [str(i) for i in range(1, 13)], 0),
             P("envelope", "Envelope", "", 0.0, 10.0, 2.0), P("rate", "Rate", "Hz", 0.5, 32.0, 4.0, "log"),
             sw("lfo", "LFO", ["Off", "On"], 0), P("sweep", "LFO/Sweep", "", 0.0, 1.0, 0.5), MIX(1.0),
             sw("tap", "Tap", ["Up", "Down"], 0), ACTIVE],
  "inputs": [audio("in", "Audio In"), cv("rate", "Rate CV"), cv("envelope", "Envelope CV"), cv("mix", "Mix CV"), cv("sweep", "LFO/Sweep CV"), trig("tap_in", "Tap", "tap"), trig("step_in", "Step", None)],
  "outputs": [audio("out_l", "Left/Mono Out"), audio("out_r", "Right Out")],
  "meters": [LEVEL, M("rate", "Rate"), ACT], "right_output": "out_r"},
 {"id": 5, "enum": "OscBox", "slug": "oscbox", "name": "Fluff Osc", "milestone": 3,
  "params": [DRIVE, OUT, P("frequency", "Frequency", "Hz", 25.0, 1600.0, 200.0, "log"), P("wave", "Wave", "", 0.0, 1.0, 0.0),
             sw("sync", "Sync", ["Off", "On"], 0), P("fm_amount", "FM Amount", "", 0.0, 1.0, 0.0),
             P("env_amount", "Env Amount", "oct", 0.0, 8.0, 0.0), MIX(0.5), ACTIVE],
  "inputs": [audio("in", "Audio In"), voct("frequency", "Frequency CV"), cv("wave", "Wave CV", 0.6), cv("env_amount", "Env Amount CV"), cv("fm_amount", "FM Amount CV"), cv("mix", "Mix CV")],
  "outputs": [audio("out", "Audio Out"), cvout("env_out", "Envelope Out"), audio("osc_out", "Oscillator Out")],
  "meters": [LEVEL, M("env", "Envelope"), ACT], "right_output": None},
 {"id": 6, "enum": "Chorus", "slug": "chorus", "name": "Fluff Swarm", "milestone": 2,
  "params": [DRIVE, OUT, P("time", "Time", "", 0.0, 1.0, 0.5), sw("range", "Range", ["Flange", "Chorus"], 1),
             P("feedback", "Feedback", "", -1.0, 1.0, 0.0), P("lfo_rate", "LFO Rate", "Hz", 0.05, 50.0, 0.5, "log"),
             P("lfo_amount", "LFO Amount", "", 0.0, 1.0, 0.3), sw("lfo_wave", "LFO Wave", LFO6, 0), MIX(0.5),
             sw("tap", "Tap", ["Up", "Down"], 0), ACTIVE],
  "inputs": [audio("in", "Audio In"), audio("fb_return", "Feedback Return"), cv("time", "Time CV"), cv("feedback", "Feedback CV"), cv("lfo_rate", "Rate CV"), cv("lfo_amount", "Amount CV"), cv("mix", "Mix CV"), trig("tap_in", "Tap", "tap")],
  "outputs": [audio("out_l", "Left/Mono Out"), audio("out_r", "Right Out"), audio("fb_send", "Feedback Send")],
  "meters": [LEVEL, M("rate", "Rate"), ACT], "right_output": "out_r"},
]
for e in effects: e["mono_sums_right"] = (e["slug"] == "filterbank")

# Pattern set of the filter bank (D-024): original patterns of this project.
# 12 base patterns; bank "Bass" uses them as written, bank "Mids" uses them with the channel order reversed.
# Each pattern is 8 channel strings (filter 1 first); 'x' = trigger the channel's envelope on that step, '.' = idle.
# Channels may differ in length (1..64). Pattern 1 of each bank is static: every channel is "-" (VCA held open).
def lfsr_bits(seed, n):
    out, state = [], seed
    for _ in range(n):
        bit = ((state >> 0) ^ (state >> 2) ^ (state >> 3) ^ (state >> 5)) & 1
        state = (state >> 1) | (bit << 15)
        out.append(state & 1)
    return out
def stair(order): return ["".join("x" if order[c] == s else "." for s in range(8)) for c in range(8)]
base = [
 ("Static", ["-"] * 8),
 ("Stairs up", stair(list(range(8)))),
 ("Stairs down", stair(list(range(7, -1, -1)))),
 ("Up and down", ["".join("x" if s in (c, 14 - c) else "." for s in range(14)) for c in range(8)]),
 ("Pairs", ["".join("x" if s == c // 2 else "." for s in range(4)) for c in range(8)]),
 ("Odd even", ["x." if c % 2 == 0 else ".x" for c in range(8)]),
 ("Outside in", ["".join("x" if s == min(c, 7 - c) else "." for s in range(4)) for c in range(8)]),
 ("Cascade", ["".join("x" if s in (c, (c + 1) % 8) else "." for s in range(8)) for c in range(8)]),
 ("Polymetre", ["x" + "." * c for c in range(8)]),
 ("Primes", ["x" + "." * (n - 1) for n in (2, 3, 5, 7, 11, 13, 17, 19)]),
 ("Scatter", ["".join("x" if b else "." for b in lfsr_bits(0xACE1 + 977 * c, 32)) for c in range(8)]),
 ("Pulse", ["x..." for c in range(8)]),
]
patterns = [{"bank": "Bass", "index": i, "name": n, "channels": ch} for i, (n, ch) in enumerate(base)] + \
           [{"bank": "Mids", "index": i, "name": n, "channels": list(reversed(ch))} for i, (n, ch) in enumerate(base)]
doc = {"api_version": 1, "patterns": patterns, "comment": "FROZEN - DO NOT MODIFY. Generated by tools/make_effects_fixture.py (decisions D-020..D-026).", "effects": effects}
pathlib.Path("tests/fixtures/effects.json").write_text(json.dumps(doc, indent=1) + "\n")
print(sum(len(e["params"]) for e in effects), "params", sum(len(e["inputs"]) for e in effects), "inputs")
