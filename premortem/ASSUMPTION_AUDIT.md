# Assumption & Premise Audit (protocol section 4.7) — mufferfluffer plan

Auditor: fresh-context agent, adversarial stance (every item assumed wrong until checked). Date: 2026-11-13. Branch audited: `gen-20261111T151412Z-mufferfluffer-vst-vcv` at `e26fa00` (clone at `/home/claude/audit`, not modified). Verdict vocabulary and severity rubric: protocol 4.7.3 and Global Rule 4.

## Method and what was executed or fetched

**Read:** `HANDOFF.md`, `plan/ASSUMPTIONS.md`, `plan/DECISIONS.md`, `plan/ENVIRONMENT.md`, `plan/RISKS.md`, relevant parts of `plan/PLAN.md` and `plan/TRACEABILITY.md`, `research/claims.json` (all 371 claims; 286 are `single-source` or `unverified`), `tests/fixtures/effects.json`, `tests/CMakeLists.txt`, `tests/py/test_repo.py`, `tests/vst3/test_vst3.cpp`, the four frozen headers, the oracle (`research/spikes/oracle/`, `oracle_vst3/`).

**Executed (copies under `/tmp/auditwork`, nothing in the clone touched):**
- Built the frozen core suite against the oracle (`cmake ... -DMUFF_IMPL_DIR=.../oracle`, Catch2 from `/home/claude/deps/Catch2`): `ctest -N` gives `Total Tests: 87`; `ctest --no-tests=error` gives `100% tests passed, 0 tests failed out of 87`.
- Catch2 filters exactly as the plan writes them: `"T-001*,T-002*,...,T-024*"` → 8 test cases; `[lowpass]` 10, `[ringmod]` 6, `[phaser]` 8, `[delay]` 11, `[chorus]` 8, `[filterbank]` 11, `[oscbox]` 7; `"[delay],[chorus]"` → 19 (OR semantics). All match `plan/PLAN.md`.
- Rebuilt with Rack's flags (`-O3 -march=nehalem -funsafe-math-optimizations -fno-omit-frame-pointer`): `All tests passed (299647 assertions in 87 test cases)`.
- `muff_bench --limit 5`: exit 0; per-effect load 0.61 % to 1.57 % on this sandbox.
- Numerical probes (`/tmp/auditwork/scripts/dsp_checks.py`): first-order ADAA droop on a linear segment and on `2 tanh(x/2)`; TPT SVF normalised band-pass gain at centre; 6-stage all-pass dip spacing.
- `git ls-remote` on JUCE, Catch2, Rack, pluginval, Mull, the five action pins, `qualitycoding/{mufferfluffer,Lessons,knowledge}`.
- Shallow clones/sparse checkouts: Mull at tags `0.38.0` and `0.37.0` (packaging rules, `docs/Installation.rst`, `ExecutionStatus`), pluginval `v1.0.4` (command line, log strings), CMake `v3.22.1` (`Source/ctest.cxx`), `actions/runner-images` (README, image readmes). JUCE 9.0.5 and Rack v2.6.6 read from `/home/claude/deps`.
- Pluginval's bundled JUCE (`5179f4e`) `juce_UnitTest.cpp` fetched from raw.githubusercontent.com.

**Fetched (WebFetch, after WebSearch; short paraphrases only):** MF-101, MF-102, MF-103 manuals (Full Compass copies) and the MF-101/MF-102 ManualMachine copies; MF-104M (ManualMachine, ManualsLib); MF-105M (Gear4Music copy and ManualsOnline page 7); MF-107 (zZounds copy); MF-108M press text (MatrixSynth), Sound On Sound MF-108S plug-in review, MusicRadar MF-108M review (the MF-108M manual host and ManualMachine copies did not yield the figures); Huovilainen DAFx 2004; Zavalishin *The Art of VA Filter Design* 2.1.0 (three windows); DAFx archive abstracts of Raffel & Smith 2010 and Holters & Parker 2018; VCV manual pages Voltage Standards, Plugin Licensing, Manifest, Panel; GitHub Docs pages on skipping runs, expressions, events, hosted runners, artefact download; OSV `POST /v1/query`; TrademarkDir MURF record; packages.ubuntu.com / ubuntuupdates for jammy `cmake` and `g++-11`; Launchpad noble `clang-19`/`libclang-rt-19-dev` (search listing); community.vcvrack.com toolchain thread and a third-party page showing Rack SDK archive names.

**Not reachable:** `vcvrack.com/downloads` (CONNECT refused), GitHub release assets and REST API (`api.github.com` 403, token invalid), `api.osv.dev` (curl), the MF-108M manual host (allowlist), Justia record pages (site error). curl to every manual host was refused by the proxy, so all manual reads went through the summarising fetch tool (same limitation as R-001).

Row counting: one row per A-###, one per D-###, one per cited weak claim, one per uncited batch, one per frozen-test premise. Verdicts are `confirmed`, `falsified` or `unverifiable`.

---

## Table 1 — Assumptions A-001 to A-017

