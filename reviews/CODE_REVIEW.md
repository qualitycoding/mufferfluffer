# Code review (S-REVIEW)

One review of the whole implementation diff (`git diff origin/gen-20261111T151412Z-mufferfluffer-vst-vcv...HEAD`) at commit 68850d1, by three fresh-context
Opus 5.5 agents that wrote none of the code, each with one brief. Their reports follow the summary verbatim. Fix commit: 88b0366.
Severity: Rule 4. No Critical or High findings.

## Summary of findings and resolutions

| id | severity | file and line | description | resolution |
|---|---|---|---|---|
| B1-01 | Medium | core/src/delay.cpp:84, core/src/chorus.cpp:88 | `tailSeconds()` of Delay and Chorus under-reports at Output +12 dB | Fixed in 88b0366: the 60 dB bound now starts from the largest first echo (9.0 nepers) |
| B1-02 | Medium | core/src/phaser.cpp:166 | Phaser tail ignores the all-pass group delay at Resonance near 0 | Fixed in 88b0366: the early return now includes twice the group delay |
| B1-03 | Low | delay.cpp, chorus.cpp, filterbank.cpp | Tap-tempo logic and some constants duplicated three times | R-021: accepted for 0.1.0; a shared helper would change three frozen-tested files late in the run; revisit after the listening gate |
| B1-04 | Low | core/src/chorus.cpp:170 | Chorus bypass sends the input to Feedback Send (Delay keeps the wet signal) | R-022: follows the text of D-018 ("every audio output = the input"); left as decided; to be raised at the listening gate |
| B1-05 | Low | core/src/delay.cpp:152 | With Spillover off, echoes keep circulating while bypassed and return on re-engaging | R-023: D-018 is silent on this and the planner's oracle behaves the same; documented in the user guide in 88b0366 |
| B1-06 | Low | docs/USER_GUIDE.md:220 | Guide said Rate is ignored when Step is connected | Fixed in 88b0366 (documentation) |
| B1-07 | Low | delay.cpp, chorus.cpp, filterbank.cpp | Query-named functions advance state | R-024: accepted; each is called once per sample in a fixed order; rename with R-021 |
| B1-08 | Low | core/src/dsp_common.hpp:25,37 | Two unused accessors | Fixed in 88b0366 (removed) |
| B2-01 | Medium | vst3/src/processor.cpp:148, editor.cpp:337 | VST3 Tap is read once per block; taps are lost or quantised at large buffers | R-025: the frozen contract reads parameters once per block; workaround (small buffer or automation) documented in the user guide in 88b0366; a hold-for-one-block editor change is a follow-up that no test can check here (no JUCE headers in the review sandbox) |
| B2-02 | Low | tools/fetch_rack_sdk.py:60 | Symbolic-link check accepted links that resolve outside the folder | Fixed in 88b0366: after unpacking every link must resolve inside the destination; tested with the reproducing archive and a benign one |
| B3-01 | Low | deps.lock.json pluginval | Newest tag is a 2.0.0 beta; 1.0.4 is the newest stable release | No change (pre-release only); recorded in the pin table |
| B3-02 | Low | clocked_line.cpp:61, effect_base.cpp:163, vst3/src/processor.cpp | Six untested paths not in the known-limits list | R-026: accepted; the reviewer exercised the first three with throw-away programs and found no defect; the VST3 paths are reached by pluginval or by no test |

Suite after the fixes: `ctest` 87 of 87 (local, Release); `muff_vst3_tests` 7 of 7 and the platform jobs are read from CI.

---

## Brief 1

Reviewer: Claude Opus 5.5 (claude-opus-5-5), separate fresh-context agent
Commit reviewed: 68850d1

Files read:
- Brief: review/BRIEF_COMMON.md, review/BRIEF1.md
- Plan: src/plan/DECISIONS.md (in full), src/tests/fixtures/effects.json (parameter, port and pattern tables, dumped)
- Frozen headers: src/core/include/muff/core.hpp, src/core/include/muff/blocks.hpp, src/adapters/include/muff/adapters.hpp, src/vst3/include/muff/vst3_shell.hpp
- Core (all of core/src): blocks.cpp, chorus.cpp, clocked_line.cpp/.hpp, create.cpp, delay.cpp, dsp_common.cpp/.hpp, effect_base.cpp/.hpp, filterbank.cpp, generated_tables.inc, lowpass.cpp, oscbox.cpp, phaser.cpp, ringmod.cpp, tables.cpp
- Adaptors and shells: src/adapters/src/adapters.cpp, src/vst3/src/processor.cpp, src/vst3/src/editor.cpp/.hpp, src/vst3/src/entry/entry.cpp, src/vst3/CMakeLists.txt (target table), src/rack/src/module_shell.cpp/.hpp, src/rack/src/plugin.cpp/.hpp, src/rack/src/Delay.cpp, src/rack/plugin.json, src/rack/Makefile
- Docs: src/docs/USER_GUIDE.md (CV, bypass and per-effect sections)
- Tests consulted: src/tests/core/test_common.cpp (T-017), src/tests/core/test_m2.cpp (T-704), src/tests/core/test_m3.cpp (T-505)
- Reference: src/research/spikes/oracle/oracle_core.cpp (Delay and Chorus classes, tail functions)
- review/impl.diff (file list)

### Findings

