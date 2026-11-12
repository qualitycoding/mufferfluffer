# Protocol and skills, copied for the implementer

Copies taken from `qualitycoding/skills` at commit `b110439286e18eb41660a7bf92ec1efb0300a834` (Apache-2.0, text in `LICENSE-Apache-2.0.txt`), so that every rule and skill the plan cites can be read on this branch without access to that repository. Do not edit them.

| File here | What the plan uses it for |
|---|---|
| `planning-protocol.md` (v3.15) | "Global Rules" 1 to 14 (Rule 4 is the severity rubric, Rule 14 honest verification); section 3.1 (step template); 3.7.1 and 3.7.2 (S-RETRO, S-KNOW); 3.8 (S-REVIEW, S-ADVERSARIAL, S-MUTATE); 3.10 (listening); Appendix A (lesson template), Appendix B (knowledge-item and source-register templates), Appendix C (store layout, tag taxonomy, schema check) |
| `skills/mutation-testing-in-ci/` | S-MUTATE: `SKILL.md`, `scripts/mull_score.py`, `scripts/mull_include_paths.py`, `references/example-mutation-workflow.yml` |
| `skills/mutation-sharding/` | S-MUTATE: `SKILL.md`, `references/per-file-matrix.yml` |
| `skills/audio-blind-listening-validation/` | S-LISTEN: `SKILL.md` (section 7 is the iteration method); its scripts serve a blind test, which this plan does not have (D-040) |

Where the plan says "protocol 3.8" or "Rule 4" it means the section or rule of that number in `planning-protocol.md`.