| id | premise | falsifying evidence | what was checked | verdict | notes |
|---|---|---|---|---|---|
| A-001 | Seven pedals exist with the stated model codes and are documented enough to model. | No manual or spec for a model. | Re-fetched manuals of MF-101, -102, -103, -104M, -105M, -107; MF-108M press text and reviews. | confirmed | |
| A-002 | Behaviour can be taken from manuals and published analyses without manufacturer code. | Key figures only in proprietary material. | Every figure frozen in the fixture that I re-read appears in a public manual copy (see Table 2, D-020..D-026). | confirmed | Pattern grids are graphics (R-003). |
| A-003 | Own names and panel design, no marks in the product. | A product-facing artefact must carry a mark or clone label. | D-004, T-901/T-902 wording; VCV ethics page. | confirmed | The plan's use of the "Hardware clone" tag is a separate issue (D-004, falsified). |
| A-004 | One VST3 per effect and one Rack plug-in with seven modules are buildable. | JUCE or Rack forbids it. | JUCE `juce_add_plugin` per target (docs, spike C-PLANNER-002 verified); Rack manifest `modules[]`. | confirmed | |
| A-005 | The pinned JUCE requires a GPL-family licence for the open route: AGPL-3.0-only. | JUCE 9.0.5 offers another open licence. | `/home/claude/deps/JUCE/LICENSE.md` (AGPLv3 or commercial), `JUCE.spdx.json` (`AGPL-3.0-only OR LicenseRef-JUCE-Commercial`). | confirmed | |
| A-006 | Win x64, macOS arm64, macOS x64, Linux x64 can all be built on hosted runners. | A needed label missing. | GitHub Docs hosted-runner table and `actions/runner-images` README: `windows-2022`, `macos-15`, `macos-15-intel`, `ubuntu-24.04` all present. | confirmed | |
| A-007 | Every hardware CV or expression jack is a CV input in Rack **and an automatable parameter in the VST3**. | A hardware CV jack with no VST3 counterpart. | Fixture vs D-016: Phaser `sweep_in` (hardware SWEEP IN, ±2.5 V control jack, re-fetched MF-103 manual) and FilterBank `step_in` (hardware TAP/STEP gate) have no parameter; D-016 leaves CV/aux inputs unexposed in the VST3. | **falsified** | Medium. See Falsified items. |
| A-008 | Owner owns no pedals to record. | Owner statement. | Not checkable by me. | unverifiable | Covered by D-040 and R-004 (owner may supply recordings). |
| A-009 | No release; binaries only behind G-002. | Plan uploads binaries elsewhere. | D-009 (no binary artefacts in `ci.yml`), D-041, GitHub Docs: signed-in users with read access can download artefacts. | confirmed | |
| A-010 | Mutation testing with Mull on the DSP core is possible on the CI platform. | Mull does not support the chosen OS/LLVM. | Mull 0.38.0 `MODULE.bazel`: `ubuntu:24.04` → LLVM 16-20. | confirmed | Tool exists; the install recipe is wrong (D-060). |
| A-011 | Each effect < 5 % of one core at 48 kHz/64. | Measured load ≥ 5 %. | Ran `muff_bench --limit 5` on the oracle: max 1.57 %. | confirmed | Runner variance is R-009. |
| A-012 | Stores are `qualitycoding/Lessons` and `qualitycoding/knowledge`. | Repos absent. | `git ls-remote` both: present (HEAD = `main`). | confirmed | `Lessons` also has a `master` branch; S-KNOW must push to `main`. |
| A-013 | Hobby project. | — | Owner record. | confirmed | Recorded owner answer; no external fact. |
| A-014 | Profiles `software`+`audio`. | — | `plan/PROFILE.md` consistent. | confirmed | |
| A-015 | Push each phase; remote equals local. | Remote behind. | `git ls-remote origin`: gen branch = `e26fa00` = clone HEAD. | confirmed | |
| A-016 | `main` holds only README and LICENSE. | Other files on `main`. | `git ls-tree origin/main`: `LICENSE`, `README.md`. | confirmed | |
| A-017 | No venue, so no disclosure rule. | — | Profile has no `publication`. | confirmed | |

## Table 2 — Premises of decisions D-001 to D-071

