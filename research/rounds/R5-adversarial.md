# R5 adversarial pass (fresh context, 2026-11-12)

Scope: decisions D-001, D-002, D-004, D-005, D-006, D-007, D-009, D-012, D-016, D-024, D-030, D-033/D-036, D-035, D-037, D-040, D-050, D-060, D-061 as written in `plan/DECISIONS.md` when this pass started (later additions to PLAN/GATES/RISKS/TRACEABILITY/HANDOFF were not re-read).

Method: read the frozen headers, fixture, tests, spike results and `research/claims.json`; read the pinned JUCE (`d60b7a7`) and Rack (`061ccf6`) checkouts in `/home/claude/deps`; built the planning oracle (`research/spikes/oracle/oracle_core.cpp`) with throw-away probe drivers in `/tmp/r5/` (not committed) to measure aliasing, ladder behaviour and denormal cost; about ten web searches/fetches (cut short by a rate limit). "Verified" = I ran or read it myself; "inferred" = reasoning without a direct check. The oracle is not the deliverable: oracle measurements show what the *specified* algorithm does, unless the decision text requires something the oracle leaves out (stated where so).

Severity = likelihood x cost, Critical / High / Medium / Low.

---

## Summary table

| # | Decision | Finding | Sev. | Status |
|---|---|---|---|---|
| 1 | D-060 | The pinned install source (Cloudsmith `mull-stable`) is the one Mull's own changelog tells users not to use: "Cloudsmith bandwidth is exhausted again" (0.37.0, 2 Nov 2026, and 0.38.0, 12 Nov 2026). `mull-runner-N` deprecated in 0.36.0. Claim C-PLANNER-012 ("newest tag 0.37.0") is stale today. | High | verified |
| 2 | D-030 | With Cutoff 12 kHz, Amount 5, Resonance 10 the cutoff hits the 0.45 fs clamp and the ladder self-oscillates at 0.45 fs: at 44.1 kHz that is a loud (peak about 1.4) **19.8 kHz whistle**, audible, where the hardware oscillates at about 40 kHz (ultrasonic). The pre-decided remedy R-006 (run twice, keep the second result) has no decimation filter and so does not remove aliasing. | High | verified (oracle) |
| 3 | D-035 | A two-sided polyBLEP cannot be applied causally to an externally triggered sync reset (it needs the sample *before* the event). The decision demands it but gives no latency or one-sided variant; the oracle's un-band-limited sync has inharmonic components at **-32 dB**; there is no test. C-DSP-061 (FM with polyBLEP), load-bearing, is "unverified". | High | verified (oracle) + inferred |
| 4 | D-037 | The tests are **not** sample-exact (tolerances 1e-3 / 2e-3) and they constrain only the **dry** tap (Mix 0, bypass). Anti-aliasing of the drive that feeds the **wet** path is not blocked by any frozen test, so the stated reason for rejecting it is wrong. Measured naive drive +28 dB: alias -21 dB (4.2 kHz input), -38 dB *below* the fundamental (3.1 kHz input). | High | verified |
| 5 | D-016 | Stereo input is summed to mono `0.5(L+R)` even when `active` = Bypass, so "bypass" on a stereo track removes the stereo image and cancels anti-phase content; FilterBank's hardware bypass sends the signal **hard left** in a stereo host. Contract does not require denormal protection (measured 13x CPU rise for FilterBank in silence). Finite tails above about 11 000 s at 192 kHz overflow JUCE's `int` conversion in `getTailSamples`. | Medium-High | verified |
| 6 | D-050 | The gate measures a 220 Hz sine only. Oracle in silence after signal (no FTZ): FilterBank 0.71 % -> **9.3 %** (over the 5 % limit), Phaser 0.57 -> 3.3 %, Delay 1.0 -> 3.0 %. The gate passes while the denormal case fails. | Medium-High | verified (oracle) |
| 7 | D-009 | `ubuntu-22.04` left active support on 17 Sep 2026 and is removed 17 Apr 2027 (brown-outs before that); `aggregate` then fails. The 22.04 image puts a new CMake (3.31.x) first on PATH, so "CMake 3.22" is not what gets tested unless it is pinned explicitly. JS actions inside `alpine:3.22` depend on a runner-provided musl Node 24 (Node 20 removed 23 Sep 2026); not verified. | Medium | partly verified |
| 8 | D-007 | pluginval's level-6+ state-restoration test is reported to fail intermittently on JUCE VST3 `AudioParameterChoice`/`Bool` parameters (JUCE forum, reports to June 2025, about 1 run in 10). Every effect has 2 to 5 choice parameters. The `SUCCESS` / `Starting tests in:` log check rests on single-source C-PLANNER-008 and has no negative control. | Medium | verified (report) / inferred |
| 9 | D-033 | A fixed-rate line with a moving read tap does not reproduce the BBD's signature time-sweep: when the clock changes, a BBD pitch-shifts the whole stored echo for about one delay time; the plan gives a 30 ms chirp. The plan's own source (C-DSP-033, Raffel & Smith) recommends variable-rate write/read for echo with clock changes. Delay uses fixed filters while Chorus uses a clock-tracking bandwidth: an inconsistency. | Medium | inferred from cited source |
| 10 | D-024 | The legal reason cited (C-LEGAL-035) is about the **MF-104Z** manual, and its own text says facts are not covered. The real basis is C-PEDB-041 ("UNKNOWN", status unverified): the grids were unreadable to the *agent*, not to the owner. | Medium | verified (claims) |
| 11 | D-005 | 8 full JUCE builds per platform plus 7 pluginval level-10 runs; the time was measured only for GCC on Linux. MSVC and the macOS Intel runner are slower, so the 45-min revisit trigger is likely on `vst3-macos-intel`/`vst3-windows`. The spike RESULT contradicts D-005 (it recommends a shared static JUCE library, which D-005 rejects). | Medium | inferred + verified (contradiction) |
| 12 | D-002 / D-006 | Rack's `compile.mk` adds `-O3 -funsafe-math-optimizations -march=nehalem` and on macOS `-mmacosx-version-min=10.9`. The core is never *tested* under these flags (SC-5, SC-7 are asserted only in the CMake build), and some C++17 library features are unavailable at 10.9. | Medium-Low | verified |
| 13 | D-012 | `kOutputLimit` 4.0 = **±20 V** in Rack, against VCV's "no voltage should be generated beyond ±12 V". 5 V per full sweep means a standard Rack 0-10 V envelope saturates at half its travel. | Low-Medium | verified (VCV manual) |
| 14 | D-061 | OSV does index `github.com/juce-framework/juce` by git range (CVE-2021-23521 record), so the scan is not vacuous for JUCE itself, but it is **blind to JUCE's vendored libraries** (zlib, libpng, libjpeg, HarfBuzz, FLAC, Ogg/Vorbis: C-LEGAL-009), which are the real parser attack surface linked into every plug-in. For Catch2 it returns nothing in practice (test-only, harmless). | Medium | verified (OSV record) |
| 15 | D-040 | Without a reference, the gate cannot test "sounds like the hardware". The synthetic-only demo inputs leave out the instruments the pedals are used on. | Medium | inferred |
| 16 | D-004 | "Mufferfluffer" / "Fluff …" copies the sound and rhythm of MOOGERFOOGER for identical goods whose owner sells plug-ins (C-LEGAL-033). Aural similarity is a factor in likelihood of confusion; the README "inspired by" line makes the link explicit. | Low-Medium | inferred (not legal advice) |
| 17 | D-001 | Not broken. | – | checked |
| 18 | D-006 | Licence use of widgets is sound; small interface gaps only. | Low | verified |

