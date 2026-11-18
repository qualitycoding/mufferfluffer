#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Writes the Rack SDK hashes recorded by .github/workflows/record-rack-sdk.yml into deps.lock.json.

usage: apply_rack_record.py <saved-job-log> <run-url>

The log must hold exactly four distinct `RACK_SDK_RECORD {...}` lines, one per platform (lin-x64, win-x64, mac-x64,
mac-arm64). No hash is typed by hand (the plan's Rule 9): this is the only path from the download to the lock file.
"""
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
PLATFORMS = {"lin-x64", "win-x64", "mac-x64", "mac-arm64"}
LINE = re.compile(r"(?:(\d{4}-\d{2}-\d{2})T[\d:.]+Z\s+)?RACK_SDK_RECORD (\{.*\})")


def parse(log_text):
    records, date = {}, None
    for match in LINE.finditer(log_text):
        record = json.loads(match.group(2))
        platform = record["platform"]
        if platform in records:
            raise SystemExit(f"apply_rack_record: platform {platform} appears twice in the log")
        if not re.fullmatch(r"[0-9a-f]{64}", record["sha256"]):
            raise SystemExit(f"apply_rack_record: bad hash for {platform}")
        records[platform] = record
        date = date or match.group(1)
    if set(records) != PLATFORMS:
        raise SystemExit(f"apply_rack_record: need records for {sorted(PLATFORMS)}, found {sorted(records)}")
    return records, date


def main(argv):
    if len(argv) != 2:
        raise SystemExit("usage: apply_rack_record.py <saved-job-log> <run-url>")
    log_path, run_url = argv
    records, date = parse(pathlib.Path(log_path).read_text(encoding="utf-8", errors="replace"))
    if date is None:
        raise SystemExit("apply_rack_record: the log carries no UTC timestamps, so the date of the run is unknown")
    lock_path = ROOT / "deps.lock.json"
    lock = json.loads(lock_path.read_text(encoding="utf-8"))
    sdk = lock["rack-sdk"]
    sdk["sha256"] = {p: records[p]["sha256"] for p in sorted(records)}
    sdk["bytes"] = {p: records[p]["bytes"] for p in sorted(records)}
    sdk["sha256_note"] = f"trust on first use: recorded from vcvrack.com on {date} by run {run_url}"
    lock_path.write_text(json.dumps(lock, indent=2) + "\n", encoding="utf-8")
    print(f"recorded {len(records)} Rack SDK hashes in deps.lock.json")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