| id | premise(s) | falsifying evidence | what was checked | verdict | notes |
|---|---|---|---|---|---|
| D-001 | JUCE 9.0.5 is AGPLv3/commercial; a JUCE plug-in under the open route is AGPL as a whole; Rack plug-ins free of charge may carry any licence; GPLv3 §13 allows GPL+AGPL combination; Component Library art is CC BY-NC; VST3 SDK MIT; Catch2 BSL. Rejected (b) is legally possible. | Different licence text at the pins. | JUCE `LICENSE.md`, `JUCE.spdx.json`; Rack `LICENSE.md` (exception under §7, CC BY-NC graphics), `LICENSE-GPLv3.txt` §13 lines 552-558; bundled `VST3_SDK/LICENSE.txt` MIT. | confirmed | |
| D-002 | Rack's `plugin.mk` passes `-std=c++11` and a later flag wins; **Rack itself needs C++17**; toolchains accept C++17; nothing needs C++20. | Rack built as C++11; flag order reversed. | Rack v2.6.6: `compile.mk:21` `CXXFLAGS += -std=c++11`, `compile.mk:59` appends `$(FLAGS)` after it (so `-std=c++17` in plugin FLAGS wins); Rack's own `Makefile:79` includes the same `compile.mk` and sets no other standard. | **falsified** | Low. Rack itself compiles as C++11; the flag is in `compile.mk`, not `plugin.mk`. Conclusion (C++17 works) stands. Also C-PLANNER-015. |
| D-003 | Archives exist at `https://vcvrack.com/downloads/Rack-SDK-2.6.6-<lin-x64, win-x64, mac-x64 or mac-arm64>.zip`. | 404 or different names. | Host unreachable. A third-party page lists the same naming for 2.6.1 (`mac-arm64`, `mac-x64`, `lin-x64`, `win-x64`); a VCV community thread reports a 404 for a combined Mac name at 2.6.3 and a fix using per-arch names. | unverifiable | Plan treats as risk: D-003 fallback (job fails, never skipped), R-002, S-030 TOFU. |
| D-004 | Own names; manufacturer marks registered/claimed; manufacturer sells plug-ins; VCV ethics; Steinberg VST rules; **the manifest tag "Hardware clone" is an honest description and appropriate to use**. | VCV defines conditions for that tag. | TrademarkDir: MURF reg. 3043710 active, owner inMusic; VCV Plugin Licensing (ethics) page; bundled VST3 SDK README trademark section; VCV Manual "Manifest" page: the Hardware clone tag is for modules copying a hardware module's functionality, panel and layout, and VCV asks to clone hardware only with the owner's permission. | **falsified** | High: frozen T-902 (`test_repo.py:102`) requires the tag. See Falsified items. |
| D-005 | JUCE 9.0.5 by commit; `juce_add_plugin` options, codes and VST3 categories are valid; one target's modules build in 3 min 05 s. | Invalid category or code; commit mismatch. | `ls-remote` 9.0.5 = `d60b7a7…`; JUCE `docs/CMake API.md` lists `Fx Filter Modulation Delay Generator`; code rules (AU-only constraints). C-PLANNER-002 is a verified spike. | confirmed | MSVC/Apple timings unmeasured (R-008). |
| D-006 | Rack makefile layout; component names; `configSwitch`, `configBypass`, `getVoltageSum`; Rack flags `-O3 -march=nehalem -funsafe-math-optimizations`; core passes 87/87 under them. | Missing API or flag; failures under flags. | Rack headers: all widget names present (`GreenLight` alias `componentlibrary.hpp:115`), `Module.hpp:153,233`, `Port.hpp:99`; `compile.mk:17,30`; re-ran suite with those flags: 87/87. | confirmed | Rack's Mac plug-in floor is `-mmacosx-version-min=10.9` (`compile.mk:41`): C++17 library features such as `std::optional::value` or aligned `new` would not compile for Intel macOS; the oracle uses none. Worth a review item. |
| D-007 | pluginval 1.0.4 takes `--strictness-level`, `--timeout-ms`, `--random-seed 0x…`, `--validate`; log has `SUCCESS`, `Starting tests in:`, `Random seed:`; Steinberg validator skipped without `--vst3validator`. | Options or strings absent. | pluginval `v1.0.4` `CommandLine.cpp:147-165,275-287,554`; `Validator.cpp:206`; `BasicTests.cpp:762`; strings `Random seed: 0x`, `Starting tests in: ` in pluginval's JUCE `5179f4e` `juce_UnitTest.cpp:165,223`. Hex seed accepted (`containsOnly("x-0123456789acbdef")`). | confirmed | R-016 remedy is executable: JUCE SPDX names VST3 SDK 3.8.0, upstream tag `v3.8.0_build_66` exists (the vendored copy itself has no validator sources). |
| D-008 | Catch2 v3.16.1 at `0809213…`; Python stdlib unittest. | Commit mismatch. | `ls-remote`: peeled `v3.16.1^{}` = `08092139…`. | confirmed | |
| D-009 | Labels exist; `windows-2025` carries VS 2026; `ubuntu:22.04` gives GCC 11.4 and CMake 3.22.1; `ubuntu-22.04` label left active support; Rack has no Linux ARM64 SDK. | Label missing; versions differ. | Hosted-runner docs + runner-images README (`windows-latest`/`windows-2025` → `windows-2025-vs2026` image); jammy-updates `cmake 3.22.1-1ubuntu1.22.04.2`, `g++-11 11.4.0-1ubuntu1~22.04.3`. | confirmed | `ubuntu-22.04` is still listed without a deprecation badge; the "left active support" wording rests on GitHub's "latest two versions" policy now that 26.04 is GA. Rack ARM64 SDK absence not checkable (host). |
| D-010 | Layout choice. | — | Internal; consistent with tests' paths. | confirmed | |
| D-011 | Tables generated from fixture; T-001/T-904 enforce. | Hand-written tables pass. | `test_repo.py` T-904 regenerates and compares. | confirmed | |
| D-012 | 1.0 = 5 V; **CV adds `depth × cv` to the knob, "5 V sweeps the whole knob travel, as the hardware manuals state"** (so 0 V leaves the knob where it is, for all seven effects); trigger at 1 V / re-arm 0.1 V; FTZ bits. | A manual stating a different CV law. | Re-fetched MF-105M: with the knob at mid-point a 0 to +5 V signal sweeps the full range; a pedal adds or subtracts half the range around the knob. MF-104M (ManualsLib): Time knob at 12:00 for 0-5 V to sweep longest to shortest. Claims C-PEDC-033/034 (MF-108M): knob centred; Feedback CV from full negative to full positive. VCV Voltage Standards (±5 V audio, triggers 10 V, Schmitt 0.1 V / 1-2 V). MXCSR 0x8040, FPCR bit 24 correct. | **falsified** | Medium. Law matches MF-101/102/103/107 only. See Falsified items. |
| D-013 | Adaptor contracts satisfiable; volts/5. | Oracle fails T-8xx. | Oracle 87/87 incl. T-801..T-821. | confirmed | |
| D-014 | Third tap yields mean; 5 s reset; quarter-note taps → eighth steps on FilterBank. | Manual differs. | Re-fetched MF-105M: three taps; animation runs at twice the tapped rate. C-PEDB-017 corroborated (MF-104M). | confirmed | |
| D-015 | LFO design (own). | — | T-021/T-008 pass on oracle. | confirmed | |
| D-016 | JUCE contract; `ScopedNoDenormals`; threading. | Contract violates JUCE. | JUCE `juce_AudioProcessor.h:121-136` (block size is a strong hint, hosts may exceed); oracle shell processes any length; tests never exceed the prepared size. | confirmed | Interaction with A-007 noted there. |
| D-017 | Smoothing design. | — | Covered by passing tests. | confirmed | |
| D-018 | Drive stays in path in bypass on MF-101/102; MF-103 bypass is a unity buffer unaffected by Drive/Output; MF-105M bypass Left only; Spillover. | Manual differs. | Re-fetched MF-101 ("always active, even…in BYPASS"), MF-103 (unity-gain buffer), MF-105M (dry to LEFT/MONO only); C-PEDB-018 corroborated. | confirmed | |
| D-020 | 15 Hz-12 kHz, ~500 Hz centre, oscillation above 8, ~40 kHz with CV, Amount 5 octaves. | Different figures. | Re-fetched two MF-101 copies: all as stated. | confirmed | |
| D-021 | Lo 0.6-80 Hz, Hi 30 Hz-4 kHz; LFO 0.1-25 Hz; 3 octaves; Carrier In 0.5 V RMS; Carrier Out ±1.25 V; LFO Out ±1.5 V. | Different figures. | Re-fetched two MF-102 copies: all as stated (0.25 × 5 V = 1.25 V; 0.3 × 5 V = 1.5 V; 7.0711 × 0.707 V/5 V = 1.0). | confirmed | |
| D-022 | LFO 0.01-2.5 / 1-250 Hz; 6-octave sweep, mid at 5; LFO Out ±2.8 V; Sweep In ±2.5 V replaces LFO on stages 7-12; Aux inverse; no Mix. | Different figures. | Re-fetched MF-103: all as stated; 1.1 kHz vs "a little more than two octaves above middle C" (1046 Hz) consistent. | confirmed | |
| D-023 | 40-400 / 80-800 ms; 3.0/1.7 kHz; feedback >8 oscillates; Drive span 35 dB; LFO 0.05-50 Hz; six shapes. | Different figures. | Re-fetched ManualMachine MF-104 page and ManualsLib MF-104M: all as stated; 8,192 buckets. | confirmed | |
| D-024 | Mids 200…3400 Hz; Bass LP 110 Hz + 160…1800 Hz; LFO 0.08-20 Hz; odd/even split; patterns are graphics. | Different figures. | Re-fetched Gear4Music copy and ManualsOnline p. 7: all eight + eight values match; LFO 0.08-20 Hz; split and bypass as stated; no Q given. | confirmed | |
| D-025 | 25 Hz-1.6 kHz; 1 V/oct CV ±5 V; Env up to 8 oct upward; wave order; VCA follows envelope; Osc out ±2.5 V; Wave CV 60 %. | Different figures. | Re-fetched zZounds MF-107: all as stated. | confirmed | |
| D-026 | Flange 0.6-10 ms, Chorus 5-50 ms; LFO 0.05-50 Hz; oscillation near 4th mark; R = dry minus wet. | Different figures. | MatrixSynth press text (0.6-10, 5-50 ms, six waves); SOS MF-108S review (0.05-50 Hz; it gives ~0.5-10 ms for the plug-in); MusicRadar (default R output = dry + inverted wet). Manual itself not reachable. | confirmed | 4th-mark figure not re-read (C-R2-038/039). |
| D-027 | Each listed item is undocumented (a design choice). | A manual documents an item. | Spot checks: item 1 (MF-104M gives only 35 dB), item 3 (no times in MF-101), item 8 (MF-105M gives no RATE limits, no Q), item 12 (√(15·12000) = 424 Hz), item 16 (MF-103 "full sweep range"). | confirmed | The list omits the M-series CV offset law (see D-012). |
| D-030 | ZDF ladder self-oscillates at k = 4 at the prewarped cutoff; DC gain 1/(1+k); oversampling recommended in literature (rejected (a)). | Theory differs; T-103 fails. | Zavalishin (fetched window 200k-300k: Eq. for u, k = 4); Huovilainen: oversampling "must be used", 2× in analysis; oracle T-103/T-110 pass. | confirmed | |
| D-031 | Ideal multiplication; carrier ≤ 4 kHz. | — | Identity; D-021 range. | confirmed | |
| D-032 | N equal first-order all-passes; 6-stage dips 1.9 octaves apart vs manual "about two". | Spacing differs. | Computed: tan 15°, 45°, 75° → 1.90 octaves each; manual re-read. | confirmed | |
| D-033 | Clocked line reproduces replay at new speed; options (c) are heavier literature models. | Tests fail. | T-401/T-409/T-701 pass on oracle; DAFx abstracts of Raffel & Smith 2010 and Holters & Parker 2018 confirm what they model. | confirmed | |
| D-034 | Zavalishin TPT SVF formula; `k·bp` unity at centre; stable under sweep. | Formula or gain differ. | Zavalishin Eq. 4.14 and normalised band-pass (fetched); numeric gain 1.0000 at centre. | confirmed | Stability-under-modulation page (C-DSP-029) not reached; T-508 passes. |
| D-035 | Two-sample polyBLEP with one-sample latency meets −55 dB (T-607). | T-607 fails. | Oracle passes T-607. | confirmed | FM use remains an inference (R-007). |
| D-036 | The device's BBD and filters are unknown, so a tracking bandwidth stands in. | Published BBD/stage count. | Re-read reviews give no part numbers; SOS/MusicRadar silent; manual unreachable. | unverifiable | Design choice: D-027 item 10; R-005. |
| D-037 | ADAA would cost **0.6 dB at 10 kHz and 2.6 dB at 20 kHz (48 kHz)** and half a sample; dry-path tests forbid delay. | Different droop. | Computed: on a linear segment first-order ADAA is the two-tap mean, gain cos(πf/fs): −2.01 dB at 10 kHz, −11.74 dB at 20 kHz; simulated with `2 tanh(x/2)` at small amplitude: same. | **falsified** | Low: rejection stands more strongly. |
| D-040 | No redistributable recording or dry/wet pair exists. | A licensed recording found. | Negative-search claims; hosts not searchable here either. | unverifiable | R-004; demo bundle and G-LISTEN. |
| D-041 | Public repo: artefacts are publications. | Artefacts private. | GitHub Docs: signed-in users with read access can download artefacts; public repos grant read access. | confirmed | |
| D-050 | Oracle 0.53-1.9 %; gate 5 % on `core-linux`. | Measured ≥ 5 %. | Re-ran: 0.61-1.57 %. | confirmed | Runner variance unverifiable (R-009). |
| D-060 | Mull for LLVM 19 is installed from **the** `.deb` of a GitHub release, selected as "starts `Mull-19-` and contains `ubuntu-24.04`"; Cloudsmith exhausted; the runner comes with that package. | Asset names or package split differ. | Mull 0.38.0 `docs/Installation.rst` (install **two** files: `Mull-<version>-<os>-<arch>-<os version>` and `Mull-<N>-<version>-LLVM-<N.x.y>-<os>-<arch>-<os version>`, example `…-ubuntu-amd64-24.04.deb`); `mull_package.bzl:384-397` (`mull-19` depends on `mull (= version)`; it ships only the plugin and versioned symlinks); `CHANGELOG.md` 0.36.0-0.38.0. | **falsified** | High. See Falsified items. |
| D-061 | OSV accepts `{"commit": sha}`; `vulns` lists matches. | API differs. | OSV docs `POST /v1/query`. | confirmed | A response may carry only `next_page_token`; the tool should treat that as "not finished", not "none". |
| D-062 | One review per protocol 3.8. | — | Process choice. | confirmed | |
| D-063 | Added tests pass a correct implementation. | Oracle fails one. | Oracle 87/87 (default and Rack flags). | confirmed | Red variants not re-run. |
| D-070 | Rack plug-in major version must be 2. | — | VCV Manifest page. | confirmed | |
| D-071 | JUCE accepts deployment target 11.0; MSVC runtime dynamic by default; Ubuntu 24.04 glibc 2.39. | JUCE refuses 11.0. | JUCE README: macOS 10.11 minimum; `juce_TargetPlatform.h:175`; JUCE sets no `MSVC_RUNTIME_LIBRARY` (CMake default DLL). | confirmed | |

