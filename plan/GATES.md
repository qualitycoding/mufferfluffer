# Human gates

Gates are pre-planned so the implementer never improvises a question. While halted at a gate the implementer may continue any step that does not depend on its outcome, but never an external or irreversible action. Each gate is a file `GATE-<id>.md` committed and pushed; the owner's answer is recorded verbatim in `GATE-<id>.RESPONSE.md`.

G-001 (headline-result gate of the `math`/`computational` profiles) is not applicable to this plan.

## G-002 Publication of built binaries
- Trigger step: S-040.
- Why a gate: the repository is public, so workflow artefacts are downloadable by any signed-in GitHub user (C-LEGAL-036); uploading plug-in binaries is a publication, which intake excluded by default (A-009).
- Evidence bundle in `GATE-G-002.md`: the commit to be built; the list of eight archives the workflow would upload and their 14-day retention; the naming and trade-mark position (D-004) and R-013; a statement that the binaries are AGPL-3.0-only works whose source is that commit.
- Question: "May the `packages` workflow upload the VST3 bundles and Rack packages of commit `<sha>` as workflow artefacts of the public repository for 14 days, so that you can try them hands-on?"
- Allowed responses and branches:
  - `proceed`: dispatch the workflow; record run URL and checksums; S-LISTEN's gate text then also asks the hands-on questions H-1 and H-2.
  - `decline`: no upload; hands-on trial needs a local build (`README.md`); R-012 stays open in `COMPLETION.md`.
  - No response: treated as `decline` for the purpose of continuing; the question stays open until S-FINAL.

## G-LISTEN Listening validation
- Trigger step: S-LISTEN (each round).
- Evidence bundle in `GATE-G-LISTEN.md`: the zip URL and raw URLs of branch `evidence/G-LISTEN`; `INDEX.md` (settings of each demo); the summary line of the behavioural tests on the bundle's commit; the list of design choices D-027; for round 2 and later, what was changed since the previous round.
- Listening instructions: headphones; each demo starts with three seconds of the unprocessed input.
- Questions (fixed):
  - L-1 Do the demos sound like the effects they model, as far as you know the hardware? Which, if any, sound wrong, and how (too clean, too bright, too dull, wrong sweep range, wrong speed, wrong level)?
  - L-2 Lowpass (demos 01 to 03): does the resonance and self-oscillation behave as you expect, and do the Fast and Smooth envelope speeds feel right?
  - L-3 Weak spot, aliasing: in demos 03, 08, 12 and 19 (high resonance, runaway feedback, synced oscillator sweep) do you hear inharmonic whistles, roughness or metallic tones that do not belong?
  - L-4 Weak spot, delay and chorus character (demos 10 to 13, 21, 22): the bucket-brigade circuits are represented only by a low-pass filter and a pitch glide (no companding, no clock noise). Is the repeat tone too clean or otherwise unlike what you expect?
  - L-5 Weak spot, filter bank (demos 14 to 17): the patterns are this project's own, not the hardware's. Are the patterns and envelope shapes acceptable, or do you want the hardware patterns transcribed (R-003)?
  - L-6 Oscillator box (demos 18 to 20): does the oscillator track the playing dynamics and does hard sync sound right?
  - L-7 Levels: is any demo much louder or quieter than its dry section?
  - H-1 (only if G-002 was `proceed` or you built locally) Do all seven plug-ins load in your VST3 host and all seven modules in VCV Rack, and does every control do something audible?
  - H-2 (same condition) In Rack, do the CV inputs sweep their controls over the full range with 0 to 5 V, and do the tap inputs follow a clock?
- Allowed responses and branches:
  - `proceed`: S-LISTEN is done.
  - `iterate: <notes>`: S-LISTEN action 5; at most 3 rounds, then the gate text says so and offers only `proceed-with-rescope`, `stop` or `extend-rounds`.
  - `extend-rounds: <n>`: the owner grants n further iterate rounds (recorded in `reviews/LISTENING.md` with the date); only the owner can extend the cap.
  - `proceed-with-rescope: <text>`: record the text as an accepted shortfall in `COMPLETION.md` and an R- entry; S-LISTEN is done.
  - `stop`: `BLOCKED.md`, then S-RETRO.
  - Named override `accept-own-patterns`: answers L-5 for good; R-003 is closed as accepted.

## G-003 Lessons and knowledge push
- Trigger step: S-KNOW.
- Evidence bundle in `GATE-G-003.md`: the list of lessons and knowledge items to push (id, title, tags), with the source register (`plan/SOURCE_REGISTER.md`) named on its own line together with its number of sources and the number of rows whose licence is `unknown` or `none stated`; the proposed targets: lessons to `qualitycoding/Lessons`, knowledge to `qualitycoding/knowledge` (A-012).
- Question: "Push these lessons and knowledge items to the proposed stores?"
- Allowed responses: `push-to-proposed`; `push-to: <lesson-target> [knowledge-target]`; `do-not-push`.
- Branches: as protocol 3.7.2 (push, retry up to three times per store, bundle fallback). No response: write the bundles and report their paths.

## No other gates
No deployment, release, package publication or external submission is planned (A-009), so no further gate is required.
