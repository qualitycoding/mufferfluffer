#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Checks that every `uses: <owner>/<repo>@<sha> # <tag>` line in .github/workflows/*.yml pins the commit that
the tag really names (`git ls-remote`, tag or peeled tag). A `uses:` line without the trailing tag comment fails.
usage: check_pins.py [workflow-dir]      run from the repository root; exit 0 when every pin is true."""
import pathlib, re, subprocess, sys

USES = re.compile(r"^\s*-?\s*uses:\s*(\S+)(.*)$")
PIN = re.compile(r"^([\w.-]+/[\w.-]+)(?:/\S+)?@([0-9a-f]{40})$")
TAG = re.compile(r"^\s*#\s*(v?\d+(?:\.\d+)*\S*)\s*$")


def remote_commits(repo, tag):
    out = subprocess.run(["git", "ls-remote", f"https://github.com/{repo}", f"refs/tags/{tag}", f"refs/tags/{tag}^{{}}"],
                         capture_output=True, text=True, timeout=120)
    if out.returncode != 0:
        raise RuntimeError(f"git ls-remote failed for {repo}: {out.stderr.strip()}")
    return {line.split()[0] for line in out.stdout.splitlines() if line.strip()}


def main(argv):
    wf_dir = pathlib.Path(argv[1] if len(argv) > 1 else ".github/workflows")
    failures, checked = [], 0
    cache = {}
    for wf in sorted(wf_dir.glob("*.yml")):
        for n, line in enumerate(wf.read_text().splitlines(), 1):
            m = USES.match(line)
            if not m or m.group(1).startswith("./"):
                continue
            where = f"{wf.name}:{n}"
            pin = PIN.match(m.group(1))
            tag = TAG.match(m.group(2))
            if not pin or not tag:
                failures.append(f"{where}: `{line.strip()}` is not of the form `uses: owner/repo@<40 hex> # <tag>`")
                continue
            repo, sha = pin.groups()
            key = (repo, tag.group(1))
            if key not in cache:
                cache[key] = remote_commits(*key)
            checked += 1
            if sha not in cache[key]:
                failures.append(f"{where}: {repo}@{sha} is not the commit of tag {tag.group(1)} (remote has {sorted(cache[key])})")
    for f in failures:
        print("FAIL:", f)
    if failures:
        return 1
    print(f"pins verified against the remote tags: {checked} uses-lines")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