---

## Per-decision findings

### D-001 Licence (AGPL-3.0-only)
- Not broken. Checked: the JUCE 9.0.5 `CMakeLists.txt` header offers AGPLv3 as the open option; C-LEGAL-002 (SPDX `AGPL-3.0-only OR LicenseRef-JUCE-Commercial`); the Rack licence exception and GPLv3/AGPLv3 section 13 (C-LEGAL-014/015/022/023); the VST3 SDK under MIT; widgets used through the Rack API only. Residual: C-LEGAL-018, -025, -026 are "unverified" and load-bearing, but each errs on the conservative side. Low.

### D-002 C++17
- (1) C++17 itself is sound. JUCE: every module declares `minimumCppStandard: 17` (verified, `modules/*/juce_*.h:52`). Rack: `compile.mk:21` adds `CXXFLAGS += -std=c++11`, and `compile.mk:59` appends `$(FLAGS)` afterwards, so `-std=c++17` in FLAGS wins (verified).
- The weak spot is what comes with Rack's flags. `compile.mk:17` sets `-O3 -funsafe-math-optimizations`, `:30` sets `-march=nehalem`, and `:41` sets `-mmacosx-version-min=10.9`. Two consequences:
  - (a) At the 10.9 deployment target, `std::optional::value`, `std::variant`/`std::visit`, `std::filesystem`, floating-point `to_chars` and aligned `new` for over-aligned types are unavailable. They fail **only** in `rack-macos*` (inferred from Apple libc++ availability markup).
  - (b) Reassociation and the different `-march` mean the Rack binary's numerics are never tested. SC-5 (1e-6 parity) and SC-7 are proven only for the CMake build.
