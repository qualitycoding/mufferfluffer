#!/usr/bin/env python3
"""mull_score.py - mutation score and per-file table from a Mull SQLite report, with a floor.

Mull's own threshold option did not fail a run that also passed the option that tolerates
surviving mutants (observed with mull-runner-19, 2026-11), so the score is computed here from the
report and the exit status is set by this script.

  mull_score.py mull.sqlite --floor 75 [--min-mutants 100] [--survived-status 2] [--strip PREFIX]
  mull_score.py mull.sqlite --floor 75 --baseline per-file.json [--tolerance 5]     (incremental run)
  mull_score.py mull.sqlite --floor 75 --write-baseline per-file.json               (record from a full run)

With --baseline the project-wide floor is not applied: each file in the report must score at least its
recorded score minus --tolerance percentage points (a file missing from the baseline must reach --floor).
An incremental run mutates a few files, and some files legitimately score below the project floor (their
survivors are justified); the per-file baseline stops them from getting worse without failing every change.

Exit status: 0 = score >= floor, 1 = below floor or a post-condition failed (missing or tiny
report), 2 = usage error. Status code 2 means "survived" in the `mutant.execution_status` column
of the Mull SQLite report observed in 2026-11; check with a report that contains a known survivor
before relying on it (the log line "Surviving mutants: N" must equal the count printed here).
"""
import argparse
import collections
import json
import pathlib
import sqlite3
import sys


def score(db_path, survived_status, strip):
    p = pathlib.Path(db_path)
    if not p.is_file() or p.stat().st_size == 0:
        raise SystemExit(f"FAIL: report missing or empty: {db_path}")
    try:
        rows = sqlite3.connect(str(p)).execute(
            "select filename, execution_status from mutant").fetchall()
    except sqlite3.Error as e:
        raise SystemExit(f"FAIL: report unreadable ({e})")
    per = collections.defaultdict(lambda: [0, 0])      # file -> [killed, total]
    for filename, status in rows:
        name = filename.rsplit(strip, 1)[-1] if strip and strip in filename else filename
        per[name][1] += 1
        per[name][0] += 0 if status == survived_status else 1
    killed = sum(k for k, _ in per.values())
    total = sum(t for _, t in per.values())
    return killed, total, dict(per)


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("db")
    ap.add_argument("--floor", type=float, required=True, help="minimum mutation score in percent")
    ap.add_argument("--min-mutants", type=int, default=1, help="post-condition: fewer mutants than this fails")
    ap.add_argument("--survived-status", type=int, default=2)
    ap.add_argument("--strip", default="", help="drop everything up to and including this path fragment")
    ap.add_argument("--baseline", help="per-file baseline JSON: gate each file against its recorded score")
    ap.add_argument("--tolerance", type=float, default=5.0, help="percentage points a file may fall below its baseline")
    ap.add_argument("--write-baseline", help="write the per-file scores of this report to this JSON file")
    a = ap.parse_args(argv)

    killed, total, per = score(a.db, a.survived_status, a.strip)
    if total < a.min_mutants:
        print(f"FAIL: only {total} mutants in the report (minimum {a.min_mutants}); the run did not do its job")
        return 1
    width = max((len(f) for f in per), default=10)
    for f, (k, t) in sorted(per.items(), key=lambda kv: kv[1][0] / kv[1][1]):
        print(f"{f:<{width}}  {k:>4}/{t:<4} {100.0 * k / t:5.1f}%")
    pct = 100.0 * killed / total
    if a.write_baseline:
        pathlib.Path(a.write_baseline).write_text(json.dumps(
            {f: round(100.0 * k / t, 1) for f, (k, t) in sorted(per.items())}, indent=2) + "\n")
        print(f"baseline written: {a.write_baseline}")
    if a.baseline:
        base = json.loads(pathlib.Path(a.baseline).read_text())
        bad = []
        for f, (k, t) in sorted(per.items()):
            need = base[f] - a.tolerance if f in base else a.floor
            if 100.0 * k / t < need:
                bad.append(f"{f}: {100.0 * k / t:.1f}% < required {need:.1f}%")
        print(f"Mutation score (incremental): {pct:.1f}% ({killed}/{total} killed); per-file baseline, tolerance {a.tolerance:g} points")
        print("\n".join("FAIL: " + b for b in bad) or "all files at or above their baseline")
        return 1 if bad else 0
    print(f"Mutation score: {pct:.1f}% ({killed}/{total} killed); floor {a.floor:g}%")
    return 0 if pct >= a.floor else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