| id | severity | file and line | description | suggested resolution |
|---|---|---|---|---|
| B1-01 | Medium | core/src/delay.cpp:84-90; core/src/chorus.cpp:88-94 | `tailSeconds()` is meant to be an upper bound on how long the output stays above -60 dB re 1.0 (frozen contract, core.hpp:135-137). For Delay and Chorus it isn't. The formula allows 6.908 nepers (ln 1000), so it assumes the decay starts from 1.0. But the first echo can reach the output at up to Output gain (+12 dB, x3.98) times a wet signal of up to 2. Only `kOutputLimit` = 4 limits it. Measured with a harness linked to `libmuffcore.a` at 48 kHz, input a 200 Hz sine at amplitude 1 then silence: Delay with Output +12, Drive +28, Feedback 7, Long, Time 1, Mix 1 reports **42.29 s** but the main output stays above 1e-3 for **52.01 s**. With Feedback 4: 8.87 s reported, 10.40 s measured. Chorus with Output +12, Drive +28, Feedback 0.75, LFO Amount 0: 5.50 s reported, 6.56 s measured. At Output 0 dB with amplitude 0.5 the bound holds (42.29 vs 40.01), and that is the only case T-017 tests. In a VST3 host, `getTailLengthSeconds()` comes from this value (processor.cpp:166), so a host that stops processing after the reported tail (offline render, or stopping an idle plug-in) cuts off the last 10 s of echoes, which are between -60 and -48 dB. The oracle uses the same formula, so no frozen test catches it. | Use ln(kOutputLimit / 1e-3) = 8.294 nepers in place of 6.908. This bound holds because the output never exceeds 4 and every loop trip multiplies by at most g: the Butterworth low-pass has \|H\| <= 1 and softClip has slope <= 1. Put the helper in one place for both effects (see B1-03). Ask the planner for a T-017 case at Output +12 dB. |
| B1-02 | Medium | core/src/phaser.cpp:166-174 (early return at 168) | When Resonance <= 0.011 (g <= 0.001), the Phaser's `tailSeconds()` returns a fixed 0.1 s. That early return skips the group delay of the all-pass chain, which the function computes only on the other branch. Low stage corners ring for much longer than that. Measured with Sweep 0, LFO Amount 1, LFO Rate 0 (Lo, so the corner sits at the 20 Hz floor), 12 stages, Resonance 0: reported **0.100 s**, measured **0.245 s** (Output 0 dB, input amplitude 0.5) and **0.289 s** (Output +12, Drive +28). The upper-bound contract of core.hpp is broken for an ordinary setting, and a host that honours the tail truncates the low-frequency ringing. With Resonance > 0 the bound holds: at Resonance 10 it reports 25.5 s against 8.7 s measured. | Compute `delay` before the early return and return `kTailBase + 2 * delay` (plus the feedback term only when g > 0.001). Apply the same ln(4000) margin as in B1-01. |
| B1-03 | Low | core/src/delay.cpp:111-125, 133-140; core/src/chorus.cpp:114-132; core/src/filterbank.cpp:110-142, 210-217; also delay.cpp:30-36 vs chorus.cpp:34-40, 84-94 vs 88-94 | The implementer's note is confirmed. D-014 tap tempo is implemented three times. Delay and Chorus each have their own copy of `tapped()` (jack Schmitt edge OR 0-to-1 step of `tap`) and of the release rule (\|knob - knobAtTap\| > 0.01). FilterBank instead has a class (`TappedRate`) with its own copy of the edge logic. The copies have already drifted. FilterBank guards against a zero period (filterbank.cpp:140); Chorus relies on a comment saying the invariant holds (chorus.cpp:130-131). Other duplicates: the Delay and Chorus tail functions and their six tail constants, `kButterworthDamping`/`kButterworthK` (three files), `kPi` (three files), the LCG (lowpass.cpp:53-56 and dsp_common.cpp:62), and two different dB-to-gain formulas in effect_base.cpp (91 uses `dbToGain`, 147 uses `exp`). Failure scenario: D-014 is one rule for three targets, so any amendment (release travel, which tap re-anchors the knob, the run timeout) has to be made three times. A missed copy gives effects that disagree with no test noticing, because the tap tests check each effect separately. | Generalise `TappedRate` into dsp_common: TapTempo, TriggerDetector, last tap step, sample clock and knob-at-tap. It should return "engaged + period", and all three effects should use it. Add a shared `feedbackTailSeconds (g, longestDelay)` helper. Move the shared constants into dsp_common.hpp. |
| B1-04 | Low | core/src/chorus.cpp:170; compare core/src/delay.cpp:160 and the oracle (oracle_core.cpp, Chorus `out[2] = wet`) | The implementer's note on the Chorus bypass, judged. With `active` = 0, the Chorus sends the input to `fb_send`. This follows D-018's text, "Phaser, OscBox, Chorus: every audio output = the input", and `fb_send` is `PortKind::Audio` in the fixture, so it **is not a defect against the decision**. The planner's oracle keeps the wet signal there. The Delay keeps `fb_send` = wet in bypass whatever Spillover is set to, so the two loop sends now behave in opposite ways. Effect of the Chorus choice: with an external insert patched `fb_send` -> effect -> `fb_return`, the bypassed line records x + 1.25·fb·softClip(processed dry input). Nobody hears it, and it causes at most a 50 ms transient on re-engaging. A Rack user who takes `fb_send` as a wet-only output hears the dry signal while bypassed. USER_GUIDE.md:276 mentions only "both outputs" and is silent about Feedback Send. No frozen test checks either way. | Planner to state what D-018 means for the loop sends of both Delay and Chorus. I recommend keeping wet on `fb_send` for both, as the oracle does, because the send is part of the loop, not the signal path. Then make the code and USER_GUIDE.md consistent. Not blocking. |
| B1-05 | Low | core/src/delay.cpp:152, 155-160; docs/USER_GUIDE.md:166 | With Spillover Off, bypass silences the wet signal only at Mix Out and Delay Out. The feedback term `value(feedback)/8 · softClip(returned)` is not scaled by the Active gain, so echoes keep circulating unheard and come back when the effect is switched on again. Measured: Feedback 7.9, Time 0.5, Mix 1, a 50 ms burst, bypass from 0.5 s to 5.5 s, then re-engage with no new input: Mix Out peaks at **0.199** (about -14 dB) from echoes of audio played before the bypass. During the bypass, `fb_send` carries the wet signal (peak 0.43). USER_GUIDE.md:166 says "Delay Out and the wet signal are silent unless Spillover is on". D-018 contrasts Spillover On ("echoes already in the line continue") with Off but does not say what happens to the line when Off. The oracle behaves the same way, so the decision is ambiguous, not the code wrong. | Planner decision. Either (a) with Spillover Off, scale the feedback term by the Active gain so the line drains while bypassed, or (b) keep the behaviour and document it in USER_GUIDE.md (that echoes resume on re-engaging, and that Feedback Send stays live). |
| B1-06 | Low | docs/USER_GUIDE.md:220; core/src/filterbank.cpp:252-254 | The guide says that when Step is connected "the Rate control is ignored". It isn't: the envelope attack and decay are in steps, converted with `samplesPerStep = fs / rate` from the Rate knob (or the tapped rate), not the external step interval. T-505 relies on this ("Rate 32 Hz and Envelope 0: 5-sample attack, 125-sample decay"). Scenario: Rate at 0.5 /s and an 8 Hz step clock with Envelope 2 gives a 1-step decay of 2 s instead of 125 ms, so the pattern smears into a drone, and the guide tells the user the knob is irrelevant. | Documentation fix: "When Step is connected, triggers advance the sequencer; the Rate control still sets the step length used for the Envelope times." |
| B1-07 | Low | core/src/delay.cpp:127-140; core/src/chorus.cpp:122-142; core/src/filterbank.cpp:210-225 | Hidden mutation in functions named like queries. `FluffDelay::delaySeconds` registers taps, can take or release tap control, advances the LFO (through `lfoFactor`) and writes the rate meter. `FluffSwarm::lfoRateHz` registers taps. `FilterBank::stepRate` advances the tap trigger detector and the last-step memory. `frequencyMultiplier` ticks the LFO. Each must be called exactly once per sample in a fixed order. A second call (for example, a later change computing the delay for a display or a tail estimate) would advance the LFO twice and swallow tap edges, with no compile-time hint. delay.cpp:52-53's comment does not say that the function advances the LFO or changes tap state. | Split into mutating steps named as such (`advanceTap`, `advanceLfo`) that run once at the top of `tick`, and pure getters that read the results. |
| B1-08 | Low | core/src/dsp_common.hpp:25, 37 | Dead code: `Smoother::value()` and `EnvelopeFollower::value()` are never called anywhere in core, adapters, vst3 or rack (grep). They add unexercised surface to the mutation-testing scope (D-060). | Remove them, or use them. |

### Checks made with no finding

