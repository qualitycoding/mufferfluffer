# Gate G-LISTEN: listening validation, round 1

Step S-LISTEN. Run: `gen-20261111T151412Z-mufferfluffer-vst-vcv`.

There is no reference set (D-040: no redistributable recording and no dry/wet pair exists), so this gate is a listening check of demos made by this project, not a blind A/B test. Every file below is this project's own work, under the repository licence.

## What to listen to

- Zip of the whole bundle: https://github.com/qualitycoding/mufferfluffer/archive/refs/heads/evidence/G-LISTEN.zip
- Branch: `evidence/G-LISTEN`; `INDEX.md` (settings, processed-section peak before normalisation and the gain applied for every demo): https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/INDEX.md ; checksums: https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/SHA256SUMS
- Rendered from commit `62e54c5` (the marker commit `e38a9b0` is an empty commit on top of it), by `evidence.yml` run https://github.com/qualitycoding/mufferfluffer/actions/runs/44468626667. Re-rendering gives identical files (the checksums above).
- Behavioural tests on that commit: `100% tests passed, 0 tests failed out of 98` (the core suite, run 44468626466, job core-linux). The frozen behavioural tests of the seven effects (ranges, frequencies, times) stand under this gate and are never relaxed by it.
- Listening: headphones. Each demo starts with three seconds of the unprocessed input; the rest is processed. 48 kHz, 24-bit, stereo files, each peak-normalised to -3 dBFS (`INDEX.md` gives the true level of each processed section, so differences in loudness between demos are not hidden by the normalisation). You may compare, privately, with any recording or official demonstration of the hardware you have access to. If you have the hardware and can record a dry signal and the same signal through it with the settings noted, say so in your answer and a blind comparison will be added.

