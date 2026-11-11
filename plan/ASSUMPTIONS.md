# Assumptions and intake record (A-###)

Intake batch presented 2026-11-11; the owner replied "Ok proceed with the defaults", so every item below is the proposed default, adopted. One further instruction was received during planning and is recorded as A-015.

## Restated task (0.3.1)
- **Goal.** An original DSP reimplementation of the behaviour of the seven Moogerfooger analogue effect pedals, as one shared C++ core delivered (i) as VST3 plug-ins and (ii) as a VCV Rack 2 plug-in, in `github.com/qualitycoding/mufferfluffer`.
- **In scope.** Seven effects (A-001); every documented front-panel control and every documented control-voltage or expression jack (A-007); a VST3 build and a Rack 2 build for the platforms of A-006; automated tests, CI, code review, mutation testing of the DSP core, and a listening gate.
- **Out of scope.** The CP-251 control processor; the plug-in-only MF-109S saturator; AU, AAX, CLAP and LV2 formats; tempo synchronisation to a host, MIDI control, presets banks and any other feature that exists only in the manufacturer's own plug-ins (A-007); any public release, VCV Library submission or binary distribution (A-009); the manufacturer's code, presets, names, logos and panel artwork (A-002, A-003).
- **Success criteria** (all measurable; each maps to tests in `plan/TRACEABILITY.md`):
  - SC-1 All seven effects exist in the core and expose exactly the parameters and ports tabulated in `plan/DECISIONS.md` D-020 to D-026 (table-equality tests).
  - SC-2 Each effect's measured behaviour meets its frozen behavioural tests (ranges, frequencies, delay times, notch counts, sideband positions, pattern timing) within the stated tolerances.
  - SC-3 Seven VST3 plug-ins build on every platform of A-006 and each passes pluginval at strictness level 10 on Linux x64, Windows x64 and macOS arm64.
  - SC-4 One Rack 2 plug-in holding seven modules builds against the pinned Rack SDK on every platform of A-006; its manifest and panels pass the validators.
  - SC-5 The VST3 adaptor and the Rack adaptor drive the same core: for equal parameter values and input, their outputs differ by at most 1e-6 (full scale = 1.0).
  - SC-6 Each effect uses less than 5 % of one core at 48 kHz with 64-sample blocks on the `ubuntu-24.04` x64 hosted runner (A-011).
  - SC-7 For any parameter values, any finite or non-finite input and any supported sample rate, every output sample is finite and its magnitude is at most 4.0 (core units; 1.0 = nominal full scale).
  - SC-8 The audio thread path (`process`) performs no heap allocation and takes no lock.
  - SC-9 No file in the repository, apart from the allow-listed research and plan documents, contains the manufacturer's marks (naming lint).
  - SC-10 The listening gate `G-LISTEN` is passed (response `proceed` or `proceed-with-rescope`).
- **Constraints.** C++ (standard decided in D-002), CMake, JUCE and the Rack 2 SDK at the versions pinned in `plan/ENVIRONMENT.md`; hosted GitHub Actions runners only; licence AGPL-3.0-only (A-005); hobby project (A-013).

## Records
| ID | Topic | Adopted value | Consequence |
|---|---|---|---|
| A-001 | Which effects | All seven: lowpass filter (MF-101), ring modulator (MF-102), 12-stage phaser (MF-103), analogue delay (MF-104M), resonant filter array (MF-105M), oscillator box (MF-107), chorus/flanger (MF-108M); three milestones: M1 = 101, 102, 103; M2 = 104M, 108M; M3 = 105M, 107. CP-251 and MF-109S excluded. | Plan size; milestone order of steps. |
| A-002 | What is cloned | The behaviour of the hardware pedals, taken from user manuals and published analyses. No manufacturer code, presets or artwork. | Research sources; R7 permitted use. |
| A-003 | Naming | Own names and own panel design under "Mufferfluffer". No manufacturer marks in product names, identifiers, panels or UI; one "inspired by" sentence in the README. | SC-9 naming lint; D-004. |
| A-004 | Packaging | One VST3 per effect (seven); one Rack plug-in with one module per effect. | Build targets; CI time. |
| A-005 | Stack and licence | C++, CMake, JUCE for VST3, Rack 2 SDK; repository licence GPL-family as the pinned JUCE requires: AGPL-3.0-only. | See D-001; `LICENSE` on `main`. |
| A-006 | Platforms | Windows x64, macOS arm64, macOS x64 (Intel), Linux x64; all built in CI. No AU, AAX or CLAP. | CI matrix (3.9). |
| A-007 | Control | Every hardware CV or expression jack is a CV input in Rack and an automatable parameter in the VST3. Extras of the manufacturer's plug-ins (host tempo sync, CV sharing between instances) are out of scope. | Port tables D-020 to D-026. |
| A-008 | Listening | The owner is the listener. Redistributable recordings of the hardware are searched first; private references are allowed (hobby). The owner is assumed to own no pedals to record. | 3.10; D-040. |
| A-009 | Release | None: `software.deploys = false`; no VCV Library submission. | `G-002` covers any binary upload (the repository is public). |
| A-010 | Profiles | `software` with `audio`; mutation testing with Mull on the DSP core only. | 3.8, 3.10. |
| A-011 | Performance | Each effect under 5 % of one core at 48 kHz, 64-sample blocks, on the Linux x64 CI runner; sample rates 44.1 kHz to 192 kHz supported. Stated at intake as an *unmeasured default*; the measured value is in `research/spikes/` (see D-050). | SC-6; T-PERF tests. |
| A-012 | Stores and tags | Lessons to `qualitycoding/Lessons`, knowledge to `qualitycoding/knowledge`. Tags: `domain:audio`, `domain:dsp`, `tech:cpp`, `tech:cmake`, `tech:juce`, `tech:vst3`, `tech:vcv-rack`, `tech:github-actions`, `tech:mull`. | R0 matching; S-KNOW. |
| A-013 | Project nature | `hobby` (personal, non-commercial). Reference material: redistributable preferred, private references allowed under Rule 15. | 3.10 reference set. |
| A-014 | Profile selection | `software` + `audio`; not `computational` (no numerical result is the deliverable), not `publication`. | `plan/PROFILE.md`. |
| A-015 | Push cadence (owner instruction, 2026-11-11 15:13 UTC) | "Push the plan and artefacts as each phase is finalised so we don't lose work." | Every phase closes with a commit, a push and an `ls-remote` check (also Rule 6). |
| A-016 | Empty repository | `main` initialised with `README.md` and `LICENSE` only, so the generation branch is not the default branch. | Done: `main` at the commit recorded in `state.json`. |
| A-017 | Generative-AI disclosure | Not applicable (no venue); AI authorship is visible in commit trailers. | none |
