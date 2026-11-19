# Listening demos for gate G-LISTEN

Rendered by `tools/render_demos.py` with `muff_render` (the effect driven through `muff::BlockRunner`, the path of
the VST3 plug-in) from the dry inputs of `tools/make_demo_inputs.py`. Every file is 48 kHz, 24-bit, stereo, and
starts with 3 s of the dry input followed by the processed signal. Each file is peak-normalised to
-3 dBFS with one gain for the whole file, so the dry and processed sections keep their level
relation. "Processed peak" is the peak of the processed section before normalisation, with 0 dBFS = 1.0 in core
units; above +9 dBFS it is marked loud (expected for demos 03, 12 and 23). Settings not named are
at their defaults. Mono demos carry what a mono host hears on both sides; stereo demos carry the effect's left and
right outputs.

| Demo | Effect | Input | Output | Settings | Processed peak (dBFS) | Gain applied (dB) | Note |
|---|---|---|---|---|---|---|---|
| demo-01-lowpass-sweep | Fluff Lowpass | saw bass | mono | resonance 5; cutoff 60 Hz to 8000 Hz over 0-10 s; cutoff 8000 Hz to 60 Hz over 10-20 s | -10.42 | +3.04 |  |
| demo-02-lowpass-envelope | Fluff Lowpass | plucked chords | mono | cutoff 150 Hz; env_amount 4; env_speed Fast then Smooth; resonance 6 | -24.38 | +4.28 |  |
| demo-03-lowpass-selfosc | Fluff Lowpass | silence then drum pattern | mono | resonance 10; cutoff 110 Hz, 220 Hz, 440 Hz, 880 Hz for 2.5 s each; resonance 9 at 10 s; poles 2-pole, 4-pole for 5 s each from 10 s | +12.04 | -15.04 | loud |
| demo-04-ring-tremolo | Fluff Ring | held chord | mono | range Lo; frequency 4 Hz to 30 Hz over 0-20 s | -6.29 | +3.27 |  |
| demo-05-ring-bells | Fluff Ring | plucked tones | mono | range Hi; frequency 350 Hz; mix 0.5 then 1 | -6.27 | +3.27 |  |
| demo-06-ring-lfo | Fluff Ring | plucked tones | mono | frequency 500 Hz; lfo_wave Sine then Square; lfo_rate 0.5 Hz then 6 Hz; lfo_amount 1 then 3 | -6.73 | +3.73 |  |
| demo-07-phase-slow | Fluff Phase | held chord | mono | stages 6-stage then 12-stage; lfo_range Lo; lfo_rate 0.2 Hz; lfo_amount 0.6; resonance 3 | -5.05 | +2.05 |  |
| demo-08-phase-resonant | Fluff Phase | saw bass | mono | stages 12-stage; resonance 9; lfo_rate 0.6 Hz; lfo_amount 0.8 | +0.73 | -3.73 |  |
| demo-09-phase-fast | Fluff Phase | plucked tones | stereo | lfo_range Hi; lfo_rate 8 Hz then 120 Hz; lfo_amount 0.4 | -7.30 | +3.91 |  |
| demo-10-delay-short | Fluff Delay | drum pattern | mono | range Short; time 120 ms; feedback 4; mix 0.4 | -10.64 | +7.15 |  |
| demo-11-delay-long-dark | Fluff Delay | plucked tones | mono | range Long; time 600 ms; feedback 6.5; mix 0.5 | -11.46 | +3.91 |  |
| demo-12-delay-runaway | Fluff Delay | one plucked note | mono | range Short; time 300 ms; feedback 7.5 to 10 over 0-10 s; feedback 3 at 10 s; time 300 ms to 150 ms over 10-12 s | -1.32 | -1.68 | expected above +9 dBFS by the plan; not reached |
| demo-13-delay-lfo | Fluff Delay | held chord | mono | time 250 ms; lfo_wave Sine then Triangle then Random step; lfo_rate 0.8 Hz then 5 Hz then 4 Hz; lfo_amount 3 then 2 then 4 | -6.62 | +3.27 |  |
| demo-14-bank-static | Fluff Bank | drum pattern | mono | voicing Mids then Bass; pattern 1; level1 1; level2 0.6571; level3 0.4286; level4 0.3143; level5 0.3143; level6 0.4286; level7 0.6571; level8 1 | -8.42 | +5.42 | mono vs stereo left: 16 % RMS difference |
| demo-15-bank-patterns | Fluff Bank | held chord | mono | voicing Mids; pattern 2, 4, 9, 11 for 5 s each; rate 6 Hz; envelope 2 | -9.02 | +3.27 |  |
| demo-16-bank-envelopes | Fluff Bank | held chord | stereo | pattern 12; rate 3 Hz; envelope 0, 2, 5, 6, 8 for 4 s each | -10.54 | +3.27 |  |
| demo-17-bank-lfo | Fluff Bank | saw bass | mono | voicing Bass; pattern 2; lfo On; sweep 0.3 | -8.27 | +3.04 |  |
| demo-18-osc-follow | Fluff Osc | plucked tones | mono | frequency 220 Hz; wave 0.33; mix 0.6; env_amount 0 | -11.88 | +3.91 |  |
| demo-19-osc-sync-sweep | Fluff Osc | saw bass | mono | sync On; frequency 100 Hz to 1600 Hz over 0-20 s; wave 0.66; mix 1 | -8.85 | +3.04 |  |
| demo-20-osc-fm-env | Fluff Osc | drum pattern | mono | fm_amount 0.7; env_amount 3; frequency 80 Hz; wave 0 | -11.17 | +7.15 |  |
| demo-21-swarm-chorus | Fluff Swarm | plucked chords | stereo | range Chorus; time 0.4; lfo_rate 0.6 Hz; lfo_amount 0.3; feedback 0.2; mix 0.5 | -9.91 | +4.28 |  |
| demo-22-swarm-flange | Fluff Swarm | drum pattern then held chord | stereo | range Flange; lfo_rate 0.15 Hz; lfo_amount 0.9; feedback 0.7 then -0.7; mix 0.5 | -1.72 | -1.28 |  |
| demo-23-drive | Fluff Lowpass | plucked chords | mono | cutoff 12000 Hz; resonance 0; mix 1; drive 0, 9, 18, 28 for 5 s each | +6.02 | -9.02 | expected above +9 dBFS by the plan; not reached |