## Table 3 — Single-source and unverified claims

### 3a. Cited by a decision, step, trace row or the fixture (one row each)

| id | premise | falsifying evidence | what was checked | verdict | notes |
|---|---|---|---|---|---|
| C-DSP-002 | Huovilainen: oversampling required; 2× in analysis. | Paper says otherwise. | Fetched P_061. | confirmed | |
| C-DSP-008 | No numeric Vt in Huovilainen. | Value given. | Fetched: cites a textbook, no number. | confirmed | |
| C-DSP-011 | Ladder self-oscillates at the cutoff as k→4. | — | Zavalishin fetched; T-103 passes. | confirmed | |
| C-DSP-018 | DC gain 1/(1+k). | — | Derived from u = (x−kS)/(1+kG) at DC (G = 1). | confirmed | Source (Aalto) not re-fetched. |
| C-DSP-026 | TPT SVF HP formula; 2R·BP unity at cutoff. | — | Zavalishin Eq. 4.14 and 4.15 (fetched); numeric 1.0000. | confirmed | |
| C-DSP-029 | Cutoff modulation cannot destabilise (time-warping). | — | Fetch windows did not reach p. 42. | unverifiable | No R-entry; T-508 (sweep) passes. |
| C-DSP-032 | Raffel-Smith model: filters around BBD. | — | DAFx abstract: low-pass filters around the device to limit aliasing. | confirmed | Component values not re-read. |
| C-DSP-033 | Raffel-Smith on clock variation, gain, distortion. | — | Abstract only; details not reached. | unverifiable | Only used to name a rejected option (D-033 c). |
| C-DSP-034 | Coefficient b illegible. | — | Not reached. | unverifiable | Not used. |
| C-DSP-035 | Holters-Parker: BBD with input/output filters, variable clock. | — | DAFx abstract confirms. | confirmed | |
| C-DSP-036 | Holters-Parker operation costs. | — | Not reached. | unverifiable | Rejected option only. |
| C-DSP-050 | Ideal ring modulation = multiplication. | — | Trigonometric identity. | confirmed | |
| C-DSP-053 | polyBLEP residual method. | — | Paper not reachable; oracle meets T-607. | unverifiable | Mitigated by T-607. |
| C-DSP-055 | Table VIII alias-free limits. | — | Not reachable. | unverifiable | Not load-bearing for tests. |
| C-DSP-061 | polyBLEP valid under audio-rate FM (inference). | — | Inference by nature. | unverifiable | R-007. |
| C-DSP-062 | One-pole follower `a = 1 − exp(−1/(τ fs))`. | — | Step response reaches 1−1/e after τ (derived). | confirmed | |
| C-DSP-064 | ADAA formula and half-sample latency. | — | Two-tap mean on linear segments (derived, simulated). | confirmed | The droop numbers in D-037 are wrong; not part of this claim. |
| C-LEGAL-011 | VST name rules. | — | Bundled `VST3_SDK/README.md:256-292`. | confirmed | |
| C-LEGAL-014 | Rack exception under GPLv3 §7. | — | Rack `LICENSE.md`. | confirmed | |
| C-LEGAL-015 | Free plug-ins may use any licence. | — | Rack `LICENSE.md`. | confirmed | |
| C-LEGAL-017 | Component Library CC BY-NC 4.0. | — | Rack `LICENSE.md`. | confirmed | |
| C-LEGAL-020 | VCV ethics: no copied names, panels, layouts; mandatory for Library. | — | VCV Plugin Licensing page fetched. | confirmed | |
| C-LEGAL-027 | Marks owned by inMusic. | — | TrademarkDir MURF (active, inMusic); Justia pages failed. | confirmed | Others still unchecked (C-LEGAL-031). |
| C-LEGAL-033 | Manufacturer sells plug-in versions. | — | SOS review of the MF-108S plug-in exists. | confirmed | Price not re-checked. |
| C-LEGAL-035 | MF-104Z manual copyright notice. | — | PDF host not allowlisted. | unverifiable | R-003, D-024. |
| C-LEGAL-036 | Artefacts downloadable by signed-in users with read access. | — | GitHub Docs fetched. | confirmed | |
| C-PEDA-001 | 15 Hz / ~500 Hz / 12 kHz; nine octaves. | — | Two MF-101 copies. | confirmed | |
| C-PEDA-003 | Oscillation 15 Hz-12 kHz, ~40 kHz with CV. | — | MF-101 copies. | confirmed | |
| C-PEDA-018 | Envelope times undocumented. | Times given. | Not specifically asked in re-fetch. | unverifiable | D-027 item 3. |
| C-PEDA-023 | Carrier ranges. | — | Two MF-102 copies. | confirmed | |
| C-PEDA-028 | LFO 0.1-25 Hz; 3 octaves. | — | MF-102. | confirmed | |
| C-PEDA-029 | Jack levels/impedances. | — | MF-102. | confirmed | |
| C-PEDA-040 | Triangle LFO, ±2.8 V. | — | MF-103. | confirmed | |
| C-PEDA-041 | Amount → full sweep; 6-octave sweep. | — | MF-103. | confirmed | |
| C-PEDA-045 | No Mix; bypass unity buffer. | — | MF-103. | confirmed | |
| C-PEDA-046 | Sweep In ±2.5 V, 50 kΩ, second six stages. | — | MF-103. | confirmed | |
| C-PEDB-003 | MF-104Z ranges (polynominal page). | — | Not reachable. | unverifiable | Non-load-bearing; other edition. |
| C-PEDB-011 | Feedback >8 self-oscillates. | — | ManualMachine MF-104 page. | confirmed | |
| C-PEDB-020 | Time at 12:00, 0-5 V sweeps whole range. | — | ManualsLib MF-104M. | confirmed | Wording "longest to shortest" may mean the Time CV is inverted relative to the knob; not settled by the text read. |
| C-PEDB-041 | Pattern tables are graphics. | — | Cannot view graphics through the fetch tool. | unverifiable | R-003. |
| C-PEDC-011 | Wave CV covers ~60 %; others 100 %. | — | MF-107 copy. | confirmed | |
| C-PEDC-043 | BBD parts unknown. | — | Absence; manual unreachable. | unverifiable | D-027 item 10. |
| C-PLANNER-007 | pluginval archive hashes. | — | Release assets unreachable. | unverifiable | Download step verifies; a wrong hash stops the job (ENVIRONMENT, S-021). |
| C-PLANNER-008 | pluginval options; exit 0 not proof. | — | pluginval source. | confirmed | |
| C-PLANNER-010 | Runner labels. | — | Docs + runner-images. | confirmed | |
| C-PLANNER-011 | `windows-2025` → VS 2026. | — | runner-images README. | confirmed | |
| C-PLANNER-012 | Mull newest 0.37.0; status 2 = survived. | — | `ls-remote`: 0.38.0 now newest (2026-11-12); `execution_status.rs`: 2 = Passed (survived). | confirmed | True when written; plan already flags it as partly out of date. |
| C-PLANNER-015 | `-std=c++11` then later flag wins; **engine needs C++17**. | — | Rack `compile.mk`, `Makefile`. | **falsified** | Low. Same fact as D-002. |
| C-PLANNER-018 | Runner CPU variance tens of per cent. | — | Not measurable here. | unverifiable | R-009. |
| C-REF-028 | MF-101: no redistributable recording. | A CC recording. | Negative search, hosts unreachable. | unverifiable | R-004. |
| C-REF-029 | MF-102 likewise. | idem | idem | unverifiable | R-004. |
| C-REF-030 | MF-103 likewise. | idem | idem | unverifiable | R-004. |
| C-REF-031 | MF-104/104M likewise. | idem | idem | unverifiable | R-004. |
| C-REF-032 | MF-105/105M likewise. | idem | idem | unverifiable | R-004. |
| C-REF-033 | MF-107 likewise. | idem | idem | unverifiable | R-004. |
| C-REF-034 | MF-108M likewise. | idem | idem | unverifiable | R-004. |
| C-REF-035 | No dry/wet pair anywhere. | idem | idem | unverifiable | R-004. |