- (2) Medium-Low: the failure is loud (a CI compile error) or minor (small numeric drift).
- (3) Add a CI job that builds and runs `muff_core_tests` with `-O3 -funsafe-math-optimizations -march=nehalem` (Linux, GCC). On macOS add `FLAGS += -mmacosx-version-min=10.15` after the include (the later flag wins), or forbid those library facilities in the core with a lint rule.

### D-004 Naming and trade marks
- (1) The brand "Mufferfluffer" and the "Fluff X" names mirror MOOGERFOOGER's sound and cadence for the same goods. The mark owner sells Moogerfooger plug-ins (C-LEGAL-033), and aural similarity is a recognised confusion factor (inferred; not legal advice). The README's "inspired by" line names the mark next to the look-alike brand. C-LEGAL-031 (status of MOOGERFOOGER) is "unverified".
- (2) Low-Medium: nothing is distributed (A-009), but the repository and its name are public, and a rename later touches the fixture, tests and bundle IDs.
- (3) Put the brand choice to the owner explicitly at Phase 5 as a trade-mark risk, with a neutral alternative. If kept, drop the mark from the README sentence or keep it purely descriptive.

### D-005 Seven `juce_add_plugin` targets
- (1) Evidence:
  - The time was measured once: GCC 13 on 2 vCPU Linux, 3 min 05 s per target (C-PLANNER-002). Nothing was measured for MSVC (`windows-2022`) or Apple Clang. `macos-15-intel` is the slowest hosted runner. On top come 7 pluginval level-10 runs plus the test target: inferred to land on the 45-minute trigger on the Intel macOS or Windows job.
  - Contradiction: `research/spikes/juce-vst3-build/RESULT.md` "Observations" says "seven plug-ins sharing one static JUCE library target avoid paying it seven times (D-005)", while D-005 rejects that option.
- (2) Medium.
- (3) Make `ccache`/`sccache` (keyed on the JUCE commit and compiler) the default now, not a revisit action, on all four VST3 jobs. Correct the spike RESULT text. Record per-job wall times in S-020 and gate the plan on `vst3-macos-intel` < 45 min.

### D-006 Rack shell and generated panels
- (1) Licence handling is sound: widgets are referenced through the API and their SVGs load from the Rack installation (C-LEGAL-017). Path-only labels avoid nanosvg's missing `<text>` support. `onSampleRateChange` is dispatched when a module is added (verified, Rack `src/engine/Engine.cpp:653-658`, `:778-782`), so `prepare` will run.
- Interface gaps: the widget list (RoundBlackKnob, PJ301MPort, CKSS, lights) has no 6- or 12-position selector (`lfo_wave`, `pattern`: a snap knob is needed) and no momentary button for `tap` (CKSS is a latching toggle).
- The Rack build is not exercised numerically (see D-002), and no automated test loads a module (R-012).
- (2) Low.
- (3) Name the widgets (`RoundBlackSnapKnob` for stepped parameters with more than 3 steps, `VCVButton` for `tap`). Add the Rack-flags core-test job from D-002.

