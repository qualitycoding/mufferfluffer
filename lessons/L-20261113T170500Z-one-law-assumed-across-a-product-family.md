---
id: L-20261113T170500Z-one-law-assumed-across-a-product-family
title: A behaviour documented for some models of a product family was assumed for all of them
status: active
severity: Medium
tags: [domain:audio, phase:research, kind:wrong-assumption]
recurrence_of: null
occurrences: 1
recorded_by: planner
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-13T17:00:00Z
---
**Trigger.** Modelling several related products and writing one shared rule (a control law, a range, a level) from the manuals of some of them.

**Mistake.** The control-voltage law was taken from the earlier pedals' manuals and applied to all seven effects, citing "the hardware manuals". Three later models document a different law, centred on the knob. Claims recording the difference existed in the claim register but no decision reconciled them; one was noticed for one control only.

**Correction.** The single law is kept as a declared design choice (D-072, D-027 item 13), with the workaround stated and a question put to the owner at the listening gate.

**Prevention rule.** When a decision states a rule for N products, list beside it the claim that supports the rule for each of the N; a product with no supporting claim, or with a contradicting one, is a visible gap.

**Detection check.** Search the claim register for claims on the same subject as each shared rule and compare every one with the rule.