### 3b. Uncited claims, batched by source document (229 claims)

| id | premise | falsifying evidence | what was checked (samples) | verdict | notes |
|---|---|---|---|---|---|
| B01 MF-101 manual: C-PEDA-009, -012, -013, -014, -019, -020, -022, C-R2-001, -002, -003, -005, -006 | Figures and statements read from the MF-101 manual. | A copy disagrees. | C-R2-001, -002, -003, -005, -006 against Full Compass and ManualMachine copies. | confirmed | |
| B02 MF-102 manual: C-PEDA-024, -026, -031, -032, -034, -035, -036, C-R2-007..-011 | idem MF-102. | idem | C-R2-007, -008, -009, -010, C-PEDA-024 (sine-like carrier). | confirmed | |
| B03 MF-103 manual: C-PEDA-042, -047, -049, -050, -052, -053, -054, C-R2-012..-016 | idem MF-103. | idem | C-R2-012, -013, -014, -015, -016. | confirmed | |
| B04 MF-104M manual copies: C-PEDB-001, -007, -008, -010, -013, -015, -016, -021, -024, C-R2-017..-022 | idem MF-104M. | idem | C-R2-017, -018, -019, -020, -021 (35 dB), C-PEDB-008 (8,192 buckets), -010 (exact 2×), -015 (0.05-50 Hz). | confirmed | |
| B05 MF-105M manual: C-PEDB-036, -039, -049, -051, -053, -054, C-R2-027, -028, C-R3-002, -006 | idem MF-105M. | idem | C-PEDB-051 (±half range about the knob), C-R2-027, C-R2-028, C-R3-002, C-R3-006. | confirmed | C-PEDB-051 is the evidence for the D-012 finding. |
| B06 MF-107 manual: C-PEDC-006, -010, -012, -013, -016, -018, C-R2-031..-035 | idem MF-107. | idem | C-R2-031..-035, C-PEDC-006. | confirmed | |
| B07 MF-108M manual: C-PEDC-026, -028, -030, -032, -034, -037, -042, C-R2-036..-041 | idem MF-108M. | idem | Manual unreachable; checked against other publishers: C-R2-036 (press text), C-R2-037 (SOS rate, press six waves), C-R2-040 (MusicRadar R output). | confirmed | C-R2-041 (−7/+28 dB) and C-PEDC-034 not re-read. |
| B08 Zavalishin book: C-DSP-012, -013, -014, -015, -016, -025, -030, -063, -068 | Book content. | Text differs. | C-DSP-012 (u formula), -013 (TPT one-pole code), -014 (prewarping). | confirmed | |
| B09 Huovilainen 2004: C-DSP-001, -003..-007, -067 | Paper content. | Text differs. | C-DSP-001 (Euler, five tanh), -003 (half-sample delay, <10 % below Fs/4), -005 (0 < r ≤ 1), -006 (impulse-invariant g), -067 (no licence on first page). | confirmed | |
| B10 Licence and legal texts: C-LEGAL-001..-006, -008..-010, -012, -013, -016, -018, -019, -021..-026, -028..-030, -037 | Pinned licence files, VCV pages, trademark records. | Text differs. | C-LEGAL-001, -002, -005 (splash flag ignored, `juce_gui_basics.cpp:58-64`), -008 (3.8.0 MIT), -016 (VCV recommends GPL-3.0-or-later), -022 (§13), -024, -029. | confirmed | |
| B11 Other DSP papers, datasheets, code: C-DSP-017, -019, -020, -022, -027, -037, -038, -040, -042, -044, -046, -047, -049, -051, -054, -057, -058, -060, -065, -066 | Paper/datasheet content. | Text differs. | Sources not reachable; C-DSP-017 (sec⁴(π/4) = 4), -027 (Simper ≡ Zavalishin algebra), -042 (abstract) checked by derivation or abstract only. | unverifiable | Not load-bearing for frozen values; R-014 for licence. |
| B12 Forums, reviews, wiki about hardware: C-PEDA-015, -016, -017, -033, C-PEDB-004, -005, -009, -023, -034, -040 | Secondary hardware facts. | — | C-PEDB-034 (ratios 1.47-1.55 from re-fetched centres) and -023 (8192/(2·0.04 s) = 102 kHz) by arithmetic; forum posts not reachable. | unverifiable | R-001/R-005. |
| B13 Reference-recording searches: C-REF-001..-006, -009..-027 | Negative or dated search results. | A redistributable recording. | Hosts unreachable from here too. | unverifiable | R-004. |
| B14 "UNKNOWN"/no-source claims (49): C-PEDA-021, -037, -038, -055..-057, C-PEDB-025..-030, -043, -045, -048, -055..-058, C-PEDC-019..-023, -044..-049, C-DSP-021, -023, -024, -028, -039, -043, -048, -052, -059, C-LEGAL-007, -031, -034, -038, C-R2-023..-026, -029, -030 | The sources do not state the item. | A manual states it. | C-PEDB-045 (no RATE limits), -055 (no Q), C-PEDC-021 (no Mix taper), C-R2-023, -024, -025, -026 (now re-read: values as the fixture has them). | confirmed | |

