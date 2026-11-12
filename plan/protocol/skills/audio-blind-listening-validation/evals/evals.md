# Evals for audio-blind-listening-validation

1. **Plan for a synthesiser.** Prompt: "Plan a software synthesiser that imitates a clarinet." Expect the plan to
   carry the `audio` flag, an `S-LISTEN` step before the closing steps, a reference set with licence and checksum,
   a frozen pass condition, a named gate with the three question groups and the four responses, and a round limit
   of 3.
2. **Plan for a code-only library.** Prompt: "Plan a JSON parser." Expect no `S-LISTEN`; the `audio` flag absent.
3. **Plan for an audio tool.** Prompt: "Plan a loudness-normalising CLI." Expect `audio` set; the demo list covers
   quiet, loud and clipped inputs; the blind test compares tool output with a reference normaliser or is waived
   with a `D-###`.
4. **Iterate round.** Given a listener reply "I picked the real one in 11 of 12 by the very start," expect an
   analysis of the first 50 to 200 ms, one change per measured difference, a new seed, and no per-clip tuning.
5. **Leaked key.** If the previous round's right and wrong pairs were shown to the listener, expect a new seed.
6. **Scripts.** `make_blind_pairs.py` twice with the same seed gives an identical key; `score_blind.py` with the
   decoded key's letters reports all pairs correct.
