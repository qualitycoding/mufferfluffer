#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# Derived from skills/mutation-testing-in-ci/scripts/mull_score.py of qualitycoding/skills at commit
# b110439286e18eb41660a7bf92ec1efb0300a834 (Apache-2.0; text in plan/protocol/LICENSE-Apache-2.0.txt).
# Changes made here (plan step S-MUTATE): the score rule (only Mull's killed statuses count as killed, any status
# that is neither killed nor survived fails), several reports at once, --plan, --compare-log, --floor-file, and a
# distinct exit status for a missing baseline.
"""mull_score.py - mutation score and per-file table from Mull SQLite reports, with a floor.

Mull's own threshold option did not fail a run that also passed the option that tolerates surviving mutants
(observed with mull-runner-19, 2026-11), so the score is computed here from the reports and the exit status is set
by this script.

  mull_score.py mull-phaser.sqlite --min-mutants 5 --compare-log mull-phaser.log           (one shard)
  mull_score.py reports/mull-*.sqlite --plan plan.json --floor-file mutation-floor.json    (full run)
  mull_score.py reports/mull-*.sqlite --plan plan.json --floor-file mutation-floor.json \\
                --baseline mutation-baseline.json --tolerance 2                           (incremental run)
  mull_score.py reports/mull-*.sqlite --write-baseline new-baseline.json                   (record from a full run)

Score rule (plan S-MUTATE, D-060): a mutant is killed only if its `execution_status` is one Mull itself counts as
killed: 1 failed, 3 timed out, 4 crashed, 5 abnormal exit. 2 is survived and counts against the score. Any other
status (0 invalid, 6 dry run, 8 not covered, or a value unknown here) makes the script fail: a report holding such
rows was not produced the way the plan pins. The numbers are Mull's `ExecutionStatus` enumeration as read for the
pinned release; sub-step 1 confirms them against the installed release's source.

With --baseline the project floor is not applied to the total: each file in the reports must score at least its
recorded score minus --tolerance points, and a file missing from the baseline must reach the floor. A baseline
with no per-file entries (keys starting with "_" are comments) is "baseline missing", which is not the same
failure as a score below the baseline and has its own exit status.

--plan names the expected shards (the JSON written by tools/mull_shards.py): each report must be called
mull-<shard>.sqlite, every planned shard must have exactly one report, a report may only hold mutants of its
shard's files, and no file may appear in two reports. --min-mutants applies to each report separately.

--compare-log (one report only) reads Mull's console output and fails if the score Mull printed differs from this
script's score by more than 0.5 points, or if Mull's count of surviving mutants differs from this script's.

Exit status: 0 = pass; 1 = below the floor or baseline, or a post-condition failed (missing, empty or tiny report,
unexpected status, incomplete or inconsistent shard set, log disagreement); 2 = usage error; 3 = baseline missing.
"""
import argparse
import collections
import json
import pathlib
import posixpath
import re
import sqlite3
import sys

KILLED = {1: "failed", 3: "timed out", 4: "crashed", 5: "abnormal exit"}
SURVIVED = 2
OTHER_NAMES = {0: "invalid", 6: "dry run", 7: "fail fast", 8: "not covered"}
LOG_TOLERANCE = 0.5
EXIT_BASELINE_MISSING = 3


class Fail(Exception):
    def __init__(self, message, status=1):
        super().__init__(message)
        self.status = status


def normalise(name, strips):
    for s in strips:
        if s and s in name:
            name = name.rsplit(s, 1)[-1]
    return posixpath.normpath(name)


def read_report(db_path, strips):
    """Returns {file: [killed, survived]} for one report; fails on a missing report or a status outside the rule."""
    p = pathlib.Path(db_path)
    if not p.is_file() or p.stat().st_size == 0:
        raise Fail(f"report missing or empty: {db_path}")
    try:
        con = sqlite3.connect(f"file:{p}?mode=ro", uri=True)
        cols = {r[1] for r in con.execute("pragma table_info(mutant)")}
        if not {"filename", "execution_status"} <= cols:
            raise Fail(f"{db_path}: no mutant table with filename and execution_status columns (columns: {sorted(cols)})")
        sel = "filename, execution_status" + (", directory" if "directory" in cols else ", ''")
        rows = con.execute(f"select {sel} from mutant").fetchall()
        con.close()
    except sqlite3.Error as e:
        raise Fail(f"report unreadable: {db_path} ({e})")
    per = collections.defaultdict(lambda: [0, 0])
    bad = collections.Counter()
    for filename, status, directory in rows:
        if directory and not posixpath.isabs(filename):
            filename = posixpath.join(directory, filename)
        name = normalise(filename, strips)
        if status in KILLED:
            per[name][0] += 1
        elif status == SURVIVED:
            per[name][1] += 1
        else:
            bad[status] += 1
    if bad:
        found = ", ".join(f"{n} with status {s} ({OTHER_NAMES.get(s, 'unknown')})" for s, n in sorted(bad.items(), key=lambda kv: str(kv[0])))
        raise Fail(f"{db_path}: mutants with a status that is neither killed {sorted(KILLED)} nor survived {SURVIVED}: {found}")
    return dict(per)


