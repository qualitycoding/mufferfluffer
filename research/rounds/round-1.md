# Round 1 (breadth)

Six parallel strands, one agent each (model tier: Sonnet; orchestrator review by the planner), plus the planner's own tool and environment probes. Raw output: `research/agents/<strand>-claims.json` and `-sources.md`; merged into `research/claims.json` and `research/SOURCES.md`.

| Strand | Questions | Claims | Outcome |
|---|---|---|---|
| `pedals-a` | Q1.1-Q1.3, Q1.8 | 57 | Controls, ranges and jacks of the three milestone-1 pedals from the manuals; 15 UNKNOWN (time constants, V/octave, circuit internals). |
| `pedals-b` | Q1.4, Q1.5 | 58 | Delay and filter-array figures; two editions of the delay manual distinguished; pattern tables not readable (graphics). |
| `pedals-c` | Q1.6, Q1.7 | 49 | Oscillator-box and chorus/flanger figures; 12 UNKNOWN (bucket-brigade parts, FM depth, sync threshold). |
| `dsp` | Q2.1-Q2.8 | 68 | Ladder, bucket-brigade, phaser, oscillator, follower, anti-aliasing and state-variable filter literature. |
| `legal` | Q4.1-Q4.4 | 38 | Licences of JUCE, the VST3 SDK and Rack; mark ownership; public artefacts. |
| `references` | Q5.1-Q5.3 | 35 | No redistributable recording and no dry/wet pair for any pedal. Interrupted once by a rate limit and resumed. |
| planner | Q3.1-Q3.9 | 19 | Pins by `git ls-remote`, runner labels, build spikes. |

## Contradictions found
- A magazine review says the ring modulator's switch selects the carrier's shape; the manual says the LFO's. The manual is followed (D-021).
- The delay manual gives two descriptions of where feedback oscillation starts ("above 8" and "about 3 o'clock"); both are near 80 % of the travel; D-023 uses 8.
- A third-party page gives the phaser's slowest LFO rate as 0.1 Hz; the manual says 0.01 Hz; the manual is followed.
- The brief to the DSP strand assumed conference papers carry a Creative Commons notice; none was visible in the text read (R-014).

## Method limits
Direct downloads were refused by the egress proxy for most hosts, so manuals and papers were read through a summarising fetch tool; locators are approximate (R-001). Manufacturer pages, the USPTO, several audio hosts and the FSF licence list were not reachable.

## Status of the question tree after round 1
Answered: Q1.1-Q1.8 (figures, single publisher), Q2.1-Q2.8, Q3.1-Q3.6, Q4.1-Q4.4, Q5.1-Q5.3. Open: Q1.9 (list of undocumented facts: closed by D-027), Q3.7-Q3.9 (closed by planner probes in round 2), independent re-reading of the load-bearing pedal figures (round 2).
