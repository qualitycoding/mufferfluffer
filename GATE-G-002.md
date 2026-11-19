# Gate G-002: publication of built binaries

Step S-040. Run: `gen-20261111T151412Z-mufferfluffer-vst-vcv`, branch `impl-20261111T151412Z-mufferfluffer-vst-vcv`.

## Why this is a gate

The repository is public, so workflow artefacts are downloadable by any signed-in GitHub user (C-LEGAL-036). Uploading plug-in binaries is therefore a publication, which intake excluded by default (A-009). Nothing has been built for upload: `.github/workflows/packages.yml` exists but every job is guarded by `if: startsWith(github.event.head_commit.message, '[build-packages G-002-approved]')`, and no commit carries that marker.

## Evidence

**Commit to be built.** The workflow builds the head commit of the push that carries the approval marker. At the time of writing the branch tip is `3a6c1c6a8928f1f51a397f0d0c2103d598aee5f7`, whose source is identical to what would be built: the approval commit adds only `GATE-G-002.RESPONSE.md`. State of that source:
- the last full CI run on the source is green on every job: seven VST3 builds validated with pluginval on Linux, Windows, macOS arm64 and macOS Intel, four Rack builds, the core suite of 98 cases in eight core jobs (several toolchains, musl, arm64, sanitizers), sanitizers, repository checks and the dependency scan;
- the last full mutation run scored 76.7 % (439 of 572 mutants killed), above its floor of 71 % (`reviews/MUTATION.md`);
- fresh-context code and adversarial reviews are recorded in `reviews/CODE_REVIEW.md` and `reviews/ADVERSARIAL_REVIEW.md`, with every Medium finding fixed.

**The eight archives the workflow would upload**, each an artefact of the run, kept 14 days (`retention-days: 14`), plus a ninth file `SHA256SUMS` with eight lines:

| # | Artefact | Content |
|---|---|---|
| 1 | `mufferfluffer-vst3-linux-x64.zip` | seven `.vst3` bundles and `LICENSE` |
| 2 | `mufferfluffer-vst3-windows-x64.zip` | seven `.vst3` bundles and `LICENSE` |
| 3 | `mufferfluffer-vst3-macos-arm64.zip` | seven `.vst3` bundles and `LICENSE` (minimum macOS version checked, D-071) |
| 4 | `mufferfluffer-vst3-macos-x64.zip` | seven `.vst3` bundles and `LICENSE` (minimum macOS version checked, D-071) |
| 5 | `Mufferfluffer-2.0.0-lin-x64.vcvplugin` (artefact `mufferfluffer-rack-lin-x64`) | Rack 2 plug-in, seven modules |
| 6 | `Mufferfluffer-2.0.0-win-x64.vcvplugin` | Rack 2 plug-in, seven modules |
| 7 | `Mufferfluffer-2.0.0-mac-arm64.vcvplugin` | Rack 2 plug-in, seven modules |
| 8 | `Mufferfluffer-2.0.0-mac-x64.vcvplugin` | Rack 2 plug-in, seven modules |

The binaries are not signed or notarised; a macOS host may ask for the quarantine flag to be cleared.

**Naming and trade-mark position (D-004, R-013).** The brand is "Mufferfluffer" and the effects are "Fluff Lowpass", "Fluff Ring", "Fluff Phase", "Fluff Delay", "Fluff Bank", "Fluff Osc" and "Fluff Swarm". No file outside the planning records and `reviews/` names the hardware manufacturer, its product family or its model codes (T-901). The panels and editors are this project's own design and "VST" appears in no product or company name; documentation carries the sentence on the Steinberg trade mark. The Rack manifest does not carry the "Hardware clone" tag and has no Modular Grid link (D-072). Residual risk R-013 (a trade-mark or trade-dress objection by the manufacturer or its owner) is unchanged by building binaries; the response to a complaint or take-down notice is to stop distribution and leave the decision to you.

**Licence.** The binaries are AGPL-3.0-only works (D-001). Their source is the commit named above, public in this repository; each VST3 archive and each Rack package carries `LICENSE`. JUCE is used under its AGPLv3 option and the VST3 SDK it bundles is MIT-licensed (D-001).

## Question

May the `packages` workflow upload the VST3 bundles and Rack packages of commit `3a6c1c6` as workflow artefacts of the public repository for 14 days, so that you can try them hands-on?

## Allowed responses

- `proceed`: I push the approval-marker commit (subject `[build-packages G-002-approved] owner approved G-002`), which starts the workflow; I record the run URL and checksums here under `## Record`. The listening gate then also asks you the hands-on questions H-1 and H-2.
- `decline`: nothing is uploaded; hands-on trial needs a local build (see `README.md`), and R-012 stays open in `COMPLETION.md`.
- No response: the work continues as if you had declined (listening evidence and everything after it needs no approval); the question stays open until the final step, and an answer arriving before then is applied.

You may answer in this session, or commit `GATE-G-002.RESPONSE.md` to the branch.