def pct(k, s):
    return 100.0 * k / (k + s) if k + s else 0.0


def parse_log(text):
    """Mull's own numbers from its console output: (score or None, survived or None, total or None)."""
    score = survived = total = None
    m = re.findall(r"[Mm]utation score:\s*([0-9]+(?:\.[0-9]+)?)\s*%", text)
    if m:
        score = float(m[-1])
    m = re.findall(r"Surviv(?:ed|ing) mutants\s*(?:\(\s*(\d+)\s*(?:/\s*(\d+))?\s*\)|:\s*(\d+))", text)
    if m:
        a, b, c = m[-1]
        survived = int(a or c)
        total = int(b) if b else None
    elif re.search(r"[Aa]ll mutants (?:were )?killed|No surviv", text):
        survived = 0
    if total is None:
        m = re.findall(r"[Rr]unning mutants[^\n]*?(\d+)\s*/\s*(\d+)", text)
        if m:
            total = int(m[-1][1])
    if score is None and survived is not None and total:
        score = 100.0 * (total - survived) / total
    return score, survived, total


def check_plan(reports, plan_path):
    plan = json.loads(pathlib.Path(plan_path).read_text())
    shards = {e["shard"]: set(e["files"]) for e in plan["matrix"]["include"]}
    if not shards:
        raise Fail(f"{plan_path}: the plan has no shards (mode {plan.get('mode')!r}); nothing to score")
    seen, problems = {}, []
    for path, per in reports.items():
        m = re.fullmatch(r"mull-(.+)\.sqlite", pathlib.Path(path).name)
        sid = m.group(1) if m else None
        if sid not in shards:
            problems.append(f"{path}: not a report of a planned shard {sorted(shards)}")
            continue
        if sid in seen:
            problems.append(f"shard {sid} has two reports: {seen[sid]} and {path}")
        seen[sid] = path
        stray = sorted(set(per) - shards[sid])
        if stray:
            problems.append(f"{path}: mutants in files outside shard {sid}: {stray}")
    missing = sorted(set(shards) - set(seen))
    if missing:
        problems.append(f"missing shard reports: {missing} (expected {len(shards)}, found {len(seen)})")
    if problems:
        raise Fail("incomplete or inconsistent shard reports:\n  " + "\n  ".join(problems))
    return plan


def read_floor(a):
    if a.floor_file:
        data = json.loads(pathlib.Path(a.floor_file).read_text())
        if not isinstance(data.get("project_floor"), (int, float)):
            raise Fail(f"{a.floor_file}: no numeric key project_floor", 2)
        if a.floor is not None and a.floor != data["project_floor"]:
            raise Fail("--floor and --floor-file disagree", 2)
        return float(data["project_floor"])
    return a.floor