| File | Effect | Input | Settings (every other control at its default, active on) |
|---|---|---|---|
| [demo-01-lowpass-sweep.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-01-lowpass-sweep.wav) | Fluff Lowpass | saw bass | resonance 5; cutoff 60 Hz to 8000 Hz over 0-10 s; cutoff 8000 Hz to 60 Hz over 10-20 s |
| [demo-02-lowpass-envelope.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-02-lowpass-envelope.wav) | Fluff Lowpass | plucked chords | cutoff 150 Hz; env_amount 4; env_speed Fast then Smooth; resonance 6 |
| [demo-03-lowpass-selfosc.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-03-lowpass-selfosc.wav) | Fluff Lowpass | silence then drum pattern | resonance 10; cutoff 110 Hz, 220 Hz, 440 Hz, 880 Hz for 2.5 s each; resonance 9 at 10 s; poles 2-pole, 4-pole for 5 s each from 10 s |
| [demo-04-ring-tremolo.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-04-ring-tremolo.wav) | Fluff Ring | held chord | range Lo; frequency 4 Hz to 30 Hz over 0-20 s |
| [demo-05-ring-bells.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-05-ring-bells.wav) | Fluff Ring | plucked tones | range Hi; frequency 350 Hz; mix 0.5 then 1 |
| [demo-06-ring-lfo.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-06-ring-lfo.wav) | Fluff Ring | plucked tones | frequency 500 Hz; lfo_wave Sine then Square; lfo_rate 0.5 Hz then 6 Hz; lfo_amount 1 then 3 |
| [demo-07-phase-slow.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-07-phase-slow.wav) | Fluff Phase | held chord | stages 6-stage then 12-stage; lfo_range Lo; lfo_rate 0.2 Hz; lfo_amount 0.6; resonance 3 |
| [demo-08-phase-resonant.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-08-phase-resonant.wav) | Fluff Phase | saw bass | stages 12-stage; resonance 9; lfo_rate 0.6 Hz; lfo_amount 0.8 |
| [demo-09-phase-fast.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-09-phase-fast.wav) | Fluff Phase | plucked tones | lfo_range Hi; lfo_rate 8 Hz then 120 Hz; lfo_amount 0.4 |
| [demo-10-delay-short.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-10-delay-short.wav) | Fluff Delay | drum pattern | range Short; time 120 ms; feedback 4; mix 0.4 |
| [demo-11-delay-long-dark.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-11-delay-long-dark.wav) | Fluff Delay | plucked tones | range Long; time 600 ms; feedback 6.5; mix 0.5 |
| [demo-12-delay-runaway.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-12-delay-runaway.wav) | Fluff Delay | one plucked note | range Short; time 300 ms; feedback 7.5 to 10 over 0-10 s; feedback 3 at 10 s; time 300 ms to 150 ms over 10-12 s |
| [demo-13-delay-lfo.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-13-delay-lfo.wav) | Fluff Delay | held chord | time 250 ms; lfo_wave Sine then Triangle then Random step; lfo_rate 0.8 Hz then 5 Hz then 4 Hz; lfo_amount 3 then 2 then 4 |
| [demo-14-bank-static.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-14-bank-static.wav) | Fluff Bank | drum pattern | voicing Mids then Bass; pattern 1; level1 1; level2 0.6571; level3 0.4286; level4 0.3143; level5 0.3143; level6 0.4286; level7 0.6571; level8 1 |
| [demo-15-bank-patterns.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-15-bank-patterns.wav) | Fluff Bank | held chord | voicing Mids; pattern 2, 4, 9, 11 for 5 s each; rate 6 Hz; envelope 2 |
| [demo-16-bank-envelopes.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-16-bank-envelopes.wav) | Fluff Bank | held chord | pattern 12; rate 3 Hz; envelope 0, 2, 5, 6, 8 for 4 s each |
| [demo-17-bank-lfo.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-17-bank-lfo.wav) | Fluff Bank | saw bass | voicing Bass; pattern 2; lfo On; sweep 0.3 |
| [demo-18-osc-follow.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-18-osc-follow.wav) | Fluff Osc | plucked tones | frequency 220 Hz; wave 0.33; mix 0.6; env_amount 0 |
| [demo-19-osc-sync-sweep.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-19-osc-sync-sweep.wav) | Fluff Osc | saw bass | sync On; frequency 100 Hz to 1600 Hz over 0-20 s; wave 0.66; mix 1 |
| [demo-20-osc-fm-env.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-20-osc-fm-env.wav) | Fluff Osc | drum pattern | fm_amount 0.7; env_amount 3; frequency 80 Hz; wave 0 |
| [demo-21-swarm-chorus.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-21-swarm-chorus.wav) | Fluff Swarm | plucked chords | range Chorus; time 0.4; lfo_rate 0.6 Hz; lfo_amount 0.3; feedback 0.2; mix 0.5 |
| [demo-22-swarm-flange.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-22-swarm-flange.wav) | Fluff Swarm | drum pattern then held chord | range Flange; lfo_rate 0.15 Hz; lfo_amount 0.9; feedback 0.7 then -0.7; mix 0.5 |
| [demo-23-drive.wav](https://raw.githubusercontent.com/qualitycoding/mufferfluffer/evidence/G-LISTEN/demo-23-drive.wav) | Fluff Lowpass | plucked chords | cutoff 12000 Hz; resonance 0; mix 1; drive 0, 9, 18, 28 for 5 s each |

Notes on the demos that you may find useful:
- Demo 12 (Delay runaway): Feedback rises from 7.5 (not 5, as the plan said) to 10 over ten seconds; with the planned 5 to 10 the loop never ran away, so there was nothing to listen to (recorded in `DEVIATIONS.md`).
- Demos 12 and 23 do not reach +9 dBFS before normalisation, as the plan expected: the soft limiter of D-037 saturates at +6.02 dBFS and the delay loop is bounded below that. Demo 03 reaches +12.04 dBFS for a few samples at the moment the Lowpass switches from 2-pole to 4-pole; the steady self-oscillation is about +6.3 dBFS. That click sets the gain of the whole file, so demo 03 sounds quieter than its true level.
- Demo 02 (Lowpass with envelope) is about 13 dB quieter than its dry section: the ladder has no make-up gain (D-030, D-027 item 4).
- Demo 16 has one second of exact silence early in the processed part: Envelope 0 with the pulse pattern at 3 steps per second leaves a gap between steps.

## Design choices that were not documented anywhere (D-027)

These could not be read from any source; each is a reasoned choice, frozen by a test. Tell me which you want changed.
1. Drive range -7 to +28 dB for every effect, into a limiter of shape `2 tanh(x/2)`.
2. Output range -24 to +12 dB.
3. Envelope follower times: Lowpass Fast 2 ms / 50 ms, Smooth 20 ms / 300 ms (attack / release); Oscillator Box 5 ms / 100 ms.
4. Lowpass 2-pole mode keeps the four-stage feedback loop and taps stage 2; no make-up gain as resonance rises; cutoff limited to 20 kHz with resonance faded out between 16 and 20 kHz.
5. Ring modulator: ideal multiplier, pure sine carrier; no noise gate.
6. Phaser: stage corner 1.1 kHz at Sweep 5; resonance feedback 0 to 0.9, never self-oscillating.
7. Delay: logarithmic Time knob; fixed two-pole filters; no compander, no clock noise; LFO depth of one octave either way at Amount 10; feedback law linear with unity at 8.
8. Filter bank: Q = 5; Rate 0.5 to 32 steps per second; sweep depth half an octave either way; **its own pattern set**.
9. Oscillator box: exponential FM of 2 octaves per core unit at full Amount; sync threshold +/-0.02; linear VCA; pulse width 10 % at the end of the Wave travel.
10. Chorus: logarithmic Time knob and LFO law; wet bandwidth `min(16 kHz, 200/d)`; unity loop gain at |feedback| 0.8; no compander.
11. Not modelled anywhere: MIDI, host tempo sync, preset storage, noise, component tolerance and drift.
12. Lowpass cutoff knob law: logarithmic from 15 Hz to 12 kHz, which puts the knob's centre at 424 Hz.
13. Control voltages on the Delay, Filter bank and Chorus add to the knob like every other control voltage (0 V leaves the setting; 5 V adds the full range), one law for all seven effects (D-072).

## Questions (fixed)

- **L-1** Do the demos sound like the effects they model, as far as you know the hardware? Which, if any, sound wrong, and how (too clean, too bright, too dull, wrong sweep range, wrong speed, wrong level)?
- **L-2** Lowpass (demos 01 to 03): does the resonance and self-oscillation behave as you expect, and do the Fast and Smooth envelope speeds feel right?
- **L-3** Weak spot, aliasing: in demos 03, 08, 12, 19 and 23 (high resonance, runaway feedback, synced oscillator sweep, high drive) do you hear inharmonic whistles, roughness or metallic tones that do not belong?
- **L-4** Weak spot, delay and chorus character (demos 10 to 13, 21, 22): the bucket-brigade circuits are represented by a clocked delay line and a low-pass filter (no companding, no clock noise, no clock-rate sampling). Is the repeat tone too clean or otherwise unlike what you expect?
- **L-5** Weak spot, filter bank (demos 14 to 17): the patterns are this project's own, not the hardware's. Are the patterns and envelope shapes acceptable, or do you want the hardware patterns transcribed (R-003)?
- **L-6** Oscillator box (demos 18 to 20): does the oscillator track the playing dynamics and does hard sync sound right?
- **L-7** Levels: is any demo much louder or quieter than its dry section?
- **H-1, H-2** (hands-on checks in a VST3 host and in VCV Rack): G-002 has no `proceed` answer, so no binaries exist. These need a local build per `README.md`; if you made one, you may answer: H-1 do all seven plug-ins load in your VST3 host and all seven modules in VCV Rack, and does every control do something audible? H-2 in Rack, with a control's knob at its minimum, does 0 to 5 V on its CV input sweep the control over its full range (the oscillator's Wave input covers 60 % and its 1 V/octave input follows pitch, by design), and do the tap inputs follow a clock?
- **L-8** Control-voltage law (for information; no listening needed): on the Delay, Filter bank and Chorus the hardware's manuals describe control inputs centred on the knob (2.5 V leaves the setting; 0 V and 5 V move it half the range down and up), while this project uses one law for all seven effects (0 V leaves the setting; 5 V adds the full range). Changing those three is not part of this plan (D-072 item 3). If you want it, say so: the request is recorded in `GATE-G-LISTEN.RESPONSE.md` and in `COMPLETION.md` as follow-up planning work, and it does not affect the answer to this gate.

## Allowed responses

- `proceed`: S-LISTEN is done.
- `iterate: <notes>`: I make one targeted change per reported difference and push a new round (at most three rounds). Remedies that need oversampling (aliasing at high resonance or drive) change frozen dry-path tests and are not pre-authorised; I would say so in the next round and offer `proceed-with-rescope`.
- `extend-rounds: <n>`: only you can grant further rounds.
- `proceed-with-rescope: <text>`: your text is recorded as an accepted shortfall in `COMPLETION.md` and in the risk register; S-LISTEN is done.
- `stop`: `BLOCKED.md`, then the retrospective.
- `accept-own-patterns`: answers L-5 for good; R-003 is closed as accepted.

You may reply in this session, or commit `GATE-G-LISTEN.RESPONSE.md` to the branch.
