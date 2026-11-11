# Question tree (R1)

Profile branch: `software` (engineering). `math`, `computational` and `publication` branches are N/A. Each leaf names what it informs. IDs of decisions, tests and steps are those of `plan/DECISIONS.md`, `plan/TRACEABILITY.md` and `plan/PLAN.md`. Status is updated at the end of every round.

## Q1 Target behaviour of each pedal (informs D-020..D-026, T-1xx..T-7xx)
- Q1.1 MF-101: controls, ranges, jacks, signal flow, 2-pole/4-pole derivation, envelope follower speeds. → D-020, T-101..T-108
- Q1.2 MF-102: carrier ranges (LO/HI), LFO rate range and waveforms, amount depth, carrier in/out. → D-021, T-201..T-206
- Q1.3 MF-103: stage count and switch, sweep range, LFO ranges, resonance, aux output. → D-022, T-301..T-306
- Q1.4 MF-104M: delay ranges, feedback and self-oscillation, LFO waveforms, loop insert, delay-only output. → D-023, T-401..T-407
- Q1.5 MF-105M: eight centre frequencies per voicing, pattern structure, envelope shapes, rate range, stereo split. → D-024, T-501..T-507
- Q1.6 MF-107: VCO range, waveform control, sync, FM, envelope amount, relation of VCO loudness to input. → D-025, T-601..T-606
- Q1.7 MF-108M: delay ranges per range position, bipolar feedback, LFO waveforms, stereo outputs. → D-026, T-701..T-706
- Q1.8 CV input scaling common to the family (volts for full sweep, volts per octave). → D-012, T-020..T-023
- Q1.9 Which facts are undocumented and must become owner-visible design choices. → D-027, R-register

## Q2 Modelling methods (informs D-030..D-037)
- Q2.1 Ladder filter model, self-oscillation, oversampling need. → D-030, T-103, T-104
- Q2.2 Bucket-brigade delay model: clock/delay relation, bandwidth versus delay time, companding. → D-033, D-036, T-404, T-704
- Q2.3 Phaser: all-pass chain, notch count, feedback placement. → D-032, T-302
- Q2.4 Ring modulation: ideal multiplier versus diode ring. → D-031, T-201
- Q2.5 Alias-suppressed oscillator with hard sync and FM. → D-035, T-602, T-603
- Q2.6 Envelope follower form. → D-030, D-035, T-106
- Q2.7 Band-pass bank filter form, stability under modulation. → D-034, T-501
- Q2.8 Anti-aliasing of the drive non-linearity. → D-037, T-030

## Q3 Frameworks, build and CI (informs D-001..D-010, `plan/ENVIRONMENT.md`, S-001..S-005, S-CI)
- Q3.1 Newest stable JUCE tag, its licence options and CMake API for a VST3-only plug-in; does it build in the sandbox? → D-001, D-005, spike `juce-vst3-build`
- Q3.2 Rack SDK version, download channel, build flags (C++ standard), platforms, stub-link testing. → D-002, D-003, D-006
- Q3.3 pluginval version, flags, strictness, headless Linux use. → D-007, T-801
- Q3.4 Test framework pin (Catch2) and its licence. → D-008
- Q3.5 Hosted runner labels that exist for the platforms of A-006 and the default extras of 3.9. → D-009, CI matrix
- Q3.6 Action pins (checkout, cache, upload/download-artifact, setup-msys2). → `plan/ENVIRONMENT.md`
- Q3.7 Mutation tool pin (Mull), its compiler pairing, report format, runtime price. → D-060, S-MUTATE
- Q3.8 Dependency vulnerability scan command. → D-061, T-903
- Q3.9 Real-time safety check (allocation counting) and sanitizer jobs. → T-031, CI

## Q4 Licensing, naming, publication (informs D-001, D-004, D-041, G-002)
- Q4.1 Licence of the whole when linking JUCE under its open option; Rack plug-in licence exception; VST3 SDK licence. → D-001
- Q4.2 Trade marks of the manufacturer and of Steinberg ("VST"); VCV rules on cloning. → D-004, T-904
- Q4.3 Are artefacts and branches of a public repository public? → D-041, G-002
- Q4.4 May tables in the manuals (patterns, frequencies) be re-expressed in code? → D-024, R-register

## Q5 Listening references (informs D-040, S-LISTEN, G-LISTEN)
- Q5.1 Redistributable recordings of the hardware (CC0, CC BY, CC BY-SA), with settings and dry counterparts. → D-040
- Q5.2 Restrictive but personally usable recordings. → D-040
- Q5.3 If no dry/wet pair exists: what is the candidate compared with? → D-040, G-LISTEN pass condition

## Pruned
- Host tempo sync, MIDI implementation charts, preset formats: inform nothing (out of scope, A-007).