## Table 4 — Premises that frozen tests silently rely on

| id | premise | falsifying evidence | what was checked | verdict | notes |
|---|---|---|---|---|---|
| TP-01 | 1.0 core unit = 5 V (T-812, RackBridge). | Rack audio standard not ±5 V. | VCV Voltage Standards: audio ±5 V, rails ±12 V. | confirmed | |
| TP-02 | Nothing needs hosts to keep blocks ≤ the prepared size (T-852..T-857). | Per-block buffers sized at prepare. | JUCE says hosts may exceed the hint; `BlockRunner` is length-agnostic; tests stay within the prepared size. | confirmed | |
| TP-03 | Catch2 `"T-001*,…"` and `"[tag]"` filters select as stated. | Counts differ. | Ran all filters: counts match PLAN. | confirmed | |
| TP-04 | `ctest --test-dir` and `--no-tests=error` exist from CMake 3.22.1. | Option absent. | `Source/ctest.cxx:120,154` at v3.22.1; 3.28.3 help. | confirmed | |
| TP-05 | `catch_discover_tests` yields 87 ctest tests; summary contains `tests failed out of 87`. | Different count/string. | Ran ctest. | confirmed | |
| TP-06 | `git ls-remote` prints `<sha>\t<ref>` with peeled `^{}` for annotated tags. | Other format. | Output inspected (`od -c`); Catch2 annotated, JUCE and actions lightweight; all pins match. | confirmed | |
| TP-07 | GitHub starts no push workflow for a commit message with `[skip ci]`. | Docs differ. | GitHub Docs "Skip workflow runs". | confirmed | |
| TP-08 | `startsWith(github.event.head_commit.message, …)` usable on push. | Function absent. | GitHub expressions docs. | confirmed | Case-insensitive; harmless for the markers. |
| TP-09 | `workflow_dispatch`/`schedule` only from the default branch. | Docs differ. | Events docs; 60-day disable in public repos. | confirmed | |
| TP-10 | Labels `ubuntu-24.04`, `ubuntu-24.04-arm`, `windows-2022`, `macos-15`, `macos-15-intel` exist (T-905). | Label missing. | Docs and runner-images README. | confirmed | |
| TP-11 | Rack SDK 2.6.6 archives exist at the URL pattern (T-905 hashes). | 404. | Host unreachable; third-party naming for 2.6.1 matches. | unverifiable | D-003, R-002. |
| TP-12 | pluginval 1.0.4 options and pass strings (T-CI-PLUGINVAL). | Absent. | Source at tag. | confirmed | |
| TP-13 | pluginval archive SHA-256 values (T-905 format, D-007). | Mismatch. | Assets unreachable. | unverifiable | Verified at download; job stops on mismatch. |
| TP-14 | A Mull release has **an** asset for Ubuntu 24.04 + LLVM 19 that installs alone and whose name contains `ubuntu-24.04`. | Names/dependencies differ. | Mull 0.38.0 docs and packaging rules. | **falsified** | High. Same item as D-060. |
| TP-15 | Mull `execution_status` 1,3,4,5 killed; 2 survived. | Enumeration differs. | `rust/mull-state/src/execution_status.rs` at 0.38.0. | confirmed | 7 = FailFast also exists; the score script exits non-zero on it (fail-closed). |
| TP-16 | OSV accepts a commit query. | — | OSV docs. | confirmed | Pagination caveat (D-061). |
| TP-17 | Core suite passes under Rack's compiler flags (`rack-linux`). | Failures. | Re-ran: 87/87. | confirmed | |
| TP-18 | Rack tag list in T-902 matches Rack's `tag.cpp`. | Tag missing. | `src/tag.cpp`: all required tags present. | confirmed | |
| TP-19 | Panel rules of T-903 (128.5 mm, 5.08 mm HP, no text). | VCV guide differs. | VCV Panel guide. | confirmed | |
| TP-20 | `ubuntu:22.04` gives GCC 11.4 and CMake 3.22.x. | Other versions. | jammy-updates package pages. | confirmed | |
| TP-21 | JUCE accepts macOS deployment target 11.0 (minos check). | Refusal. | JUCE README/headers (10.11 minimum). | confirmed | |
| TP-22 | Hosted-runner variance stays inside the 5 % gate. | Runs over 5 %. | Not measurable here. | unverifiable | R-009. |
| TP-23 | Trigger 1 V rise / 0.1 V re-arm handles Rack 10 V triggers (T-812). | Standard differs. | VCV Voltage Standards (Schmitt 0.1 V / 1-2 V); Rack `digital.hpp`. | confirmed | |

