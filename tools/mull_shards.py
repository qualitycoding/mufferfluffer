#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Scope decision and shard matrix of .github/workflows/mutation.yml (plan step S-MUTATE, D-060).

usage: mull_shards.py plan --event <push|schedule|workflow_dispatch> [--dispatch-mode full|incremental]
                           [--subject <head commit subject>] [--before <sha>] [--base-missing]
                           [--changed <file with one path per line>] [--out plan.json]
       mull_shards.py self-test

`plan` prints and writes one JSON object:
  {"mode": "full" | "incremental" | "not-applicable", "reason": "...",
   "matrix": {"include": [{"shard": id, "files": [...], "tests": "<Catch2 filter>", "include_regex": [...]}, ...]},
   "shard_count": N, "changed": [...]}
Modes:
  full            schedule; workflow_dispatch with mode full (the default); a push whose head commit subject starts
                  with `[mutation full]`; a push whose `before` is all zeros (new branch) or not in the clone
                  (--base-missing); or a changed file that can alter every score (FULL_TRIGGERS below).
  incremental     only the shards that own a changed in-scope .cpp file.
  not-applicable  nothing under core/, adapters/ or tests/ and none of the mutation files changed.
Every in-scope source file on disk (core/src/*.cpp, adapters/src/*.cpp) must belong to exactly one shard, and an
in-scope .cpp that no shard owns is an error: the script fails rather than emit a partial matrix (exit 1).
`self-test` checks the scope rules on synthetic change sets and the shard table against the files on disk.
"""
import argparse
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SCOPE_GLOBS = ("core/src/*.cpp", "adapters/src/*.cpp")
ZERO_SHA = "0" * 40
FULL_MARKER = "[mutation full]"
SHARED = "shared"
MIN_MUTANTS = 5  # per shard (plan S-MUTATE); a shard measured below this is listed in MERGED_INTO_SHARED

SHARED_TESTS = "[tables],[params],[blocks],[lifecycle],[cv],[bypass],[smoothing],[meters]"
EFFECT_TAGS = "[lowpass],[ringmod],[phaser],[delay],[chorus],[filterbank],[oscbox]"

# shard id -> (source files, Catch2 test filter; a comma between tags means "or"). Order is the matrix order.
SHARDS = {
    "lowpass": (["core/src/lowpass.cpp"], "[lowpass]"),
    "ringmod": (["core/src/ringmod.cpp"], "[ringmod]"),
    "phaser": (["core/src/phaser.cpp"], "[phaser]"),
    "delay": (["core/src/delay.cpp"], "[delay]"),
    "chorus": (["core/src/chorus.cpp"], "[chorus]"),
    "filterbank": (["core/src/filterbank.cpp"], "[filterbank]"),
    "oscbox": (["core/src/oscbox.cpp"], "[oscbox]"),
    "clocked_line": (["core/src/clocked_line.cpp"], "[delay],[chorus]"),
    # DEVIATIONS.md row of S-010: dsp_common.cpp gets a shard of its own with the effect tags as its test list.
    "dsp_common": (["core/src/dsp_common.cpp"], EFFECT_TAGS),
    SHARED: (["core/src/blocks.cpp", "core/src/create.cpp", "core/src/effect_base.cpp", "core/src/tables.cpp"], SHARED_TESTS),
    "adapters": (["adapters/src/adapters.cpp"], "[adapters]"),
}

# Shards that the measuring run (sub-step 1) found below MIN_MUTANTS: their files move into `shared` and their test
# filter is added to shared's, so the tests that exercise those files still run. Each entry is recorded in
# reviews/MUTATION.md. Empty until measured.
MERGED_INTO_SHARED = ()

# A change to any of these can alter every file's score, so it widens the run to `full`.
FULL_TRIGGER_PATTERNS = (
    r"tests/.*",
    r"mull\.yml",
    r"tools/mull_[^/]*",
    r"\.github/workflows/mutation\.yml",
    r"mutation-floor\.json",
    r"mutation-baseline\.json",
    r"deps\.lock\.json",  # holds the Mull pin
    r"(core|adapters)/.*",  # any header, generated table or other non-.cpp file under core/ or adapters/ (checked last)
)
SCOPE_CPP = re.compile(r"^(core|adapters)/src/[^/]+\.cpp$")
RELEVANT_PREFIXES = ("core/", "adapters/", "tests/")


class PlanError(Exception):
    pass


def shard_table(merged=MERGED_INTO_SHARED):
    table = {k: (list(f), t) for k, (f, t) in SHARDS.items()}
    for sid in merged:
        if sid == SHARED or sid not in table:
            raise PlanError(f"cannot merge shard {sid!r} into {SHARED!r}")
        files, tests = table.pop(sid)
        shared_files, shared_tests = table[SHARED]
        tags = shared_tests.split(",") + [t for t in tests.split(",") if t not in shared_tests.split(",")]
        table[SHARED] = (sorted(shared_files + files), ",".join(tags))
    return table


def owner_of(table):
    owner = {}
    for sid, (files, _) in table.items():
        for f in files:
            if f in owner:
                raise PlanError(f"{f} is in two shards: {owner[f]} and {sid}")
            owner[f] = sid
    return owner


def scope_files(root):
    found = set()
    for pattern in SCOPE_GLOBS:
        found |= {p.relative_to(root).as_posix() for p in root.glob(pattern) if p.is_file()}
    return sorted(found)


def check_coverage(table, root):
    owner = owner_of(table)
    on_disk = scope_files(root)
    missing = [f for f in on_disk if f not in owner]
    absent = [f for f in owner if f not in on_disk]
    if missing:
        raise PlanError(f"in-scope files that no shard owns: {missing} (add them to SHARDS in tools/mull_shards.py)")
    if absent:
        raise PlanError(f"shard files that do not exist: {absent}")
    return owner


def entry(sid, table):
    files, tests = table[sid]
    return {"shard": sid, "files": files, "tests": tests, "include_regex": [re.escape(f) + "$" for f in files]}


def is_full_trigger(path):
    if SCOPE_CPP.match(path):
        return False
    return any(re.fullmatch(p, path) for p in FULL_TRIGGER_PATTERNS)


def decide(event, changed, *, dispatch_mode="full", subject="", before="", base_missing=False, root=ROOT,
           merged=MERGED_INTO_SHARED):
    table = shard_table(merged)
    owner = check_coverage(table, root)
    full = {"mode": "full", "matrix": {"include": [entry(s, table) for s in table]}}

    def done(result, reason):
        result = dict(result, reason=reason, changed=sorted(changed))
        result.setdefault("matrix", {"include": []})
        result["shard_count"] = len(result["matrix"]["include"])
        return result

    if event == "schedule":
        return done(full, "scheduled run")
    if event == "workflow_dispatch":
        if dispatch_mode not in ("full", "incremental"):
            raise PlanError(f"workflow_dispatch mode must be full or incremental, not {dispatch_mode!r}")
        if dispatch_mode == "full":
            return done(full, "workflow_dispatch, mode full")
    elif event == "push":
        if subject.startswith(FULL_MARKER):
            return done(full, f"head commit subject starts with {FULL_MARKER}")
        if not before or before == ZERO_SHA:
            return done(full, "new branch: no commit before the push")
        if base_missing:
            return done(full, f"the commit before the push ({before}) is not in the clone")
    else:
        raise PlanError(f"unsupported event {event!r}")

    triggers = [p for p in changed if is_full_trigger(p)]
    if triggers:
        return done(full, f"changed files that can alter every score: {triggers[:10]}")
    cpp = [p for p in changed if SCOPE_CPP.match(p)]
    unowned = [p for p in cpp if p not in owner and (root / p).exists()]
    if unowned:
        raise PlanError(f"changed in-scope files that no shard owns: {unowned}")
    shards = {owner[p] for p in cpp if p in owner}
    deleted = [p for p in cpp if p not in owner]
    if deleted:  # a deleted .cpp leaves the build changed: mutate everything rather than guess
        return done(full, f"in-scope files deleted or renamed: {deleted}")
    if shards:
        include = [entry(s, table) for s in table if s in shards]
        return done({"mode": "incremental", "matrix": {"include": include}}, f"changed in-scope files: {sorted(cpp)}")
    if any(p.startswith(RELEVANT_PREFIXES) for p in changed):
        raise PlanError(f"unclassified change under core/, adapters/ or tests/: {changed}")
    return done({"mode": "not-applicable"}, "no file under core/, adapters/ or tests/ and no mutation file changed")


def cmd_plan(a):
    changed = []
    if a.changed:
        changed = [l.strip() for l in pathlib.Path(a.changed).read_text().splitlines() if l.strip()]
    result = decide(a.event, changed, dispatch_mode=a.dispatch_mode, subject=a.subject, before=a.before,
                    base_missing=a.base_missing)
    if result["mode"] != "not-applicable" and result["shard_count"] == 0:
        raise PlanError("empty matrix for a run that should mutate")
    text = json.dumps(result, indent=2)
    print(text)
    if a.out:
        pathlib.Path(a.out).write_text(text + "\n")
    return 0


def self_test():
    failures = []

    def check(name, got, want):
        if got != want:
            failures.append(f"{name}: got {got!r}, want {want!r}")

    def shards_of(r):
        return [e["shard"] for e in r["matrix"]["include"]]

    sha = "1" * 40
    push = lambda changed, **kw: decide("push", changed, before=kw.pop("before", sha), **kw)
    all_ids = list(SHARDS)

    r = push(["core/include/muff/core.hpp"]);                      check("header -> full", (r["mode"], shards_of(r)), ("full", all_ids))
    r = push(["core/src/effect_base.hpp"]);                        check("private header -> full", r["mode"], "full")
    r = push(["core/src/generated_tables.inc"]);                   check("generated table -> full", r["mode"], "full")
    r = push(["core/src/phaser.cpp"]);                             check("phaser.cpp -> one shard", (r["mode"], shards_of(r)), ("incremental", ["phaser"]))
    check("phaser shard tests", r["matrix"]["include"][0]["tests"], "[phaser]")
    check("phaser include regex", r["matrix"]["include"][0]["include_regex"], [r"core/src/phaser\.cpp$"])
    r = push(["core/src/tables.cpp", "core/src/create.cpp", "README.md"])
    check("two shared files -> shared once", (r["mode"], shards_of(r)), ("incremental", ["shared"]))
    r = push(["core/src/delay.cpp", "adapters/src/adapters.cpp"])
    check("two shards in matrix order", shards_of(r), ["delay", "adapters"])
    r = push(["README.md", "docs/USER_GUIDE.md", "reviews/MUTATION.md"]); check("docs only -> not-applicable", (r["mode"], r["shard_count"]), ("not-applicable", 0))
    r = push(["vst3/src/plugin.cpp", "rack/src/module_shell.cpp"]); check("shells only -> not-applicable", r["mode"], "not-applicable")
    r = push(["core/src/phaser.cpp"], before=ZERO_SHA);            check("zero before -> full", r["mode"], "full")
    r = push(["core/src/phaser.cpp"], before="");                  check("no before -> full", r["mode"], "full")
    r = push(["core/src/phaser.cpp"], base_missing=True);          check("before not in clone -> full", r["mode"], "full")
    r = push(["README.md"], subject="[mutation full] measure");     check("marker -> full", r["mode"], "full")
    r = push(["README.md"], subject="fix: x [mutation full]");      check("marker not at start -> no", r["mode"], "not-applicable")
    for path in ("tests/core/test_m1.cpp", "mull.yml", "tools/mull_score.py", ".github/workflows/mutation.yml",
                 "mutation-floor.json", "mutation-baseline.json", "deps.lock.json", "adapters/include/muff/adapters.hpp"):
        check(f"{path} -> full", push([path])["mode"], "full")
    check("ci.yml alone -> not-applicable", push([".github/workflows/ci.yml"])["mode"], "not-applicable")
    check("schedule -> full", decide("schedule", [])["mode"], "full")
    check("dispatch default -> full", decide("workflow_dispatch", [])["mode"], "full")
    r = decide("workflow_dispatch", ["core/src/oscbox.cpp"], dispatch_mode="incremental")
    check("dispatch incremental", shards_of(r), ["oscbox"])
    check("dispatch incremental, nothing", decide("workflow_dispatch", [], dispatch_mode="incremental")["mode"], "not-applicable")
    for bad in (lambda: decide("pull_request", []), lambda: decide("workflow_dispatch", [], dispatch_mode="fast")):
        try:
            bad(); failures.append("an unsupported event or mode was accepted")
        except PlanError:
            pass

    # every in-scope file on disk is in exactly one shard; ten plan shards plus dsp_common
    owner = check_coverage(shard_table(), ROOT)
    check("files covered", sorted(owner), scope_files(ROOT))
    check("shard count", len(SHARDS), 11)
    full = decide("schedule", [])
    check("full matrix covers every file once", sorted(f for e in full["matrix"]["include"] for f in e["files"]), scope_files(ROOT))

    # merge rule: a small shard moves into shared, with its tests
    t = shard_table(("adapters",))
    check("merged shard gone", "adapters" in t, False)
    check("merged files", "adapters/src/adapters.cpp" in t[SHARED][0], True)
    check("merged tests", t[SHARED][1], SHARED_TESTS + ",[adapters]")
    r = decide("push", ["adapters/src/adapters.cpp"], before=sha, merged=("adapters",))
    check("merged shard routes to shared", shards_of(r), [SHARED])
    t = shard_table(("clocked_line",))
    check("merged tags not duplicated", t[SHARED][1], SHARED_TESTS + ",[delay],[chorus]")
    try:
        shard_table((SHARED,)); failures.append("merging shared into itself was accepted")
    except PlanError:
        pass

    # an unowned in-scope file fails closed (synthetic tree)
    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        root = pathlib.Path(tmp)
        for f in owner:
            (root / f).parent.mkdir(parents=True, exist_ok=True); (root / f).write_text("")
        decide("schedule", [], root=root)
        (root / "core/src/new_effect.cpp").write_text("")
        try:
            decide("push", ["core/src/new_effect.cpp"], before=sha, root=root); failures.append("an unowned file was accepted")
        except PlanError as e:
            check("unowned message", "no shard owns" in str(e), True)
        (root / "core/src/new_effect.cpp").unlink()
        (root / "core/src/oscbox.cpp").unlink()
        try:
            decide("schedule", [], root=root); failures.append("a missing shard file was accepted")
        except PlanError:
            pass

    for f in failures:
        print("FAIL:", f)
    if failures:
        return 1
    print("mull_shards self-test: all checks passed")
    return 0


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("plan")
    p.add_argument("--event", required=True)
    p.add_argument("--dispatch-mode", default="full")
    p.add_argument("--subject", default="")
    p.add_argument("--before", default="")
    p.add_argument("--base-missing", action="store_true")
    p.add_argument("--changed")
    p.add_argument("--out")
    sub.add_parser("self-test")
    a = ap.parse_args(argv)
    try:
        return cmd_plan(a) if a.cmd == "plan" else self_test()
    except PlanError as e:
        print(f"FAIL: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