### D-007 pluginval at strictness 10
- (1) Evidence:
  - JUCE forum, "Pluginval can set an AudioParameterBools to odd values", page 2 (https://forum.juce.com/t/pluginval-can-set-an-audioparameterbools-to-odd-values/50971?page=2): the state-restoration test (level 6+) fails intermittently for VST3 `AudioParameterChoice`/`Bool`. Example: "Expected value within 0.1 of: 0.666667, Actual value: 0". Reproducible on CI but not locally; reports run to June 2025 (about 1 in 10 runs); no resolved cause.
  - This project has many 2-step choice parameters (`active`, `tap`, `range`, `poles` and others) and a hand-written XML state path. Mitigating detail: every effect's parameter 0 is a float (`drive`), which one reporter said removed the failures.
  - The log check (`SUCCESS`, `Starting tests in:`) comes from single-source C-PLANNER-008 and has no negative control.
- (2) Medium: a flaky red `aggregate` with no code change.
- (3) Pass a fixed `--seed` (log it) and record it with the result. If a run fails, re-run with the same seed to separate a real defect from the flake. Add a one-off negative control in S-021: a plug-in built to fail one test must make the step fail.

### D-009 CI matrix
- (1) Evidence:
  - (a) `ubuntu-22.04`: active support ended 17 Sep 2026; deprecated support ends 17 Apr 2027 (https://www.endoflife.date/github-actions-runner-images). GitHub's practice is scheduled brown-outs before removal, so `core-oldest-toolchain`, and with it `aggregate`, will go red with no code change.
  - (b) The hosted 22.04 image installs a recent CMake ahead of `/usr/bin` (inferred from the runner-image toolsets; not re-verified). "CMake 3.22" is then not exercised unless the job installs and calls it explicitly.
  - (c) `core-musl` in `alpine:3.22`: JS actions (`actions/checkout` v7, Node 24) need the runner's Alpine Node build. Node 20 was removed on 23 Sep 2026 (github.blog changelog); whether a musl Node 24 is shipped was not verified. `git` must also be installed *before* checkout, or checkout falls back to a REST download without `.git`.
  - (d) The runner labels exist (C-PLANNER-010 corroborated). The Rack SDK file names `mac-x64`/`mac-arm64` are confirmed by a third party (shakfu gen-dsp docs) and by `arch.mk:33`. The MSYS2 package list includes `jq` and `zstd`, which `plugin.mk` needs (verified, `plugin.mk:5` and `:95`). JUCE 9.0.5 on MSVC 2022 and Apple Clang has not been built anywhere in planning (C-PLANNER-002 covers Linux GCC only).
- (2) Medium.
- (3) Move the oldest-toolchain job to `ubuntu-24.04` with `apt-get install g++-11` and a pinned CMake 3.22.x (Kitware tarball or `pip install cmake==3.22.*`), calling it by absolute path. Run Alpine by `docker run` from a host step after checkout (no JS action inside musl). Add a first-week smoke run of all four VST3 jobs before M1 work relies on them.

### D-012 CV conventions
- (1) VCV Voltage Standards (https://vcvrack.com/manual/VoltageStandards):
  - "No voltage should be generated beyond this range [±12 V]", but `kOutputLimit` 4.0 x 5 V = ±20 V. Self-oscillating Lowpass reaches about ±10.4 V; Output +12 dB can reach ±20 V.
  - CV sources are "0 to 10 V (unipolar)": with depth 1.0 per 5 V, a standard ADSR saturates the target at half its travel.
  - Trigger thresholds (0.1 V / 1 V) match VCV's Schmitt guidance: not broken.
- (2) Low-Medium (usability, no failure).
- (3) In the Rack shell, soft-limit outputs to ±11.7 V after `RackBridge` (T-821 compares adaptor paths, not the shell). Offer a context-menu "CV range: 5 V (hardware) / 10 V (Rack)" that scales CV in the shell only. Document the choice.

### D-016 VST3 shell contract (host friendliness)
- (1) Six problems:
  - **Stereo collapse in bypass.** `adapters.hpp:203` makes a stereo input `0.5(L+R)`, and D-018 defines bypass as "output = input" (that mono sum). So `active` = Bypass on a stereo track is not a bypass: the image is lost, anti-phase content cancels, and uncorrelated material drops by about 3 dB. Verified by reading.
  - **FilterBank bypass is hard left** in a stereo host. T-010 requires `out_r` < 1e-4 in bypass, and `BlockRunner` maps `rightOutput` to the right channel. Verified.
  - **Two bypasses.** `getBypassParameter()` is null, so the JUCE VST3 wrapper adds its own "Bypass" (`byps`) parameter (verified, `juce_audio_plugin_client_VST3.cpp:675-686`). Host bypass is a true stereo pass-through; `active` is the mono hardware bypass with spillover. Users will see two bypass controls with different results.
  - **Tail overflow.** `getTailSamples` does `(uint32) roundToIntAccurate(tail * fs)` (verified, line 3555). A finite tail above 2^31/fs (about 11 000 s at 192 kHz) overflows `int`. T-017 allows any finite value below 1e8 s, so Delay feedback just under 8 can produce it. Tail changes with parameters but VST3 hosts read it on activation only.
  - **Denormals not required.** The contract does not require `juce::ScopedNoDenormals`. The oracle shell has it (`research/spikes/oracle_vst3/processor.cpp:41`), but a fresh implementation may not; see D-050 for the measured cost.
  - **Momentary `tap`.** It is a 2-step automatable choice sampled once per block. A press and release within one block is lost, timing is quantised to the block, and a state saved while "Down" can fire a spurious tap on load. Low.
  - **Gap:** the default bus layout (what the constructor declares) is not stated. pluginval and hosts start from it; T-856 uses it implicitly.
- (2) Medium-High overall (the first two are user-visible in every stereo DAW session).
- (3) Re-open D-018/D-016 for VST3 only: with two input channels and `active` = Bypass, pass L and R straight through (crossfaded with the 5 ms `active` ramp); FilterBank bypass in stereo copies the input to both channels. Add `ScopedNoDenormals` to the contract. Clamp tail in the shell: return +infinity when tail exceeds 600 s. State the default layout (stereo -> stereo). Decide whether `active` is exposed as the bypass parameter or whether the host bypass is documented as the true bypass.

### D-024 Own pattern set
- (1) The decision cites "C-LEGAL 'manual text, tables and artwork are copyrighted'", which is C-LEGAL-035. That claim concerns the **MF-104Z** manual and itself says "measured facts … are not themselves covered". The load-bearing claim is C-PEDB-041, status **unverified** ("UNKNOWN … presented as graphics that could not be read as text"). That was a limit of the agent's text extraction; the owner can read the grids on pp. 28-32. The pattern names *are* known (C-PEDB-039).
- (2) Medium: the most recognisable behaviour of the bank effect will differ from the hardware, the owner may reject it at G-LISTEN, and changing patterns means re-opening the frozen fixture (T-024, T-505).
- (3) Correct the citation. Add an owner task before freezing M3: read Appendix A and transcribe the step grids into a private, git-ignored file. Let `tools/gen_tables.py` accept an optional local pattern file, keeping the committed fixture as the public default. Then decide publication separately; step grids are arguably factual, but that is a legal judgement for the owner.

### D-030 No oversampling in the non-linear ladder
- (1) Verified with the oracle (`/tmp/r5/drv2.cpp`, `drv3.cpp`), which implements D-030 exactly:
  - Tuning is excellent below the clamp: within 0.3 % at 2 to 12 kHz cutoffs, at 44.1/48/96 kHz.
  - At Cutoff 12 kHz, Amount 5, Resonance 10 with a 0.5 amplitude 3 kHz input, the envelope pushes fc to the 0.45 fs clamp. The output then alternates sign almost every sample (sample run: -1.01 0.77 -0.36 0.20 0.28 -0.44 0.95 -1.07 1.26 -1.26 …). 97 % of its energy is near Nyquist and its peak is 1.41, against a 0.23 component at the input frequency.
  - At 44.1 kHz the clamp is 19.8 kHz, an audible, full-scale whistle. The hardware's corresponding oscillation is ultrasonic (about 40 kHz, C-PEDA-003).
  - Literature already recorded (C-DSP-002, C-DSP-016) says oversample.
  - The pre-decided remedy in D-030/R-006 ("run the loop twice per sample on an interpolated input … take the second result") is plain decimation by 2 **without an anti-alias filter**. Everything between fs/2 and fs folds back, so it fixes the cutoff range but not aliasing.
- (2) High: easy to reach (two knobs at maximum plus envelope), audible at 44.1 kHz.
- (3) Choose 2x oversampling now, with a proper half-band decimator. If latency must stay 0, use a minimum-phase IIR half-band; T-016 also allows up to 64 samples. Alternatively cap fc at about 0.40 fs and scale k down towards the cap. Add a test: Lowpass, Cutoff 12 kHz, Amount 5, Resonance 10, fs 44.1 kHz; energy above 18 kHz < -40 dB re the in-band output.

### D-033 / D-036 Bucket-brigade delays reduced to a low-pass and a glide
- (1) A BBD stores samples taken at the old clock. A clock (time) change replays all stored content at the new rate, so the echo already in the line is pitch-shifted by d_old/d_new for about one delay time. D-033's fixed-rate buffer with a 30 ms-smoothed read tap gives a short chirp, then normal pitch. The plan's own source says so: C-DSP-033 (Raffel & Smith Sec 3.1) "propose, for echo with clock changes, an interpolating delay line with the input down-sampled and output up-sampled according to the clock".
- Inconsistency: Chorus uses a clock-tracking bandwidth (`200/d`), but Delay uses fixed 3.0/1.7 kHz although its clock ratio within a range is 10:1. By C-DSP-031's relation, the BBD's own band limit at the long end of each range is probably *below* the manual's filter figures (inferred: stage count unknown).
- (2) Medium: knob-sweeping the time is a signature use of this pedal; the owner judges by ear with no reference.
- (3) Implement Delay as a virtual-clock line: write and read at a fractional rate f_clock/f_s with interpolation (Raffel & Smith's structure), keeping the fixed output filters. Add a new test: record a 1 kHz burst at 400 ms, step Time to 200 ms, and the echo emerging after the step has a fundamental of about 2 kHz (D-033 as written gives 1 kHz). Optionally add a clock-tracking low-pass in Delay as in Chorus.

### D-035 polyBLEP with hard sync and FM
- (1) A two-sample polyBLEP corrects the sample before *and* after a discontinuity. A wrap can be predicted from the phase, but a sync reset triggered by the input crossing +0.02 cannot, so "the same residual scaled by the size of the jump" cannot be applied causally without one sample of latency or a minimum-phase (one-sided) BLEP: Brandt's approach, cited only as related work in C-DSP-056 (inferred). The decision does not say which. It also does not say:
  - how the sub-sample crossing position is found (interpolation of the input);
  - how the jump is computed for the triangle/saw/pulse cross-fade;
  - whether the naive triangle gets the correction (it jumps on sync).
- Measured on the oracle (no sync BLEP; `/tmp/r5/drv4.cpp`): synced saw, 1.6 kHz slave on a 311 Hz input, worst inharmonic component below 12 kHz **-32 dB**; free-running polyBLEP saw better than -90 dB.
- D-035 makes the fix a review item with no test. FM rests on C-DSP-061, status **unverified**, load-bearing.
- (2) High: hard sync is a headline feature of the oscillator effect; the specification forces the implementer to guess; nothing catches a weak result.
- (3) Specify: OscBox output delayed by one sample (`latencySamples()` = 1, allowed by T-016 ≤ 64, dry path delayed too), the sync crossing position by linear interpolation, the jump taken from the mixed waveform, and BLEP applied to all shapes. Add a test: synced saw alias < -50 dB re the strongest harmonic at the settings above.

### D-037 Memoryless drive, no anti-aliasing, "frozen by sample-exact tests"
- (1) The text contradicts the tests. T-107 and T-206 allow 1e-3 and T-010 allows 2e-3: not sample-exact. These tests (and T-108) measure only the dry tap (Mix 0) or bypass. Nothing frozen fixes the drive signal that feeds the ladder, ring multiplier, all-passes, delay lines or filter bank. Computing a naive dry tap plus an anti-aliased wet drive (ADAA or 2x oversampled) is allowed by every frozen test I read. The cost: a half-sample offset between dry and wet, a mild cos(pi f / fs) tilt at Mix 0.5, which T-107 (2 kHz, filter contributes nothing) tolerates.
- Measured (oracle, Lowpass Mix 0, drive +28 dB, 0.5 amplitude sine, 48 kHz): 3.11 kHz input gives the strongest alias below the fundamental at -38 dB (1.3 kHz); 4.2 kHz input gives an inharmonic component at -21 dB (18.6 kHz).
- (2) High: drive is common to all seven effects and +28 dB is in range.
- (3) Restate D-037: dry tap naive (frozen); wet-path drive uses first-order ADAA (C-DSP-064), latency compensated or accepted. Add a wet-path alias test (RingMod or Lowpass, Mix 1, drive +28 dB, 3.1 kHz: alias below the fundamental < -60 dB). Correct "sample-exactly" in the text.

### D-040 Listening gate without reference recordings
- (1) Inferred: with no reference, the questions "sounds like the hardware" and L-3 (aliasing) are judged against memory. Synthetic inputs only (plucks, saw bass, noise drums, chord) leave out the real instruments these pedals are used on, which reveal envelope-follower and drive behaviour best.
- Private references are allowed by A-008/A-013. Two sources were not considered: the manufacturer's own demo videos/audio at stated settings, listened to privately, and a trial (if one exists) of the manufacturer's official plug-ins, which would give a settings-matched private A/B. Whether a trial exists was **not verified**; the fetch failed on the rate limit.
- (2) Medium: G-LISTEN can pass while fidelity is poor, or fail without a diagnosis.
- (3) Add owner-recorded dry DI material (guitar/bass/voice; private, not committed) to the demo inputs. Add a private reference-listening step with whatever official demos or trial the owner can get. Keep fixed questions per weak spot: L-3 aliasing (D-030/D-037), BBD time sweep (D-033), sync (D-035).

### D-050 5 % CPU gate on a shared runner
- (1) Flakiness is unlikely: oracle under 1 %, best of three. But the benchmark (`tests/perf/bench.cpp`) only feeds a continuous 0.3 amplitude 220 Hz sine, so it never sees decaying tails. Measured with the oracle, no FTZ, 64-sample blocks, 48 kHz (`/tmp/r5/drv.cpp denorm`), CPU after 60 s of silence that follows signal:

  | Effect | With signal | In silence |
  |---|---|---|
  | FilterBank | 0.71 % | **9.30 %** |
  | Phaser | 0.57 % | 3.31 % |
  | Delay (fb 6) | 1.00 % | 3.00 % |
  | Chorus | 0.81 % | 1.88 % |
  | Lowpass | 0.62 % | 1.12 % |
  | OscBox | 0.36 % | 1.04 % |
  | RingMod | 0.47 % | 0.64 % |

  These are denormals. Rack's engine sets FTZ/DAZ (`include/system.hpp:200`); the VST3 contract does not require it (D-016).
- (2) Medium-High: SC-6 is claimed met while a common real-world case breaks it.
- (3) Add a second benchmark phase (signal, then silence) to the gate. This is a new benchmark mode or tool, since `bench.cpp` is frozen. Require denormal-safe state in the core (flush tiny states or add a -200 dB DC/noise offset as the Lowpass already does) rather than relying on the host's FTZ.

### D-060 Mull with clang-19
- (1) Mull's `CHANGELOG.md` (https://raw.githubusercontent.com/mull-project/mull/main/CHANGELOG.md):
  - 0.38.0 (12 Nov 2026) and 0.37.0 (2 Nov 2026) both say "Cloudsmith bandwidth is exhausted again, use packages from GitHub Releases for installation".
  - 0.36.0 made `mull-runner`/`mull-reporter` LLVM-version-agnostic and deprecated `mull-runner-N`, the name C-PLANNER-012 records.
  - 0.35.0 dropped Ubuntu 22.04 packages.
  - LLVM 19 is still a supported version for `ubuntu:24.04` (`MODULE.bazel` `os_version_mapping`), so clang-19 itself is fine.
- Cost (inferred): Mull runs the whole test executable per mutant. The oracle suite takes 14 s (26 s under sanitizers). A core of 1.5-3k lines gives thousands of mutants, so many CPU-hours against the 6-hour job limit, plus timeouts from mutated loop bounds.
- (2) High: the install step as pinned is likely to fail now, and the runtime is unknown.
- (3) Install the `.deb` from GitHub Releases by URL and SHA-256 in `deps.lock.json`, and use `mull-runner` (unsuffixed). Pin to 0.38.0 or record why 0.37.0. Build a dedicated fast mutation test binary (shorter signals) and shard by source file; set Mull's timeout explicitly. Measure in S-MUTATE.1 before setting the 60 % floor.

### D-061 OSV query by commit
- (1) OSV does index JUCE by git range. The record CVE-2021-23521 (https://osv.dev/vulnerability/CVE-2021-23521) lists a GIT range on `https://github.com/juce-framework/juce`, fixed commits `0abbba3b…` and `2e874e80…`, so a commit query is meaningful for JUCE (verified). It returns empty for 9.0.5 because the fix is an ancestor.
- Two weaknesses:
  - (a) The query does **not** cover code vendored inside JUCE: zlib 1.3.2, libpng 1.6.58, jpeglib, HarfBuzz 15.0.0, FLAC/Ogg/Vorbis (C-LEGAL-009). Their advisories are recorded against their own repositories. The plug-ins link `juce_audio_utils`, which pulls in the graphics and image codecs.
  - (b) Catch2 and the Rack tag most likely have no OSV records, so the result is always empty: harmless but uninformative.
- `api.osv.dev` was unreachable from this sandbox as well (CONNECT 403), so the script has not run anywhere.
- (2) Medium.
- (3) Extend `check_advisories.py` to read `JUCE.spdx.json` from the pinned checkout. For each bundled package, query OSV by the upstream repository's tag commit (or by version where OSV has an ecosystem entry). Fail on any hit for code that is linked. Log "0 records exist for repository X" separately from "0 vulnerabilities", so a vacuous result is visible.

---

## Frozen tests that contradict decision text
1. D-037 says the dry path is compared "sample-exactly"; T-107/T-206 allow 1e-3 and T-010 allows 2e-3 (`tests/core/test_m1.cpp:115-116, 219-220`; `tests/core/test_common.cpp:252-253`). The tests also do not cover the wet-path drive, so D-037's reason for rejecting anti-aliasing does not hold.
2. SC-7 says |output| ≤ 4.0; T-856 checks ≤ 2 x `kOutputLimit` = 8.0 (`tests/vst3/test_vst3.cpp`, T-856). Looser rather than contradictory; T-007 enforces 4.0 in the core.
3. `research/spikes/juce-vst3-build/RESULT.md` (Observations) recommends a shared static JUCE library "(D-005)"; D-005 rejects it.
4. D-020 limits the cutoff to min(40 kHz, 0.45 fs). The oracle limits to 40 kHz in the Lowpass and applies 0.45 fs inside `tanG`; equivalent in effect. No test exercises the clamp at high resonance (see D-030).

## Load-bearing claims with status "unverified"
- C-PEDB-041 (pattern tables unknown): the basis of D-024.
- C-DSP-061 (polyBLEP under FM/sync is the agent's inference): the basis of D-035.
- C-LEGAL-018, C-LEGAL-026 (licence deductions, D-001/D-006): conservative.
- C-LEGAL-025 (Catch2/MIT compatibility): low risk.
- C-LEGAL-031 (MOOGERFOOGER status unknown): D-004.
- C-LEGAL-036 (artefacts on public repos are downloadable): D-009 no-binary rule; conservative.
- C-REF-028 to C-REF-035 (reference search): D-040.
- C-PEDB-045 (MF-105M rate range unknown): D-024 Rate 0.5-32 steps/s.
- Stale: C-PLANNER-012 ("newest Mull tag 0.37.0"; 0.38.0 released 2026-11-12) also names a deprecated binary and an exhausted package host.
- Single-source and load-bearing for a CI gate: C-PLANNER-008 (pluginval success string), C-PLANNER-015 (Rack flags; but I verified it in `compile.mk`), C-PLANNER-018 (runner variance).

## Frozen-interface gaps that force the implementer to guess
1. D-035: causal band-limiting of sync (latency or one-sided BLEP), the sub-sample crossing position, the jump of the mixed waveform, the triangle under sync.
2. D-016: the default bus layout; denormal handling; behaviour of `tap` restored from state as "Down"; tail values too large for the host's integer.
3. D-006: widgets for multi-position selectors and the momentary tap.
4. D-030/R-006: the remedy as written has no decimation filter; if it is invoked, the implementer must invent one.
5. D-018 in a stereo VST3 host: whether `active` = Bypass may pass L/R through (frozen behaviour says it is mono-summed). This needs an owner decision, not an implementer guess.

## Not examined
- Later additions to `plan/PLAN.md`, `GATES.md`, `TRACEABILITY.md`, `HANDOFF.md` and the new "program name 'Default'" contract line (they appeared after this pass started). If the program name is now a frozen VST3 test, check it against `getProgramName(0)` behaviour in JUCE's wrapper; not done here.

## Reproduction of measurements
- Probe drivers: `/tmp/r5/drv.cpp` (modes `ladder`, `drive`, `denorm`), `drv2.cpp`, `drv3.cpp` (ladder at the clamp), `drv4.cpp` (sync aliasing). Built as `g++ -O2 -std=c++17 -Icore/include -Iadapters/include -Iresearch/spikes/oracle research/spikes/oracle/oracle_core.cpp /tmp/r5/<driver>.cpp`, run from the repository root. They are outside the repository and are temporary; re-create them from this description if needed.
- Sources consulted on the web: Mull CHANGELOG and MODULE.bazel (raw.githubusercontent.com, mull-project/mull main); https://osv.dev/vulnerability/CVE-2021-23521; https://osv.dev/blog/posts/introducing-broad-c-c++-support; https://vcvrack.com/manual/VoltageStandards; https://forum.juce.com/t/pluginval-can-set-an-audioparameterbools-to-odd-values/50971?page=2; https://www.endoflife.date/github-actions-runner-images; https://shakfu.github.io/gen-dsp/backends/vcvrack; https://github.blog/changelog/2026-09-23-node-20-is-no-longer-available-in-github-actions (title only, from search results).
