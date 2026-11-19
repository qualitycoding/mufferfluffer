# Mutation testing (S-MUTATE)

Tool and set-up: `.github/workflows/mutation.yml`, `mull.yml`, `tools/mull_shards.py` (scope and shard matrix), `tools/mull_release.py`
(chooses, records and verifies the two Mull release packages), `tools/mull_score.py` (score from the SQLite reports),
`tools/mull_include_paths.py`. Decision D-060; scope `core/src/*.cpp` and `adapters/src/*.cpp`.

## Installed Mull

TBD (filled from the first full run: release tag, the two package names and their SHA-256 values, the runner and plug-in paths).

## Measuring run (sub-step 1)

TBD: run URL, per-shard mutant counts, baseline test times, per-mutant timeouts, job times, scores.

## Failing gate and agreement with Mull (sub-step 2)

TBD: `mull_score.py --floor 99` on a real report (non-zero exit and message); equality of the script's survivor count and score with
Mull's own lines for one shard; a report with one row changed to the not-covered status (exit non-zero).

## Tests omitted from each shard (D-060)

TBD: the output of `muff_core_tests --list-tests "<filter>"` for each shard's filter, and the tests that are therefore omitted.

## Survivors (sub-step 3)

TBD: by file, lowest score first; each survivor killed by a new test or justified by class.

## Recommendation

The owner should mark `mutation-gate` (this workflow) and `aggregate` (`ci.yml`) as required status checks; that is a repository
setting and is not changed from here.