- **D-004 / (a): rack/plugin.json** has no `modularGridUrl` key and no "Hardware clone" tag (read in full). Module names, slugs and `license` match D-004/D-001.
- **D-002 / (b): no ruled-out C++17 library features in core/src**:
  - grep for `filesystem`, `optional`, `variant`, `visit`, `std::get`, `align`, `any`, `pmr`, `to_chars`/`from_chars`, `launder` found nothing in core/src (only English words in comments).
  - The includes are only `<algorithm> <array> <cmath> <cstddef> <cstdint> <cstring> <iterator> <memory> <stdexcept> <vector> <xmmintrin.h>`.
  - `nm -C libmuffcore.a` shows no `align_val_t`, `filesystem`, `bad_optional_access` or `bad_variant_access` symbols.
  - Every core/src/*.cpp compiles cleanly with `g++ -std=c++17 -fno-aligned-new -Wall -Wextra -Wpedantic -Wshadow -Wconversion` (no warnings).
- **Core test suite** (copy under /tmp/brief1, Release, GCC): 87 of 87 pass.
- **Rack flags** (`-O3 -march=nehalem -funsafe-math-optimizations -fno-omit-frame-pointer`): 87 of 87 pass.
- **D-011**: core/src/generated_tables.inc is byte-identical to `python3 tools/gen_tables.py tests/fixtures/effects.json` output, and core/src/stubs.cpp is deleted.
- **D-012 / D-017 (effect_base)**:
  - Input sanitising and the ±16 limit are present.
  - CvNormalised inputs add depth·cv to the normalised position before smoothing, then clamp. The OscBox Wave depth of 0.6 comes from the table.
  - CvVoltPerOct is applied unsmoothed (oscbox.cpp:192).
  - The Schmitt thresholds are 0.2 / 0.02.
  - FTZ/DAZ is set and the caller's mode restored (MXCSR 0x8040, FPCR bit 24).
  - Outputs are limited to ±4, and a non-finite result triggers a reset with 0 output.
  - Smoothers snap to targets without CV at `reset()`. Stepped parameters switch at once, and `active` cross-fades with the 5 ms one-pole.
- **D-013**: BlockRunner (stereo averaged, mono sums the right output only when `monoSumsRight`, aliasing safe) and RackBridge (V/5, ±12 V limit, mono-sum on an unpatched right output) match the header.
- **D-014**: TapTempo (third tap gives the mean, 5 s run timeout, old period kept) is correct. Targets and limits match: Delay range clamp; Chorus 1/period limited to 0.01-100 Hz; FilterBank 2/period limited to 0.05-200 Hz. Release is at 0.01 of travel, CV included.
- **D-015**: LFO starts at phase 0. RandomStep holds 0 until the first wrap and the generator is reseeded at reset. `lfoShape` handles the wrap and non-finite input.
- **D-016 / D-005**:
  - processBlock sets parameters once per block, uses ScopedNoDenormals, and asks BlockRunner for one channel on a mono output.
  - The tail atomic is set in the constructor, prepareToPlay and processBlock. The editor reads meters only through atomics at 30 Hz.
  - The seven targets' names, codes, bundle IDs and categories match the D-005 table.
- **D-018 per effect**:
  - Lowpass and RingMod: driven dry signal.
  - Phaser: input on both audio outputs.
  - OscBox: input, with `osc_out` running.
  - FilterBank: Left = input, Right = 0.
  - Delay: Mix Out = input; Spillover adds the wet signal scaled by Output and Mix; no new input is recorded.
  - Chorus: see B1-04.
  - CV and LFO outputs keep running.
- **D-020 to D-026**: every law was checked line by line against the decision text and the fixture ranges:
  - cutoff clamp and resonance fade from the raw cutoff;
  - carrier ranges, external carrier gain and monitor gains;
  - phaser corner law, Sweep In on stages 7-12 only in 12-stage mode, Out/Aux = 0.5(u ± y);
  - Delay d = lo·10^n, LFO 2^(lfo·amount/10), loop a·x + (fb/8)·softClip(f), filter cut-offs;
  - FilterBank centres, Bass low-pass band, shift laws, envelope slope rule, first step on the first sample, static pattern 1;
  - OscBox frequency law, wave morph and pulse width, sync threshold, VCA by min(env, 1);
  - Chorus d law, 200/d wet cut-off, 1.25·feedback loop, L/R = dry ± wet.
- **D-030 to D-037**:
  - ZDF ladder with one softClip refinement, noise 1e-5 seeded at reset, follower law;
  - first-order all-pass with tanh feedback;
  - clocked line (read before write, the silent start-up sample, oldest-sample clamp, rebase at 1e6, buffer sizes 0.85 fs + 8 and 0.06 fs + 8);
  - trapezoidal SVF with k·band and the √2 low-pass;
  - polyBLEP one sample late with the sync correction rules;
  - memoryless drive.
- **Tails measured and within bound**: Lowpass (15 Hz, Resonance 7: 3.45 reported / 2.30 measured), FilterBank (0.5 / 0.10), OscBox main output (1.0 / 0.90), RingMod (0.1 / 0).
- **British English**: no American spellings found in core, adapters, vst3, rack, tools, docs or README (word-list scan).


---

## Brief 2

Reviewer: Claude Opus 5.5, separate fresh-context agent

Commit reviewed: 68850d1

Files read:
- BRIEF_COMMON.md, BRIEF2.md
- src/core/include/muff/core.hpp, src/core/include/muff/blocks.hpp, src/adapters/include/muff/adapters.hpp, src/vst3/include/muff/vst3_shell.hpp
- src/core/src/effect_base.hpp, effect_base.cpp, dsp_common.hpp, dsp_common.cpp, clocked_line.hpp, clocked_line.cpp, blocks.cpp, delay.cpp, chorus.cpp, phaser.cpp, filterbank.cpp, oscbox.cpp, lowpass.cpp, ringmod.cpp, tables.cpp, tables.hpp, create.cpp, generated_tables.inc
- src/adapters/src/adapters.cpp
- src/vst3/src/processor.cpp, editor.hpp, editor.cpp, entry/entry.cpp, src/vst3/CMakeLists.txt
- src/rack/src/module_shell.hpp, module_shell.cpp, plugin.hpp, plugin.cpp, Delay.cpp, generated_layout.hpp, src/rack/Makefile
- src/tools/fetch_rack_sdk.py, fetch_pluginval.py, apply_rack_record.py, run_pluginval.py, check_advisories.py, check_rack_package.py
- src/.github/workflows/ci.yml, src/.github/workflows/record-rack-sdk.yml, src/deps.lock.json, src/CMakeLists.txt
- src/plan/DECISIONS.md (D-003, D-014, D-016 and the denormal rule, by search), src/docs/USER_GUIDE.md (tap sections), src/DEVIATIONS.md

| id | severity | file and line | description | suggested resolution |
|---|---|---|---|---|
| B2-01 | Medium | vst3/src/processor.cpp:148-149 with vst3/src/editor.cpp:337-345 | **The VST3 tap control loses taps and quantises the tap interval to the host block length.** `processBlock` samples the momentary `tap` parameter once, before the first sample of each block. The core detects a tap only when this sampled value goes from 0 to 1, and timestamps it at the start of the block. This has two effects. (1) If a click is pressed and released between two `processBlock` calls, both calls read 0, so the tap is lost. A block of 8192 samples lasts 186 ms at 44.1 kHz and 1.02 s at 8 kHz. Ordinary mouse clicks last about 60-150 ms, so at large buffer sizes most taps vanish. (2) Every tap time is rounded to a block boundary, so the tapped period carries an error of up to one block, divided by (taps - 1). The frozen contract (vst3_shell.hpp) causes this, but neither the shell nor the user guide guards against it or mentions it. USER_GUIDE.md lines 163, 210 and 274 describe Tap as an ordinary momentary control. | Inside the frozen contract, make the editor hold `tap` at 1 for at least one block after a press. `prepareToPlay` already receives `maximumExpectedSamplesPerBlock`, so the processor can publish the block duration through an atomic for the editor to read. Also document in USER_GUIDE.md that, in the plug-ins, tap timing is resolved to the host buffer and that small buffers give accurate taps. |
| B2-02 | Low | tools/fetch_rack_sdk.py:60-80 | **The symlink check in `unpack()` accepts links that resolve outside the destination directory.** For a link member, `safe_target(target.parent, link_target)` checks the link target with `Path.resolve()`. That call resolves path components that do not exist yet lexically, and it does not re-check links that were created earlier. Reproduced with a throw-away zip (in /tmp/brief2/t/mkzip.py): first member `a -> b/..` (accepted, because `b` does not exist yet and `b/..` resolves lexically to the staging directory), then member `b -> .` (accepted). After unpacking, `staging/a` resolves to the parent of the staging directory. The same trick inside `sdk/` survives the `shutil.move` into `<dest>/Rack-SDK`. Files cannot be written through such a link, because every file member's path is resolved before it is opened. Normal CI only unpacks hash-verified archives. The exposure is therefore limited to `--record` mode (record-rack-sdk.yml), which unpacks an unverified download. So the stated guarantee is broken, but I found no exploitable write. That is why this is Low and not Critical. | In `--record` mode, do not unpack at all: only the hash and size are needed. Otherwise, reject link targets that are absolute or contain any `..` component, or create links only after all files are written and then check `os.path.realpath` for every link. Simplest of all, reject symlink members, if the four pinned SDK archives contain none. |

Checks made with no finding (all programs written under /tmp/brief2, built against the public headers and a copy of the snapshot's core):
- **Stress test, normal build and ASan/UBSan build (stress.cpp).** Covered all 7 effects at 8 kHz, 44.1 kHz, 192 kHz and 768 kHz, up to 400 000 samples each, with every input connected. Parameters were re-drawn every 4096 samples, about 60 % of them at their exact minimum or maximum. Inputs included NaN, +/-inf, +/-1e30, denormals and +/-16. Results: no non-finite output, no output beyond +/-4, no meter outside [0,1], no `tailSeconds()` value other than <= 60 or exactly 1e9, no sanitiser report, and no `operator new` call inside `process()`. The test counted these calls by replacing the global `operator new`.
- **Denormals (denorm.cpp).** With the caller's MXCSR flush-to-zero and denormals-are-zero bits cleared, I fed an impulse followed by 20 s of silence at high feedback or resonance. Per-segment time stayed flat for all 7 effects, and MXCSR was restored after each call. The AArch64 path sets FPCR bit 24 (FZ), as D-050 requires.
- **ClockedLine rebase (rebase.cpp).** Chorus in flange range at 8 kHz, run for 12 M samples, which crosses the 1e6 phase rebase at about 4.8 M samples. The largest second difference of the output came from start-up (n of about 1000), so there is no click at the rebase, with or without the LFO. The worst-case number of iterations in the read loop per sample is limited by the ratio of the longest to the shortest delay (no more than about 83 for the Chorus range switch). It is not proportional to the buffer size.
- **Sample counters over 24 hours.** All are `std::int64_t`, `std::uint64_t` or `double`: the Delay and Chorus `clock_`, the FilterBank `sampleCount_` and `nextStep_`, and ClockedLine `written_` and `readIndex_`. LFO, carrier, oscillator and sequencer phases are doubles, wrapped every cycle. The uint32 LCGs wrap by design. Nothing overflows at 192 kHz, or even at 768 kHz, in 24 hours. TapTempo time (`clock_ / fs`, about 86 400 s) keeps sub-nanosecond resolution.
- **Guards at the extremes.** Every Log-taper minimum is above 0. `prewarp` limits the cutoff to 0.45 fs, which gives tan(0.45π) of about 6.3 at 8 kHz. The tapped rate `1/period` is reached only after a tap has produced a period, and two taps cannot share a sample, so the period is above 0. `-log(g)` is reached only for 0.001 < g < 1. The OscBox `exp2` of up to 80 octaves stays finite in double precision and is then clamped. The Chorus wet cutoff `200/d` has d >= 0.6 ms. The ladder denominator `1 + kG^4` is >= 1.
- **Block size and aliasing (blocks.cpp).** BlockRunner gave bit-identical results for block size 1 and block size 8192, in place (in == out), at 8 kHz and 768 kHz, including NaN and 1e30 frames.
- **VST3 state parsing.** `setStateInformation` rejects null or empty data, a wrong tag and non-numeric, inf or NaN values (character whitelist plus `isfinite`). It clamps the remaining values and uses JUCE's length-checked `getXmlFromBinary`. Nothing in it throws. The temporary `juce::String` passed to `findParam` lives to the end of the full expression.
- **VST3 thread safety.** Parameters cross threads through JUCE's atomics, and the tail and the meters through `std::atomic`. The editor never touches the effect. The attachments are destroyed before the widgets. `processBlock` neither allocates nor locks.
- **Latent, no failure today.** The processor's `meters_[kMaxMeterLamps = 3]` is indexed by `info.numMeters` without a check. All seven frozen tables have exactly 3 meters.
- **RackBridge and the Rack shell.** No allocation in `process`. Inputs are divided by 5 and then sanitised by the core. Outputs are limited to +/-12 V. The connection flags only change on an edge. `prepare` runs only from `onSampleRateChange`.
- **CI and tooling.** Every workflow has `permissions: contents: read`. No untrusted `${{ }}` expression is expanded inside a `run:` script: the commit message is used only in an `if:`, and `toJSON(needs)` is passed through `env`. Every action is pinned by commit SHA. `fetch_pluginval.py` verifies the SHA-256 against deps.lock.json before `extractall`, after a path check of every member. `fetch_rack_sdk.py` verifies the hash before unpacking in normal mode. Pipelines run under `set -euo pipefail`, so a failed fetch fails its step. The empty-input behaviour of `xargs` is covered by the bundle-count checks that run first.


---

## Brief 3

Reviewer: Claude Opus 5.5, separate fresh-context agent
Commit reviewed: 68850d1

Scope: (a) code paths with no test, and (b) whether the dependency pins are current.

Files read:
- /home/claude/review/BRIEF_COMMON.md, /home/claude/review/BRIEF3.md, /home/claude/review/newest_tags.txt, /home/claude/review/impl.diff (file list)
- src/core/src/: blocks.cpp, chorus.cpp, clocked_line.cpp, clocked_line.hpp, create.cpp, delay.cpp, dsp_common.cpp, dsp_common.hpp, effect_base.cpp, effect_base.hpp, factories.hpp, filterbank.cpp, lowpass.cpp, oscbox.cpp, phaser.cpp, ringmod.cpp, tables.cpp, tables.hpp (generated_tables.inc was only counted: it holds data, not functions)
- src/adapters/src/adapters.cpp
- src/vst3/src/processor.cpp, editor.hpp, editor.cpp, entry/entry.cpp; src/vst3/CMakeLists.txt
- src/rack/src/module_shell.hpp, module_shell.cpp, plugin.hpp, plugin.cpp, Chorus.cpp (and the other six one-line model files), generated_layout.hpp (head); src/rack/Makefile
- src/tests/CMakeLists.txt, tests/vst3/CMakeLists.txt, tests/vst3/test_vst3.cpp, tests/core/test_common.cpp (T-007, T-008, T-017), list of all TEST_CASE names in tests/core/*.cpp, tests/py/test_repo.py
- src/.github/workflows/ci.yml, record-rack-sdk.yml; src/deps.lock.json; src/plan/ENVIRONMENT.md; src/plan/PLAN.md (S-011 to S-017 tail formulas, S-REVIEW); src/plan/TRACEABILITY.md:159; src/plan/RISKS.md (R-010, R-012); src/premortem/ASSUMPTION_AUDIT.md:60; src/tools/check_pins.py, run_pluginval.py, check_rack_package.py (headers)

### Method

- I copied the snapshot to /tmp/brief3/src. I built the core and the adaptors with `-DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage -O0"`, against the offline Catch2. All 87 test cases passed (299 647 assertions).
- **Whole suite, gcov (JSON):** the suite runs 176 of the 177 functions defined in core/src and adapters/src, and 872 of their 880 lines.
- **One test case at a time:** I ran each of the 87 test cases on its own (`muff_core_tests "T-xxx *"`), deleting the .gcda files between runs, so that each function maps to the exact T- ids that run it. All 87 single runs passed. Scripts: /tmp/brief3/scripts/{pertest.sh,funcs.py,lines.py,br.py}. Map: /tmp/brief3/map.txt.
- **"Runs" versus "asserts":** some functions are executed by a test that asserts nothing about their result. I mark these "executed only". The main case is T-011, which calls `tailSeconds()` only to count heap allocations.
- **Throw-away programs for the gaps** (/tmp/brief3/harness, built -O2 against the snapshot sources):
  - `rebase.cpp` drives `ClockedLine` past its phase rebase at 1e6.
  - `tail.cpp` checks every `tailSeconds()` bound against a measured -60 dB decay.
- **vst3/ and rack/:** JUCE and the Rack SDK are not available here, so I mapped these by reading the code and the frozen tests only.

### Findings

| id | severity | file and line | description | suggested resolution |
|---|---|---|---|---|
| B3-01 | Low | deps.lock.json `pluginval.version` "1.0.4"; plan/ENVIRONMENT.md:11 | **Pin currency, pluginval.** The pin is v1.0.4. The newest tag in newest_tags.txt is `v2.0.0-beta3` (442d7de…). The three newest tags listed are all pre-releases (`v2.0.0-beta3`, `-beta2`, `-beta1`). No stable tag newer than v1.0.4 appears in the list the integrator gave, and research/claims.json:9831 records v1.0.4 as the newest stable release. So the pin is the newest stable release, and differs only from a pre-release. | No pin change in this step (as the brief says). Record that only 2.0.0 betas are newer, and look again when a stable 2.x is tagged. |
| B3-02 | Low | plan/TRACEABILITY.md:159 (known limits list); code at core/src/clocked_line.cpp:61-67, core/src/effect_base.cpp:163-166, core/src/phaser.cpp:166, filterbank.cpp:149, oscbox.cpp:154, ringmod.cpp:85, vst3/src/processor.cpp:120-124, 142-145, 154-157, 61-64 | **Untested paths that are not in the accepted "Known limits of the suite".** That list names only the Rack shells (R-012), the VST3 editor (pluginval only), aliasing, delay interpolation and FM band-limiting. These six gaps are not named there:<br>(1) `ClockedLine::rebase()` is the only function that no test ever runs: the phase never passes 1e6 in any test.<br>(2) The recovery path in `EffectBase::process` (reset and write zeros after a non-finite output) never runs. T-007 never drives a state non-finite, so the behaviour that effect_base.hpp and the T-007 comment promise is unverified.<br>(3) `tailSeconds()` of the Phaser, FilterBank, OscBox and RingMod is executed only by T-011, with no assertion on the value. For the Lowpass and the Chorus, T-017 asserts only the endless flag and "< 1e8", not that the value bounds the decay. Only the Delay's bound is measured.<br>(4) The VST3 path that leaves the effect unprepared on an unsupported sample rate, and the silent `! prepared_` branch of `processBlock`, are reached by no frozen test.<br>(5) The `numOut < 1` branch of `processBlock` is reached by no frozen test.<br>(6) `getCurrentProgram`, `setCurrentProgram` and `changeProgramName` are reached by no frozen test.<br>**No defect found behind gaps 1 to 3:** see "Checks behind B3-02" below. A regression in any of these paths would still pass CI. | Add these paths to the known-limits list in TRACEABILITY.md, or add frozen tests through HANDOFF.md 9.4. The most useful tests would be:<br>- a ClockedLine run past the rebase;<br>- a measured tail for the Phaser at resonance 10, sweep 0 and LFO Amount 1;<br>- a VST3 `prepareToPlay(1000.0)` followed by `processBlock`, expecting silence. |

Two counts: no Critical, High or Medium findings, and no pin differs from the newest stable tag. The rest of this report is the two tables (a) and (b) and the checks behind them.

### Checks behind B3-02

- **ClockedLine rebase.** I wrote and read with constant delays of 1, 3, 7.5, 29 and 400 samples, running 1e6×d + 2e5 samples so that each run passes the rebase.
  - The output equals the input d samples earlier, both before and after the rebase. The largest deviation is 0 at d = 1 and 9.9e-6 at d = 400.
  - Those deviations come from double-precision drift in the phase steps before the rebase, not from the rebase itself.
- **Tail bounds**, measured from the end of a 0.3 s tone to the last sample of the main or right output above -60 dB of the tone's peak:

| Effect and settings | Bound returned | Measured decay |
|---|---|---|
| Phaser, resonance 10, sweep 0, Amount 1, 12 stages | 25.5 s | 1.54 s |
| Chorus, feedback ±0.79 | 27.6 s | 19.6 s / 19.8 s |
| Chorus, feedback 0 | 0.15 s | 0.05 s |
| Lowpass, resonance 7.5, cutoff minimum | 6.97 s | 3.22 s |
| FilterBank, sweep 0, 78 Hz | 0.5 s | 0.07 s |
| OscBox | 1.0 s | 0.69 s |
| RingMod | 0.1 s | 0 s |

  Every bound held. (The always-running monitor outputs Carrier Out and Osc Out are left out on purpose: they are not tails.)

### Other checks with no finding

- **Branches never taken that are defensive or unreachable:**
  - blocks.cpp:29 (`p >= 1` guard) and the fall-through after the switch at blocks.cpp:38;
  - tables.cpp:43/58 (`numSteps < 2`) and tables.cpp:70/75 (out-of-range pattern or channel);
  - filterbank.cpp:140 (`period <= 0`: `engaged_` implies a period);
  - adapters.cpp:30 (`out == nullptr`) and adapters.cpp:34/78 (`monoSumsRight` with `rightOutput < 0`, which does not occur in the tables);
  - effect_base.cpp:89/91/93/145/146 (effects without `active` or `drive`: none in the tables);
  - chorus.cpp:91/93 and phaser.cpp:168 (the negligible-gain and over-60 s returns, untested but trivial; covered by the harness above).
- **Platform-specific code.** The aarch64 `ScopedNoDenormals` (effect_base.cpp:35-41) is not compiled on this x86 host. CI runs the core tests on arm64 in the core-linux-arm64 and core-macos jobs.
- **Catch2 pin matches.** The tag ref `v3.16.1` = 5010924c… in newest_tags.txt is the annotated tag object. premortem/ASSUMPTION_AUDIT.md:60 records the peeled `v3.16.1^{}` as 08092139…, the pinned commit. The local build from that source passed.
- **Workflow action pins.** Every `uses:` line in ci.yml and record-rack-sdk.yml carries one of four distinct pins, each equal to the newest release tag (table below).

### (a) Function-to-test table

How to read this table:
- **Coverage source.** For core/ and adapters/, the T- ids come from gcov runs of one test case at a time, so they are measured. For vst3/ and rack/, they come from reading the frozen tests only.
- **Shorthand: "common".** It stands for the tests that construct and run every effect through the base class: T-005, T-007, T-008, T-009, T-010, T-011, T-013, T-015 and T-016 (and T-012/T-014/T-018 where noted), plus the adaptor tests T-801, T-802, T-811, T-812 and T-821.
- **Shorthand: "ctor set".** It means the same tests plus T-004 and T-006, which construct effects without processing.
- **Which tests assert.** Where a function runs under many tests, the ids in **bold** are the ones that assert on its behaviour.

#### core/src/tables.cpp, tables.hpp, create.cpp, factories.hpp

| Function | Tests that run it | Notes |
|---|---|---|
| `findById<ParamSpec/PortSpec/MeterSpec>` (anon., tables.cpp:19) | 80-83 tests; **T-001, T-002** | |
| `clampTo` (anon., :26) | 80 tests; **T-003** | |
| `effectInfo` (:29) | 83 tests; **T-001, T-002** (throw) | |
| `findParam`, `findInput`, `findOutput`, `findMeter` (:35-38) | **T-001, T-002** and 70-83 others | |
| `toNormalised` (:40) | **T-003, T-004**, T-012 and 80 others | `numSteps < 2` branch never taken (no such spec) |
| `fromNormalised` (:54) | **T-003**, T-012 and 77 others | as above |
| `detail::patternLength` (:69) | **T-505**, T-501..T-509 and common | out-of-range branch never taken |
| `detail::patternMask` (:74) | **T-505, T-503**, T-504, T-506, T-509, T-007, T-011, adaptors | out-of-range branch never taken |
| `createEffect` (create.cpp:11) | 80 tests; **T-002** (invalid id throws) | |

#### core/src/blocks.cpp

| Function | Tests that run it | Notes |
|---|---|---|
| `softClip` (:24) | **T-020, T-108** and 77 others | |
| `lfoShape` (:26) | **T-021**, T-203, T-304, T-705 and 52 others | `p >= 1` guard never taken |
| `TapTempo::reset` (:41) | **T-022** and 45 others | |
| `TapTempo::tap` (:48) | **T-022, T-406, T-506, T-706**, T-007, T-011, T-811, T-812 | |
| `TapTempo::periodSeconds` (:65) | same as `tap` | |
| `filterBankEnvelopeTimes` (:67) | **T-023, T-504** and FilterBank tests | |

#### core/src/dsp_common.cpp, dsp_common.hpp

| Function | Tests that run it | Notes |
|---|---|---|
| `onePoleCoefficient` (anon., :19) | 77 tests; **T-014** (smoothing), **T-106** (follower speeds) | |
| `dbToGain` (:25) | 77 tests; **T-108**, **T-307** (Output) | |
| `prewarp` (:27) | 64 tests; **T-101, T-102, T-303, T-404, T-501** | |
| `Smoother::configure`, `Smoother::next`, `Smoother::snap` (hpp:23) | 76-77 tests; **T-014** | |
| `EnvelopeFollower::configure`, `next`, `reset` (hpp:35) | 32-34 tests; **T-106, T-604** | |
| `Lfo::prepare` (hpp:50), `Lfo::reset` (:51), `Lfo::tick` (:57) | 51-59 tests; **T-203, T-304, T-405, T-507, T-705** | |
| `Lfo::phase` (hpp:53) | 34 tests; **T-706** (Rate meter) | |
| `TriggerDetector::rising` (:68), `TriggerDetector::reset` (hpp:64) | 43-44 tests; **T-406, T-505, T-506, T-706** | |
| `StateVariableFilter::tick` (:77), `StateVariableFilter::reset` (hpp:76) | 43-44 tests; **T-404, T-501, T-707** | |

#### core/src/effect_base.cpp, effect_base.hpp

| Function | Tests that run it | Notes |
|---|---|---|
| `clamp` (anon., :24) | 77 tests; **T-007, T-012** | |
| `ScopedNoDenormals` ctor/dtor (:27-46) | 76 tests | Only the x86 branch is compiled here; aarch64 runs in the CI arm64/macOS jobs. No test asserts that the CSR is restored. |
| `EffectBase::EffectBase` (:52) | 79 tests; **T-001**, T-004 | |
| `EffectBase::prepare` (:72) | 77 tests; **T-005** | |
| `EffectBase::reset` (:84) | 77 tests; **T-008** | |
| `EffectBase::setParam` (:97) | 73 tests; **T-004** | |
| `EffectBase::getParam` (:106) | **T-004**, T-011, T-017, T-704 | |
| `EffectBase::setInputConnected` (:110) | 17 tests; **T-013, T-012, T-812** | |
| `EffectBase::getMeter` (:114) | **T-015, T-706, T-812**, T-004, T-007, T-010, T-011 | |
| `paramIndex`, `inputIndex`, `outputIndex` (:118-120) | 69-79 tests (construction) | |
| `EffectBase::connected` (:122) | 51 tests; **T-013, T-205, T-408, T-707** | |
| `EffectBase::value` (:126) | 76 tests | |
| `EffectBase::drive` (:128) | 76 tests; **T-108, T-010** | |
| `EffectBase::trackLevel` (:130) | 76 tests; **T-015** | |
| `EffectBase::setMeter` (:134) | 76 tests; **T-015** | |
| `EffectBase::advanceControls` (:138) | 76 tests; **T-012, T-014** | |
| `EffectBase::process` (:150) | 77 tests; **T-006, T-007** | **GAP:** the non-finite recovery branch (:163-166) is never taken (B3-02) |
| `info` (hpp:23), `sampleRate` (hpp:46), `norm` (hpp:53), `step` (hpp:57), `activeGain` (hpp:59) | 66-79 tests; `activeGain` **T-010, T-407** | |
| `latencySamples` (hpp:31) | **T-016**, T-011 | |

#### core/src/clocked_line.cpp, clocked_line.hpp

| Function | Tests that run it | Notes |
|---|---|---|
| `ClockedLine::size` (:16), `ClockedLine::clear` (:23) | 35 tests (Delay/Chorus ctor set) | |
| `ClockedLine::oldestIndex` (:33) | 34 tests; **T-401, T-409, T-701** | |
| `ClockedLine::read` (:37) | 34 tests; **T-401, T-409, T-701** | |
| `ClockedLine::write` (:50) | 34 tests; **T-401, T-409, T-701** | |
| `ClockedLine::slot` (hpp:35) | 34 tests | |
| `ClockedLine::rebase` (:64) | **none** | **GAP** (B3-02): the phase never passes 1e6 in any test. Harness: correct. |

#### core/src/lowpass.cpp

| Function | Tests that run it | Notes |
|---|---|---|
| `NoiseSource::reset` (:47), `NoiseSource::next` (:53) | T-101..T-110 and common; **T-103, T-008** | |
| `Ladder::reset` (:63), `Ladder::tick` (:70) | **T-101..T-105, T-109, T-110** | |
| `Lowpass::Lowpass` (:109) | ctor set | |
| `Lowpass::tailSeconds` (:115) | **T-017** (endless flag only), T-011 | Bound not asserted; harness: holds |
| `Lowpass::onPrepare` (:123), `Lowpass::onReset` (:128) | **T-005, T-008** and Lowpass tests | |
| `Lowpass::followEnvelope` (:138) | **T-106** | |
| `Lowpass::updateCutoff` (:144) | **T-101, T-106, T-109** | |
| `Lowpass::tick` (:151) | **T-101..T-110, T-018** | |
| `makeLowpass` (:169) | ctor set | |

#### core/src/ringmod.cpp

| Function | Tests that run it | Notes |
|---|---|---|
| `carrierBaseHz` (:33) | **T-202, T-204** | |
| `lfoWaveFor` (:39) | **T-203** | |
| `externalCarrier` (:41) | **T-205**, T-007, T-011, T-811 | |
| `RingMod::RingMod` (:71) | ctor set | |
| `RingMod::tailSeconds` (:85) | T-011 (executed only) | **GAP:** value not asserted; harness: holds |
| `RingMod::onPrepare` (:87), `RingMod::onReset` (:89) | T-201..T-206 and common | |
| `RingMod::advanceOscillator` (:95) | **T-201, T-202, T-204** | |
| `RingMod::tick` (:104) | **T-201..T-206** | |
| `makeRingMod` (:125) | ctor set | |

#### core/src/phaser.cpp

| Function | Tests that run it | Notes |
|---|---|---|
| `AllPassStage::reset` (:43), `tick` (:44), `flushed` (:51) | **T-301, T-302, T-305**, T-014 | |
| `AllPassChain::reset` (:59), `AllPassChain::tick` (:60) | **T-301, T-306** | |
| `stageCoefficient` (:69) | **T-303, T-307, T-308** | |
| `Phaser::Phaser` (:98) | ctor set | |
| `Phaser::onPrepare` (:113), `Phaser::onReset` (:115) | T-301..T-308 and common | |
| `Phaser::twelveStages` (:122) | **T-301, T-306** | |
| `Phaser::lfoValue` (:125) | **T-304** | |
| `Phaser::chainOutput` (:133) | **T-301, T-306, T-307** | |
| `Phaser::tick` (:147) | **T-301..T-308, T-014** | |
| `Phaser::tailSeconds` (:166) | T-011 (executed only) | **GAP:** value never asserted (B3-02); harness: holds |
| `makePhaser` (:178) | ctor set | |

#### core/src/delay.cpp

| Function | Tests that run it | Notes |
|---|---|---|
| `FluffDelay::FluffDelay` (:75) | ctor set | |
| `FluffDelay::tailSeconds` (:84) | **T-017** (bound measured), T-011 | |
| `FluffDelay::onPrepare` (:92), `FluffDelay::onReset` (:99) | T-401..T-411, T-017, T-018 and common | |
| `FluffDelay::tapped` (:111), `FluffDelay::updateTapControl` (:119) | **T-406** | |
| `FluffDelay::lfoFactor` (:127) | **T-405, T-411** | |
| `FluffDelay::delaySeconds` (:133) | **T-401, T-406, T-409** | |
| `FluffDelay::tick` (:142) | **T-401..T-411** | |
| `makeDelay` (:165) | ctor set | |

#### core/src/filterbank.cpp

| Function | Tests that run it | Notes |
|---|---|---|
| `StepEnvelope::reset` (:43) | FilterBank ctor set | |
| `StepEnvelope::trigger` (:44), `StepEnvelope::next` (:59) | **T-504, T-509**, T-503, T-505, T-506 | |
| `StepEnvelope::hold` (:53) | **T-501, T-502, T-507** | |
| `Sequencer::reset` (:77), `Sequencer::clockStep` (:86), `Sequencer::tick` (:98) | **T-503, T-505, T-506** | |
| `Sequencer::inFirstHalfOfStep` (:79) | FilterBank tests and common | No test asserts the Rate meter of the FilterBank (T-015 checks level/active only) |
| `TappedRate::reset` (:124), `TappedRate::update` (:130), `TappedRate::engaged` (:116) | **T-506** | |
| `TappedRate::stepRate` (:138) | **T-506**, T-007, T-011, T-811 | `period <= 0` branch unreachable |
| `FilterBank::tailSeconds` (:149) | T-011 (executed only) | **GAP:** constant 0.5 not asserted; harness: holds |
| `FilterBank::FilterBank` (:181) | ctor set | |
| `FilterBank::onPrepare` (:192), `FilterBank::onReset` (:194) | T-501..T-509 and common | |
| `FilterBank::stepRate` (:210) | **T-503, T-506** | |
| `FilterBank::frequencyMultiplier` (:220) | **T-507, T-508** | |
| `FilterBank::updateCoefficients` (:227) | **T-501, T-507** | |
| `FilterBank::band` (:235) | **T-501, T-502** | |
| `FilterBank::tick` (:240) | **T-501..T-509** | |
| `makeFilterBank` (:274) | ctor set | |

#### core/src/oscbox.cpp

| Function | Tests that run it | Notes |
|---|---|---|
| `shapeFor` (:39), `naiveValue` (:53) | **T-603** | |
| `SyncDetector::reset` (:65), `SyncDetector::next` (:73) | **T-605** | |
| `BandLimitedOscillator::reset` (:91), `next` (:105), `advanceFreely` (:113), `correctPulseEdge` (:137), `addStep` (:144) | **T-601, T-603, T-607** | |
| `BandLimitedOscillator::resetBySync` (:128) | **T-605, T-607**, T-007, T-011 | |
| `OscBox::tailSeconds` (:154) | T-011 (executed only) | **GAP:** constant 1.0 not asserted; harness: holds |
| `OscBox::OscBox` (:172) | ctor set | |
| `OscBox::onPrepare` (:179), `OscBox::onReset` (:184) | T-601..T-607 and common | |
| `OscBox::oscillatorIncrement` (:191) | **T-601, T-602, T-606** | |
| `OscBox::tick` (:199) | **T-601..T-607** | |
| `makeOscBox` (:217) | ctor set | |

#### core/src/chorus.cpp

| Function | Tests that run it | Notes |
|---|---|---|
| `FluffSwarm::FluffSwarm` (:79) | ctor set | |
| `FluffSwarm::tailSeconds` (:88) | **T-017, T-704** (endless flag / "< 1e8" only), T-011 | Bound not asserted; the negligible-gain and over-60 s branches are never taken; harness: holds |
| `FluffSwarm::onPrepare` (:96), `FluffSwarm::onReset` (:101) | T-701..T-708, T-017 and common | |
| `FluffSwarm::tapped` (:114), `FluffSwarm::lfoRateHz` (:122) | **T-706** | |
| `FluffSwarm::delaySeconds` (:134) | **T-701, T-705** | |
| `FluffSwarm::wetFilterG` (:144) | **T-707, T-708** | |
| `FluffSwarm::tick` (:152) | **T-701..T-708** | |
| `makeChorus` (:175) | ctor set | |

#### adapters/src/adapters.cpp

| Function | Tests that run it | Notes |
|---|---|---|
| `limitVolts` (anon., :16) | **T-811, T-812**, T-821 | |
| `BlockRunner::BlockRunner` (:20), `~BlockRunner` (:22), `effect` (:24), `prepare` (:26) | **T-801, T-802**, T-821 | |
| `BlockRunner::process` (:28) | **T-801, T-802**, T-821 | `out == nullptr` and right-output-absent branches never taken |
| `RackBridge::RackBridge` (:53) | **T-811, T-812**, T-802, T-821 | |
| `~RackBridge` (:58), `RackBridge::effect` (:60), `RackBridge::prepare` (:62) | **T-811, T-812**, T-821 | |
| `RackBridge::process` (:64) | **T-811, T-812, T-821** | |

#### vst3/src/processor.cpp (by reading tests/vst3/test_vst3.cpp; not runnable here)

| Function | Frozen tests | Notes |
|---|---|---|
| `hostTailSeconds` (anon., :22) | **T-852** | The infinite branch is asserted only if the random parameters happen to give an endless tail (conditional) |
| `isSupportedSet` (anon., :26) | **T-854, T-857**, T-852 | |
| `parseStateValue` (anon., :31) | **T-853** ("not a number", 1e9) | |
| `Processor::Processor` (:89) | T-851..T-857 | |
| Lambda `fromNormalised` in NormalisableRange (:109) | **T-851** (`convertFrom0to1`) | |
| Lambda `toNormalised` in NormalisableRange (:110) | **T-851** (`convertTo0to1`), **T-853** via `applyStateValue` | |
| `getName` (:46) | **T-851** | |
| `releaseResources` (:49) | T-852 (called, nothing asserted) | |
| `getTailLengthSeconds` (:53) | **T-852** | |
| `acceptsMidi`, `producesMidi` (:54-55) | **T-851** | |
| `hasEditor` (:57) | **T-851** | |
| `getNumPrograms` (:60), `getProgramName` (:63) | **T-851** | |
| `getCurrentProgram` (:61), `setCurrentProgram` (:62), `changeProgramName` (:64) | **GAP** (B3-02) | No frozen test; pluginval in CI may call them |
| `prepareToPlay` (:120) | **T-852** (incl. repeat at 96 kHz), T-854, T-855, T-856, T-857 | **GAP:** the unsupported-rate branch (B3-02) |
| `isBusesLayoutSupported` (:129) | **T-854**, T-852, T-857 | |
| `processBlock` (:137) | **T-852, T-854, T-855, T-856, T-857** | Zero-length block: T-852. **GAP:** `! prepared_` (:142) and `numOut < 1` (:154) branches (B3-02) |
| `createEditor` (:171) | **GAP** | No frozen test creates the editor. Exercised only by pluginval (strictness 10) in the vst3-* CI jobs; accepted at TRACEABILITY.md:159 |
| `getStateInformation` (:175), `setStateInformation` (:186), `stateTag` (:78) | **T-853** | |
| `currentValue` (:200) | **T-853**, T-852 | |
| `applyStateValue` (:205) | **T-853** (clamping to max) | |
| `createProcessor` (:217) | **T-851** (invalid id throws) and every vst3 test | |

#### vst3/src/editor.cpp, editor.hpp

| Function | Frozen tests | Notes |
|---|---|---|
| `parameterAt` (anon., :29), `Editor::Editor` (:47), `~Editor` (:57), `addControl` (:59) | **GAP** | pluginval only (accepted, TRACEABILITY.md:159) |
| Tap attachment callback lambda (:73) | **GAP** | as above |
| Tap button `onStateChange` lambda (:74) | **GAP** | as above |
| `paint` (:98), `lampArea` (:116), `resized` (:121), `timerCallback` (:137) | **GAP** | as above |

#### vst3/src/entry/entry.cpp

| Function | Frozen tests | Notes |
|---|---|---|
| `createPluginFilter` (:11) | **GAP** | Not linked into muff_vst3_tests. Exercised only when pluginval loads the seven bundles in CI |

#### rack/src (no automated test runs any of it; accepted as R-012 and at TRACEABILITY.md:159)

CI only compiles the Rack plug-in and checks the package contents (tools/check_rack_package.py). The tested `RackBridge` (T-811, T-812, T-821) sits underneath, but none of the shell code below is executed. **All rows are GAP.**

| Function | Notes |
|---|---|
| `isTap` (module_shell.cpp:28), `isLevelSlider` (:31), `controlFor` (:33) | |
| `LogTaperQuantity::getDisplayValue` (:44), `LogTaperQuantity::setDisplayValue` (:45) | |
| `unitLabel` (:49), `configParameter` (:51) | |
| `effectValue` (:69) | The log-taper conversion between Rack values and core values is untested; a mistake here would be silent |
| `toPx` (:73), `createControl` (:75) | |
| `configureModule` (:90), `processFrame` (:100), `populateWidget` (:116) | |
| `MuffModule` ctor (module_shell.hpp:28), `MuffModule::process` (:30), `MuffModule::onSampleRateChange` (:33) | |
| `MuffWidget` ctor (:42) | |
| `init` (plugin.cpp:7) | |
| The seven `model<Enum>` initialisers (`Lowpass.cpp:4` … `Chorus.cpp:4`) | |

### (b) Pin table

Sources compared:
- /home/claude/review/newest_tags.txt, gathered by the integrator with `git ls-remote --tags --sort=-v:refname`;
- deps.lock.json;
- plan/ENVIRONMENT.md;
- tests/CMakeLists.txt and vst3/CMakeLists.txt (FetchContent `GIT_TAG`);
- the `uses:` lines of .github/workflows/ci.yml and record-rack-sdk.yml.

| Dependency | Pinned (lock / ENVIRONMENT / build or workflow) | Newest tag found (stable) | Newer pre-release tags | Result |
|---|---|---|---|---|
| JUCE | 9.0.5, d60b7a78…; same in lock, ENVIRONMENT.md:8 and vst3/CMakeLists.txt:7 | 9.0.5 = d60b7a78… | none listed | Current |
| Catch2 | v3.16.1, commit 08092139…; same in lock, ENVIRONMENT.md:9 and tests/CMakeLists.txt:5 | v3.16.1, ref 5010924c… | none listed | Current. 5010924c is the annotated tag object; the peeled `v3.16.1^{}` is 08092139 (premortem/ASSUMPTION_AUDIT.md:60). No difference. |
| pluginval | v1.0.4 (tag commit ed19c2c1…, ENVIRONMENT.md:11); release-archive SHA-256s in the lock | not in the three listed tags; v1.0.4 per research/claims.json:9831 | v2.0.0-beta3 (442d7de…), -beta2, -beta1 | Newest stable as far as the evidence goes; newer tags are pre-releases only. **B3-01 (Low)** |
| Mull | **Not pinned yet.** No `mull` entry in deps.lock.json and no mutation.yml. ENVIRONMENT.md:12 defers the pin to S-MUTATE.1 (later step). | 0.38.0 (b383a52e…) | none listed | Not applicable yet. ENVIRONMENT.md already names 0.38.0 as the newest tag on 2026-11-13, which matches. |
| VCV Rack SDK | 2.6.6 (Rack tag v2.6.6 = 061ccf63…, ENVIRONMENT.md:10); archive SHA-256s in the lock (recorded on first use) | v2.6.6 = 061ccf63… | none listed | Current |
| actions/checkout | v7.0.1 @ 3d3c42e5… (17 uses) | v7.0.1 = 3d3c42e5… | none listed | Current |
| actions/upload-artifact | v7.0.2 @ cf430e03… (16 uses) | v7.0.2 = cf430e03… | none listed | Current |
| actions/cache | v6.1.0 @ 55cc8345… (3 uses) | v6.1.0 = 55cc8345… | none listed | Current |
| msys2/setup-msys2 | v2.33.0 @ ec48f7c5… (1 use) | v2.33.0 = ec48f7c5… | none listed | Current |
| actions/download-artifact | v8.0.2 @ 9000827c… in ENVIRONMENT.md:23; **used by no workflow at this commit** | v8.0.2 = 9000827c… | none listed | ENVIRONMENT pin is current; nothing to compare in the workflows |

None of the pre-release tags in newest_tags.txt is pinned anywhere. The only pre-release tags among the newest are pluginval's 2.0.0 betas. For every other repository, the newest tags listed are stable releases.