---

## Falsified items

### F-1 Mull install recipe (rows D-060, TP-14) — High
- **Corrected fact.** Since Mull 0.36.0 the runner and reporter are a separate package `mull` (`Mull-<version>-<os>-<arch>-<os version>.deb`); the per-LLVM package `mull-19` (`Mull-19-<version>-LLVM-<19.x.y>-<os>-<arch>-<os version>.deb`) holds only the compiler plugin and versioned symlinks and declares `Depends: mull (= <version>)`. Mull's own instructions download both files and install them together with `apt-get install ./Mull-*.deb`. The asset names place the architecture between OS and version (documented example `…-ubuntu-amd64-24.04.deb`), so the plan's rule "contains `ubuntu-24.04`" matches nothing; and `dpkg -i` of the `Mull-19` file alone would fail on the unmet dependency.
- **Sources.** Mull tag 0.38.0: `docs/Installation.rst` (section "Install from GitHub Releases"), `mull_package.bzl:56-58, 208-215, 325-332, 384-397`, `CHANGELOG.md` entries 0.36.0-0.38.0; same dependency at tag 0.37.0.
- **Dependents.** D-060; `plan/PLAN.md` S-MUTATE (Tool and pin, sub-step 1), R-010 (its fallback also uses the name rule), `deps.lock.json` key `mull` (one asset, one hash), `.github/workflows/mutation.yml`, `reviews/MUTATION.md`; skill `plan/protocol/skills/mutation-testing-in-ci` as copied. No frozen test (T-905 checks only job ids).
- **Severity.** High (Rule 4: implementer blocked or forced to guess at S-MUTATE.1; R-010 ends in "stop and report").
- **Needed.** Lesson (`kind:wrong-assumption`, trigger: tool packaging split between planning and use) and knowledge item (two-package install, asset naming); amend S-MUTATE/R-010/`deps.lock.json` schema to record both assets and hashes and select by pattern `Mull-<ver>-ubuntu-*-24.04.deb` plus `Mull-19-<ver>-*-ubuntu-*-24.04.deb`; new D-### through 2E.4.

### F-2 "Hardware clone" tag (row D-004; frozen T-902) — High
- **Corrected fact.** The VCV manual's Manifest page defines the "Hardware clone" tag for modules that copy a hardware module's functionality, panel and layout, and asks plug-in authors to clone hardware only with permission from its owner (also `modularGridUrl` is meant for such modules). This project has no permission (C-LEGAL-034) and deliberately does not copy panels; labelling all seven modules as hardware clones contradicts VCV's condition and self-describes the product in the way R-013 tries to avoid.
- **Source.** vcvrack.com/manual/Manifest (fetched 2026-11-13) and its kx.studio mirror; VCV Plugin Ethics Guidelines on Plugin Licensing.
- **Dependents.** D-004 (last bullet); T-902 (`tests/py/test_repo.py:102`, `assertIn("Hardware clone", tags)`); S-032 (`rack/plugin.json`); R-013; SC-4, SC-9.
- **Severity.** High (Rule 4: a frozen test proves invalid — it enforces a requirement whose premise is false). Fix through the amendment route: drop the requirement (and preferably forbid the tag) in T-902, re-freeze.

### F-3 CV law for the M-series pedals (row D-012) — Medium
- **Corrected fact.** The MF-105M manual says a 0 to +5 V CV covers the full knob travel when the knob is at its mid-point; an expression pedal adds or subtracts half the range around the knob. The MF-104M manual (Time) and the MF-108M manual (Time, Feedback: full negative to full positive with the knob centred; claims C-PEDB-020, C-PEDB-051, C-PEDC-033, C-PEDC-034) say the same. D-012 applies the MF-101/102/103/107 law (0 V = knob position, +5 V = knob + full travel) to all seven effects, citing "the hardware manuals". D-027 item 13 owns the difference for Delay Time only; FilterBank and Chorus (and the other Delay CVs) are not in the owner-visible list.
- **Sources.** Gear4Music MF-105M manual copy; ManualsLib MF-104M; claims above.
- **Dependents.** D-012, D-027 (items 13 and missing entries), T-012 (`tests/core/test_common.cpp:286`), behaviour tests that drive CV on Delay/FilterBank/Chorus, S-010 (CV law in the base), S-014, S-015, S-016, S-018; R-005.
- **Severity.** Medium (fidelity of CV response degraded; workaround: offset the CV by −2.5 V in Rack; not applicable to the VST3, which exposes no CV). At minimum add the items to D-027 and the Phase 5 report; or amend D-012 per effect with T-012 re-frozen. The possible inversion of the Time CV direction ("longest to shortest") should be re-read in a manual before deciding.

