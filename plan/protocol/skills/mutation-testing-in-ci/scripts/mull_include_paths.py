#!/usr/bin/env python3
"""mull_include_paths.py - write a mull.yml that mutates only the changed source files.

  git diff --name-only BASE...HEAD | mull_include_paths.py --scope 'Source/DSP/' --scope 'Source/Presets/' \
      --base mull.yml --out mull.incremental.yml

Reads changed file names on stdin, keeps those under a --scope prefix with a source extension, and
writes the --base config with `includePaths` replaced by one anchored regex per kept file. Prints the
kept files; exits 3 when none remain (the caller reports "not applicable", it must not call that a pass).
Mull reads `includePaths`/`excludePaths` as regular expressions (observed with mull-runner-19, 2026-11).
"""
import argparse
import re
import sys

EXT = (".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx")


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--scope", action="append", required=True, help="path prefix that may be mutated")
    ap.add_argument("--base", required=True, help="base mull.yml (its excludePaths and mutators are kept)")
    ap.add_argument("--out", required=True)
    a = ap.parse_args(argv)

    changed = [l.strip() for l in sys.stdin if l.strip()]
    kept = [f for f in changed if f.endswith(EXT) and any(f.startswith(s) for s in a.scope)]
    if not kept:
        print("no mutable source files changed")
        return 3

    lines, skipping = [], False
    for line in open(a.base).read().splitlines():
        if re.match(r"^includePaths\s*:", line):
            skipping = True
            continue
        if skipping and re.match(r"^\s+-\s", line):
            continue
        skipping = False
        lines.append(line)
    lines.append("includePaths:")
    lines += [f"  - '{re.escape(f)}$'" for f in kept]
    open(a.out, "w").write("\n".join(lines) + "\n")
    print("\n".join(kept))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
