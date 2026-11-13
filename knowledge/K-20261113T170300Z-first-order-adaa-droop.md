---
id: K-20261113T170300Z-first-order-adaa-droop
statement: "First-order antiderivative anti-aliasing of a memoryless non-linearity acts on small signals as a two-sample mean, with gain |cos(pi f / fs)|: at 48 kHz that is -2.0 dB at 10 kHz and -11.7 dB at 20 kHz, plus half a sample of delay."
status: current
supersedes: []
tags: [domain:dsp, subject:analogue-modelling-dsp, phase:research]
applies_to_version: "version-independent"
source: {"url": "", "doi": "", "title": "Derivation and simulation by the assumption auditor", "locator": "two-tap mean of the linearised non-linearity; simulation with 2 tanh(x/2) at amplitude 0.05", "accessed": "2026-11-13", "tier": 1}
confidence: verified
derived_from: []
discovered_by: assumption audit (fresh-context reviewer), confirmed by planner
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-13T17:00:00Z
---
Checked by: closed form and simulation, 2026-11-13. An earlier figure in this run (0.6 dB at 10 kHz, 2.6 dB at 20 kHz) was wrong.
