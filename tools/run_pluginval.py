#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Runs pluginval over VST3 bundles with the command and pass rule of D-007.

usage: run_pluginval.py <pluginval-binary> <vst3-bundle>...
       run_pluginval.py --self-test <pluginval-binary>

The command is `pluginval --strictness-level 10 --timeout-ms 600000 --random-seed 0x6d756666 --validate <bundle>`.
A bundle passes only if the exit status is 0 AND the captured log contains `SUCCESS`, at least one line starting
`Starting tests in:`, and a `Random seed:` line that shows the requested seed (pluginval ignores options it does not
know, so the seed line proves that the option was understood). The log of each bundle goes to pluginval-<name>.log in
the current directory. With --self-test the same rule is applied to an empty directory NotAPlugin.vst3 and the script
exits 0 only if the rule reports failure (the negative control of D-007).
"""
import pathlib
import re
import subprocess
import sys
import tempfile

SEED = "0x6d756666"
SEED_NUMBER = int(SEED, 16)
TIMEOUT_MS = "600000"
STRICTNESS = "10"
SEED_LINE = re.compile(r"Random seed:\s*(\S+)")


def command(binary, bundle):
    return [binary, "--strictness-level", STRICTNESS, "--timeout-ms", TIMEOUT_MS, "--random-seed", SEED,
            "--validate", str(bundle)]


def seed_shown(log):
    for match in SEED_LINE.finditer(log):
        text = match.group(1).rstrip(".,;")
        try:
            if int(text, 0) == SEED_NUMBER:
                return True
        except ValueError:
            continue
    return False


def judge(exit_status, log):
    """Returns a list of reasons why the log does not pass; an empty list means pass (D-007)."""
    problems = []
    if exit_status != 0:
        problems.append(f"exit status {exit_status}")
    if "SUCCESS" not in log:
        problems.append("no SUCCESS in the log")
    if not re.search(r"(?m)^Starting tests in:", log):
        problems.append("no line starting 'Starting tests in:'")
    if not seed_shown(log):
        problems.append(f"the 'Random seed:' line does not show {SEED}")
    return problems


def validate(binary, bundle, log_path):
    result = subprocess.run(command(binary, bundle), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                            errors="replace")
    log = result.stdout
    pathlib.Path(log_path).write_text(log, encoding="utf-8")
    return judge(result.returncode, log)


def self_test(binary):
    with tempfile.TemporaryDirectory() as tmp:
        bundle = pathlib.Path(tmp) / "NotAPlugin.vst3"
        bundle.mkdir()
        problems = validate(binary, bundle, "pluginval-NotAPlugin.log")
    if problems:
        print("self-test: the pass rule rejects an empty bundle, as it must: " + "; ".join(problems))
        return 0
    print("self-test FAILED: the pass rule accepted an empty NotAPlugin.vst3", file=sys.stderr)
    return 1


def main(argv):
    if len(argv) == 2 and argv[0] == "--self-test":
        return self_test(argv[1])
    if len(argv) < 2:
        print(__doc__, file=sys.stderr)
        return 2
    binary, bundles = argv[0], argv[1:]
    failed = 0
    for bundle in bundles:
        name = pathlib.Path(bundle).name
        problems = validate(binary, bundle, f"pluginval-{pathlib.Path(bundle).stem}.log")
        if problems:
            failed += 1
            print(f"FAIL {name}: " + "; ".join(problems))
        else:
            print(f"PASS {name}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
