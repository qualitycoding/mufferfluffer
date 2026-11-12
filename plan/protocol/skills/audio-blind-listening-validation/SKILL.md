---
name: audio-blind-listening-validation
description: Use when a plan or task generates audio or audio tooling (instruments, synthesis, DSP, effects, codecs, audio tools) and needs human listening validation. Defines the S-LISTEN phase - a blind A/B test against reference recordings, a set of demos rendered from event files, evidence delivery for phone listening, a human gate with fixed questions and responses, scoring against chance, and how to iterate on what the listener actually heard.
---

# Blind-listening and demo validation for audio work

Measured metrics (spectra, pitch error, levels) do not tell you whether audio sounds right. A listener does. This
skill turns listening into a planned, repeatable gate with evidence, questions and scoring, instead of an ad hoc
"does it sound okay?". It was derived from a physically modelled trumpet instrument whose metrics passed while a
listener could pick out the real recording in most blind pairs, by the start of each clip.

## When it applies

Include the phase whenever the deliverable produces sound, or tooling that shapes or judges sound, and a
reference exists or can be recorded. Skip it only with a recorded reason (no audible output, no reference and no
listener available), as a `D-###` decision in a plan.

## Procedure

### 1. Fix the reference set and the metrics first
- Choose reference recordings under planning-protocol Rule 15. Prefer redistributable ones (own recordings, CC0, CC BY
  and similar) and say what was searched. For a hobby project, restrictive recordings (non-commercial, no-derivatives,
  personal-use site terms) may be used as **private references** when the redistributable pool cannot meet the need:
  record the source URL, author, licence or terms URL, checksum and a `redistributable` flag for each, keep the files out
  of the repository, and download them on demand (the human downloads them if the sandbox cannot reach the site). Refuse
  unknown licences and recordings with no named author or original source page.
- Freeze quantitative thresholds (for example harmonic envelope error, spectral centroid, onset time) before any
  listening. Listening gates sit on top of the metrics; they never replace them and never relax them.

### 2. Build the blind pairs (`scripts/make_blind_pairs.py`)
- 12 pairs by default. Each pair holds one reference clip and one candidate clip of the same note, patch or phrase,
  cut to 1.0 s, scaled to a common RMS (−23 dBFS), faded at the cut ends, written as 24-bit WAV.
- Which side (A or B) holds the reference is drawn from a seeded RNG. The key is stored base64-encoded and is not
  decoded until the listener has answered.
- Cut at least one set with `--window start`. The onset is where candidates most often differ from a real source
  (a clean attack, no breath or noise, no pitch settling). A set cut from the middle of a sustained note tests only
  the steady state.
- Use a new seed after every round whose results the listener has been told. Telling the listener which pairs were
  right or wrong leaks the key for that seed.

### 3. Build the demos
- Define each demo as a text event file (time, event type, parameters) that a render tool turns into a WAV
  headlessly, so demos are reproducible and diffable. Cover: a short melody, a fast passage, a slow sustained
  passage, dynamics from softest to loudest, every expressive control the product has, the lowest and highest
  registers, and any mode that is known to be weak.
- Render at the product's working sample rate. Name files by number and topic (`demo-03-dynamics.wav`).

### 4. Deliver the evidence for listening on a phone
- Build the bundle in CI from a marker commit (message contains `[gate-evidence <gate-id>]`) and push it to an
  orphan branch `evidence/<gate-id>`, with `SHA256SUMS` and attribution for the reference material. The job has its
  own concurrency group so unrelated runs cannot cancel it. If any reference is a private reference, the branch holds
  no reference-derived audio (the pairs included): it carries the demos, a link record for every reference and the
  hashes of the private pairs, and the pairs reach the listener privately, for example as files in the conversation.
  The tooling checks a per-reference `redistributable` flag and treats a missing flag as false.
- Give the listener a zip URL, `https://github.com/<owner>/<repo>/archive/refs/heads/evidence/<gate-id>.zip`, and
  per-file raw URLs. For a private repository, say so and offer another route.
- Ask the listener to play the pairs in order on their own headphones, A then B, one listen each, and write down
  which side they take to be the reference.

### 5. The gate
Write `GATE-<id>.md` with the bundle location, the metric table, and these questions:
1. For each of the pairs, which side is the reference (letters in order)?
2. Do the demos sound realistic? Which, if any, sound off, and how (too bright, too clean, synthetic, wrong pitch)?
3. One question per known weak spot (for example a register, an expressive control, a mode).

Allowed responses: `proceed`, `iterate: <notes>`, `proceed-with-rescope: <text>`, `stop`. Add a named override for a
specific accepted shortfall when the plan has one. Record the answer in `GATE-<id>.RESPONSE.md`.

### 6. Score (`scripts/score_blind.py`)
- Decode the key only now. Chance is half the pairs. For 12 pairs, 10 correct gives one-sided p ≈ 0.019, 9 gives
  0.073, 7 gives 0.387.
- Pass condition to freeze in the plan before listening, for example "no more than 8 of 12 correct". The listener's
  qualitative remarks about demos count alongside the score; a passing score with "the pedal tone sounds synthetic"
  is a finding, not a pass.
- A listener who answers one letter for nearly every pair (for example A for 10 of 12) is showing side bias, which
  is itself evidence they could not hear a difference on those pairs.

### 7. Iterate on what was heard, not on what was measured
When the response is `iterate`:
1. Use the listener's words to choose the analysis. If they say the start gave it away, compare the first 50 to 200 ms
   of the correctly identified pairs: pitch trajectory, level envelope, noise and high-frequency share, jitter.
2. Put the measured difference in the iteration log, make one targeted change per difference, and re-run the
   metric thresholds so the change does not break them.
3. Rebuild the bundle with a new seed and ask the same questions again.
4. Maximum 3 rounds. After round 3, escalate with the residual differences and let the human choose between
   `proceed-with-rescope` and `stop`.
5. Do not tune to the specific clips in the pair set. Changes must be explained by a general mechanism (an attack
   transient model, a noise source, a level envelope), not by per-note constants.

### 8. Record
Keep per round: seed, pass condition, score, p-value, the listener's remarks verbatim, the measured differences, the
change made, and the commit. Credentials for the evidence branch or the repository never enter the record.

## Using it from a planning protocol
The planning protocol (`protocols/planning-protocol.md`, section 3.10) adds step `S-LISTEN` and a gate to every
plan that has the `audio` mode flag. `S-LISTEN` points to this skill and fixes the reference set, pass condition,
demo list, gate id and round limit in the plan, so the implementer asks nothing.

## Dependencies of the scripts
Python 3 with `numpy` and `soundfile`; `scipy` when sample rates differ. `make_blind_pairs.py` takes two
directories of WAV files with matching file names. Adapt the clip preparation (window, length, loudness) to the
material: percussive sounds need a shorter window, and loudness-matching by RMS may need A-weighting for
broadband material.
