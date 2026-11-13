# Human gates

Gates are pre-planned so the implementer never improvises a question. While halted at a gate the implementer may continue any step that does not depend on its outcome, but never an external or irreversible action. Each gate is a file `GATE-<id>.md` committed and pushed; the owner's answer is recorded verbatim under the heading `## Answer` in `GATE-<id>.RESPONSE.md` (or, when it names the hardware, verbatim in `reviews/GATE-<id>.owner-answer.md` with a restatement under `## Answer`: `HANDOFF.md` section 9.2), and facts that follow from it (run URLs, checksums) under `## Record`. How the question reaches the owner, how the answer comes back and what "no response" means: `HANDOFF.md` section 9.2. Gate files are scanned by T-901: they name effects by this project's names and never name the manufacturer, its product family or a model code (cite D-004 and R-013 by id instead).

The protocol's headline-result gate (numbered 001 there, for the `math` and `computational` profiles) does not apply to this plan and is not defined here; the gates of this plan are G-002, G-LISTEN and G-003.

## G-002 Publication of built binaries
- Trigger step: S-040.
- Why a gate: the repository is public, so workflow artefacts are downloadable by any signed-in GitHub user (C-LEGAL-036); uploading plug-in binaries is a publication, which intake excluded by default (A-009).
- Evidence bundle in `GATE-G-002.md`: the commit to be built; the list of eight archives the workflow would upload and their 14-day retention; the naming and trade-mark position (D-004) and R-013; a statement that the binaries are AGPL-3.0-only works whose source is that commit.
- Question: "May the `packages` workflow upload the VST3 bundles and Rack packages of commit `<sha>` as workflow artefacts of the public repository for 14 days, so that you can try them hands-on?"
- Allowed responses and branches:
  - `proceed`: push the approval-marker commit of S-040 action 3, which starts the workflow; record run URL and checksums; S-LISTEN's gate text then also asks the hands-on questions H-1 and H-2.
  - `decline`: no upload; hands-on trial needs a local build (`README.md`); R-012 stays open in `COMPLETION.md`.
  - No response by the time S-MUTATE and S-LISTEN are done: treated as `decline` for the purpose of continuing; the question stays open until S-FINAL, and an answer arriving before then is applied.

## G-LISTEN Listening validation
- Trigger step: S-LISTEN (each round).
- Evidence bundle in `GATE-G-LISTEN.md`: the zip URL and raw URLs of branch `evidence/G-LISTEN`; `INDEX.md` (settings of each demo); the summary line of the behavioural tests on the bundle's commit; the list of design choices D-027; for round 2 and later, what was changed since the previous round.
- Listening instructions: headphones; each demo starts with three seconds of the unprocessed input. You may compare, privately, with any recording or official demonstration of the hardware you have access to; if you have the hardware and can record a dry signal and the same signal through a pedal with the settings noted, say so in your answer and a blind comparison will be added.
- Questions (fixed):
  - L-1 Do the demos sound like the effects they model, as far as you know the hardware? Which, if any, sound wrong, and how (too clean, too bright, too dull, wrong sweep range, wrong speed, wrong level)?
  - L-2 Lowpass (demos 01 to 03): does the resonance and self-oscillation behave as you expect, and do the Fast and Smooth envelope speeds feel right?
  - L-3 Weak spot, aliasing: in demos 03, 08, 12, 19 and 23 (high resonance, runaway feedback, synced oscillator sweep, high drive) do you hear inharmonic whistles, roughness or metallic tones that do not belong?
  - L-4 Weak spot, delay and chorus character (demos 10 to 13, 21, 22): the bucket-brigade circuits are represented by a clocked delay line and a low-pass filter (no companding, no clock noise, no clock-rate sampling). Is the repeat tone too clean or otherwise unlike what you expect?
  - L-5 Weak spot, filter bank (demos 14 to 17): the patterns are this project's own, not the hardware's. Are the patterns and envelope shapes acceptable, or do you want the hardware patterns transcribed (R-003)?
  - L-6 Oscillator box (demos 18 to 20): does the oscillator track the playing dynamics and does hard sync sound right?
  - L-7 Levels: is any demo much louder or quieter than its dry section?
  - H-1 (asked when `GATE-G-002.RESPONSE.md` records `proceed`; otherwise the gate text says the owner may answer it if they built the plug-ins themselves) Do all seven plug-ins load in your VST3 host and all seven modules in VCV Rack, and does every control do something audible?
  - H-2 (same condition) In Rack, with a control's knob at its minimum, does 0 to 5 V on its CV input sweep the control over its full range, and do the tap inputs follow a clock?
  - L-8 Control-voltage law (for information; no listening needed): on the Delay, FilterBank and Chorus the hardware's manuals describe control inputs centred on the knob (2.5 V leaves the setting; 0 V and 5 V move it half the range down and up), while this project uses one law for all seven effects (0 V leaves the setting; 5 V adds the full range). Changing those three is not part of this plan (D-072 item 3). If you want it, say so: the request is recorded in `GATE-G-LISTEN.RESPONSE.md` and in `COMPLETION.md` as follow-up planning work, and it does not affect the answer to this gate.
- Allowed responses and branches:
  - `proceed`: S-LISTEN is done.
  - `iterate: <notes>`: S-LISTEN action 5; at most 3 rounds, then the gate text says so and offers only `proceed-with-rescope`, `stop` or `extend-rounds`.
  - `extend-rounds: <n>`: the owner grants n further iterate rounds (recorded in `reviews/LISTENING.md` with the date); only the owner can extend the cap.
  - `proceed-with-rescope: <text>`: record the text as an accepted shortfall in `COMPLETION.md` and an R- entry; S-LISTEN is done.
  - `stop`: `BLOCKED.md`, then S-RETRO.
  - Named override `accept-own-patterns`: answers L-5 for good; R-003 is closed as accepted.

## G-003 Lessons and knowledge push
- Trigger step: S-KNOW.
- Evidence bundle in `GATE-G-003.md`: the list of lessons and knowledge items to push (id, title, tags), with the source register (knowledge item `K-20261112T130000Z-source-register-mufferfluffer`, generated from `plan/SOURCE_REGISTER.md`) named on its own line together with its number of sources and the number of rows whose licence is `unknown` or `none stated`; the proposed targets: lessons to `qualitycoding/Lessons`, knowledge to `qualitycoding/knowledge` (A-012).
- Question: "Push these lessons and knowledge items to the proposed stores?"
- Allowed responses: `push-to-proposed`; `push-to: <lesson-target> [knowledge-target]`; `do-not-push`.
- Branches: as section 3.7.2 of `plan/protocol/planning-protocol.md` (push, retry up to three times per store, bundle fallback). The two bundles are written when the step halts, so they exist whether or not an answer comes; no response: report their paths (`HANDOFF.md` section 9.2).

## No other gates
No deployment, release, package publication or external submission is planned (A-009), so no further gate is required.
