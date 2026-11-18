#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Asks the OSV service about every commit-pinned dependency of a lock file (D-061).

usage: check_advisories.py <lockfile>

For each top-level entry with a `commit` key the commit is posted to https://api.osv.dev/v1/query (or to the URL in the
environment variable MUFF_OSV_URL, which exists only so that the failure path can be shown). The tool prints one line
per dependency: name, commit and the number of advisories not covered by the optional top-level list `advisory_allow`
of the lock file. Entries without a commit pin (the Rack SDK, pluginval) are printed as not queried. The exit status is
non-zero if any advisory is not allowed, if a request still fails after three attempts 20 s apart, or if the answer
stays incomplete (a response with `next_page_token` is followed, at most 20 pages; an incomplete answer is never read as
"no advisories").
Limits, printed on every run: an empty answer means "none known", not "proven safe", and the query does not see
libraries bundled inside JUCE (zlib, libpng, FLAC, HarfBuzz and others).
"""
import json
import os
import sys
import time
import urllib.request

DEFAULT_URL = "https://api.osv.dev/v1/query"
ATTEMPTS = 3
WAIT_SECONDS = 20
MAX_PAGES = 20


class ScanError(Exception):
    pass


def post(url, payload):
    body = json.dumps(payload).encode("utf-8")
    last = None
    for attempt in range(1, ATTEMPTS + 1):
        try:
            request = urllib.request.Request(url, data=body, headers={"Content-Type": "application/json"})
            with urllib.request.urlopen(request, timeout=60) as response:
                return json.loads(response.read().decode("utf-8"))
        except (OSError, ValueError) as err:
            last = err
            print(f"request attempt {attempt} of {ATTEMPTS} failed: {err}", file=sys.stderr)
            if attempt < ATTEMPTS:
                time.sleep(WAIT_SECONDS)
    raise ScanError(f"request to {url} failed after {ATTEMPTS} attempts: {last}")


def advisories_for(url, commit):
    """All advisory ids that OSV lists for the commit, following the pages of the answer."""
    ids = []
    payload = {"commit": commit}
    for _ in range(MAX_PAGES):
        answer = post(url, payload)
        ids += [v["id"] for v in answer.get("vulns", [])]
        token = answer.get("next_page_token")
        if not token:
            return ids
        payload = {"commit": commit, "page_token": token}
    raise ScanError(f"the answer for {commit} still has more pages after {MAX_PAGES}: treated as incomplete")


def main(argv):
    if len(argv) != 1:
        print(__doc__, file=sys.stderr)
        return 2
    with open(argv[0], encoding="utf-8") as handle:
        lock = json.load(handle)
    url = os.environ.get("MUFF_OSV_URL", DEFAULT_URL)
    allowed = set(lock.get("advisory_allow", []))
    print(f"advisory_allow: {sorted(allowed) if allowed else 'none'}")
    print("limits: an empty answer means none known, not proven safe; libraries bundled inside JUCE are not seen")
    failed = False
    for name, entry in lock.items():
        if not isinstance(entry, dict):
            continue
        commit = entry.get("commit")
        if not commit:
            print(f"{name}: not queried: no commit pin")
            continue
        try:
            found = advisories_for(url, commit)
        except ScanError as err:
            print(f"{name} {commit}: {err}", file=sys.stderr)
            return 1
        open_ids = sorted(set(found) - allowed)
        print(f"{name} {commit}: {len(open_ids)} advisories" + (f" ({', '.join(open_ids)})" if open_ids else ""))
        failed = failed or bool(open_ids)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