### F-4 A-007 not met by the VST3 (row A-007) — Medium
- **Corrected fact.** Two hardware control jacks have no VST3 counterpart: Phaser SWEEP IN (±2.5 V control or audio, drives stages 7-12) and FilterBank TAP/STEP step input. D-016 exposes no CV or auxiliary inputs and the fixture has no parameter for them.
- **Source.** Re-fetched MF-103 manual (SWEEP IN); MF-105M manual (tap/step and gate); `tests/fixtures/effects.json`; D-016.
- **Dependents.** A-007, D-016, D-022, D-024, S-020, SC-1 wording, `COMPLETION.md` claims.
- **Severity.** Medium (feature missing in one format; documented workaround: the Rack module). Either record a waiver decision amending A-007 for the VST3, or add a side-chain bus (interface change, halt-class).

### F-5 Rack's own C++ level (rows D-002, C-PLANNER-015) — Low
- **Corrected fact.** Rack v2.6.6 builds itself with `-std=c++11` from `compile.mk` (included by both the Rack `Makefile` and `plugin.mk`); it does not need C++17. The plug-in's `-std=c++17` still wins because `$(FLAGS)` is appended after `-std=c++11`.
- **Source.** `/home/claude/deps/Rack` at `061ccf6`: `compile.mk:21,59`, `Makefile:79`.
- **Dependents.** D-002 rationale text, C-PLANNER-015, knowledge items K-20261009T2342xx named in that claim (should be superseded). No test, no step behaviour changes.
- **Severity.** Low (documentation). Related latent point: Rack's Mac floor 10.9 restricts C++17 library use in the core on Intel macOS (add to S-REVIEW brief).

### F-6 ADAA cost figures (row D-037) — Low
- **Corrected fact.** First-order ADAA acts on small or linear signals as the two-tap mean, gain |cos(πf/fs)|: at 48 kHz −2.0 dB at 10 kHz and −11.7 dB at 20 kHz (not 0.6 dB and 2.6 dB). Simulation with `2 tanh(x/2)` at 0.05 amplitude gives the same.
- **Source.** `/tmp/auditwork/scripts/dsp_checks.py` output.
- **Dependents.** D-037 rationale; R-006 text if it quotes the figure. Decision unchanged (the true cost is larger).
- **Severity.** Low (documentation).

## Unverifiable items and their coverage
A-008 (D-040, R-004); D-003 and TP-11 (D-003 fallback, R-002, S-030); D-036 and C-PEDC-043 (D-027 item 10, R-005); D-040, C-REF-028..035, B13 (R-004); C-PLANNER-007 and TP-13 (hash check at download; add an R- entry: a wrong recorded hash blocks S-021); C-PLANNER-018 and TP-22 (R-009); C-DSP-061 (R-007); C-DSP-029, -053, -055 (mitigated by T-508/T-607; no R- entry — add one or accept); C-DSP-033, -034, -036 (rejected option only; no risk needed); C-PEDA-018 (D-027 item 3); C-PEDB-003 (out-of-scope edition); C-PEDB-041 and C-LEGAL-035 (R-003); B11 (R-014); B12 (R-001, R-005). Per 4.7.5 the ones without an R- entry (C-PLANNER-007/TP-13, C-DSP-029/-053/-055) should get one.

## Other observations (not counted as falsified)
- D-061: an OSV answer that carries only `next_page_token` would currently read as "no vulnerabilities"; treat it as incomplete.
- R-016 remedy: the bundled VST3 SDK has no validator sources; the upstream tag for "3.8.0" is `v3.8.0_build_66` — name it.
- `qualitycoding/Lessons` has both `main` and `master`; S-KNOW should name the branch.
- Mull 0.38.0 is now the newest tag; `mull-runner-19` survives only as a deprecated symlink.

CONFIRMED: 114 FALSIFIED: 8 UNVERIFIABLE: 32

---

## Planner's disposition of the falsified items (added after the audit; the text above is the auditor's report)

| Item | Severity | Lesson | Knowledge item | Amendment |
|---|---|---|---|---|
| F-1 Mull install recipe | High | `L-20261113T093000Z-action-pin-recorded-as-verified-was-not-the-tag-commit` (same class: a tool fact recorded without running it) | `K-20261113T170100Z-mull-two-packages-since-0-36` | D-072 item 2; S-MUTATE, R-010, `plan/ENVIRONMENT.md` |
| F-2 "Hardware clone" tag | High | `L-20261113T170000Z-platform-tag-used-without-reading-its-definition` | `K-20261113T170000Z-rack-hardware-clone-tag-needs-permission` | D-072 item 1; D-004; T-902 amended and re-frozen; S-032 |
| F-3 CV law of three effects | Medium | `L-20261113T170500Z-one-law-assumed-across-a-product-family` | `K-20261113T170200Z-m-series-cv-law-centred-on-2v5` | D-072 item 3; D-012; D-027 item 13; gate question L-8; user guide (S-035) |
| F-4 A-007 in the VST3 | Medium | the same lesson as F-3 (a rule stated for both formats without checking each jack) | none needed (a fact about this plan, not the world) | D-072 item 4; A-007 narrowed; D-027 item 17 |
| F-5 Rack's own C++ level | Low | none (no consequence) | `K-20261113T170400Z-rack-2-6-6-builds-as-cpp11` | D-072 item 5; D-002; C-PLANNER-015 |
| F-6 ADAA cost figures | Low | none (no consequence) | `K-20261113T170300Z-first-order-adaa-droop` | D-072 item 5; D-037 |

Unverifiable items without a risk entry now have one (R-020). The auditor's other observations were applied: the advisory tool follows `next_page_token` (S-034), R-016's remedy names the upstream SDK, S-KNOW names the stores' branch.

Because F-2 changed a frozen test and F-1 to F-4 changed steps and decisions, the cold read was repeated and one pre-mortem round was run on the changed parts (records: `reviews/COLD_READ.md`, `premortem/round-5.md`).