def run(a):
    if a.compare_log and len(a.db) != 1:
        raise Fail("--compare-log needs exactly one report", 2)
    floor = read_floor(a)
    if a.baseline and floor is None:
        raise Fail("--baseline needs --floor or --floor-file (for files missing from the baseline)", 2)

    reports = {d: read_report(d, a.strip) for d in a.db}
    if a.plan:
        check_plan(reports, a.plan)

    per, owner = collections.defaultdict(lambda: [0, 0]), {}
    print(f"{'report':<34} {'files':>5} {'mutants':>7} {'killed':>6} {'survived':>8} {'score':>6}")
    tiny = []
    for d, rep in reports.items():
        k = sum(v[0] for v in rep.values()); s = sum(v[1] for v in rep.values())
        print(f"{pathlib.Path(d).name:<34} {len(rep):>5} {k + s:>7} {k:>6} {s:>8} {pct(k, s):5.1f}%")
        if k + s < a.min_mutants:
            tiny.append(f"{d}: {k + s} mutants (minimum {a.min_mutants})")
        for f, (fk, fs) in rep.items():
            if f in owner:
                raise Fail(f"{f} appears in two reports: {owner[f]} and {d}")
            owner[f] = d
            per[f][0] += fk; per[f][1] += fs
    if tiny:
        raise Fail("too few mutants; the run did not do its job:\n  " + "\n  ".join(tiny))

    killed = sum(v[0] for v in per.values()); survived = sum(v[1] for v in per.values()); total = killed + survived
    print()
    width = max((len(f) for f in per), default=10)
    print(f"{'file':<{width}}  killed/total  survived  score")
    for f, (k, s) in sorted(per.items(), key=lambda kv: (pct(*kv[1]), kv[0])):
        print(f"{f:<{width}}  {k:>6}/{k + s:<5}  {s:>8}  {pct(k, s):5.1f}%")
    score = pct(killed, survived)
    print(f"\nSurviving mutants: {survived}")

    if a.compare_log:
        mull_score, mull_survived, mull_total = parse_log(pathlib.Path(a.compare_log).read_text(errors="replace"))
        print(f"Mull's log: score {mull_score}, surviving {mull_survived}, total {mull_total}; this script: "
              f"score {score:.2f}, surviving {survived}, total {total}")
        if mull_score is None:
            raise Fail(f"{a.compare_log}: no mutation score or surviving-mutant count found in Mull's output")
        if abs(mull_score - score) > LOG_TOLERANCE:
            raise Fail(f"score from the report {score:.2f}% and Mull's printed score {mull_score:.2f}% differ by more than {LOG_TOLERANCE} points")
        if mull_survived is not None and mull_survived != survived:
            raise Fail(f"surviving mutants: report {survived}, Mull's log {mull_survived}")
        print(f"log check: agrees within {LOG_TOLERANCE} points")

    if a.write_baseline:
        pathlib.Path(a.write_baseline).write_text(json.dumps(
            {f: round(pct(k, s), 1) for f, (k, s) in sorted(per.items())}, indent=2) + "\n")
        print(f"baseline written: {a.write_baseline}")

    if a.baseline:
        base = {f: v for f, v in json.loads(pathlib.Path(a.baseline).read_text()).items() if not f.startswith("_")}
        if not base:
            raise Fail(f"BASELINE MISSING: {a.baseline} holds no per-file scores; record it from a full run "
                       f"(mull_score.py <reports> --write-baseline {a.baseline}). This is not a score failure.",
                       EXIT_BASELINE_MISSING)
        bad = []
        for f, (k, s) in sorted(per.items()):
            need, why = (base[f] - a.tolerance, "baseline") if f in base else (floor, "project floor: not in the baseline")
            print(f"  {f}: {pct(k, s):.1f}% (required {need:.1f}%, {why})")
            if pct(k, s) < need:
                bad.append(f"{f}: {pct(k, s):.1f}% < required {need:.1f}% ({why})")
        print(f"Mutation score (incremental): {score:.1f}% ({killed}/{total} killed); per-file baseline, tolerance {a.tolerance:g} points")
        if bad:
            raise Fail("SCORE TOO LOW:\n  " + "\n  ".join(bad))
        print("all files at or above their baseline")
        return 0

    if floor is None:
        print(f"Mutation score: {score:.1f}% ({killed}/{total} killed); no floor applied")
        return 0
    print(f"Mutation score: {score:.1f}% ({killed}/{total} killed); floor {floor:g}%")
    if score < floor:
        raise Fail(f"SCORE TOO LOW: {score:.1f}% is below the floor {floor:g}%")
    return 0


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("db", nargs="+", help="Mull SQLite report(s)")
    ap.add_argument("--floor", type=float, help="minimum project mutation score in percent")
    ap.add_argument("--floor-file", help="JSON file with the key project_floor (mutation-floor.json)")
    ap.add_argument("--min-mutants", type=int, default=1, help="post-condition per report: fewer mutants than this fails")
    ap.add_argument("--strip", action="append", default=[], help="drop everything up to and including this path fragment")
    ap.add_argument("--plan", help="shard plan JSON from tools/mull_shards.py: check the set of reports against it")
    ap.add_argument("--baseline", help="per-file baseline JSON: gate each file against its recorded score")
    ap.add_argument("--tolerance", type=float, default=2.0, help="percentage points a file may fall below its baseline")
    ap.add_argument("--write-baseline", help="write the per-file scores of these reports to this JSON file")
    ap.add_argument("--compare-log", help="Mull console log of this (single) report: its score must agree")
    a = ap.parse_args(argv)
    try:
        return run(a)
    except Fail as e:
        print(f"FAIL: {e}")
        return e.status


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
