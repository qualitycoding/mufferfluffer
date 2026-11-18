#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Downloads the pluginval release archive of D-007, verifies its SHA-256 and unpacks it.

usage: fetch_pluginval.py <Linux|macOS|Windows> <dest>

The hash comes from deps.lock.json and is checked before anything is unpacked. Exits non-zero on a mismatch or after
three failed downloads 30 s apart. The last line of output is the absolute path of the one pluginval binary found
under <dest>.
"""
import hashlib
import json
import os
import pathlib
import shutil
import subprocess
import sys
import time
import urllib.request
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
PLATFORMS = ("Linux", "macOS", "Windows")
URL = "https://github.com/Tracktion/pluginval/releases/download/v{version}/pluginval_{platform}.zip"
ATTEMPTS = 3
WAIT_SECONDS = 30


def fail(message):
    print(f"fetch_pluginval: {message}", file=sys.stderr)
    sys.exit(1)


def download(url, target):
    last = None
    for attempt in range(1, ATTEMPTS + 1):
        try:
            with urllib.request.urlopen(url, timeout=120) as response, open(target, "wb") as out:
                shutil.copyfileobj(response, out)
            return
        except OSError as err:
            last = err
            print(f"attempt {attempt} of {ATTEMPTS} failed: {err}", file=sys.stderr)
            if attempt < ATTEMPTS:
                time.sleep(WAIT_SECONDS)
    fail(f"download of {url} failed after {ATTEMPTS} attempts: {last}")


def sha256_of(path):
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def unpack(archive, dest):
    root = dest.resolve()
    with zipfile.ZipFile(archive) as zf:
        for info in zf.infolist():
            target = (dest / info.filename).resolve()
            if root != target and root not in target.parents:
                fail(f"archive member {info.filename!r} would leave {dest}")
        zf.extractall(dest)


def binary_name(platform):
    return "pluginval.exe" if platform == "Windows" else "pluginval"


def find_binary(dest, platform):
    name = binary_name(platform)
    found = [p for p in dest.rglob(name) if p.is_file() and (platform != "macOS" or p.parent.name == "MacOS")]
    if len(found) != 1:
        fail(f"expected exactly one {name} under {dest}, found {len(found)}")
    return found[0]


def main(argv):
    if len(argv) != 2 or argv[0] not in PLATFORMS:
        fail(f"usage: fetch_pluginval.py <{'|'.join(PLATFORMS)}> <dest>")
    platform, dest = argv[0], pathlib.Path(argv[1]).resolve()
    lock = json.loads((ROOT / "deps.lock.json").read_text(encoding="utf-8"))["pluginval"]
    want = lock["sha256"][platform]
    dest.mkdir(parents=True, exist_ok=True)
    archive = dest / f"pluginval_{platform}.zip"
    download(URL.format(version=lock["version"], platform=platform), archive)
    got = sha256_of(archive)
    if got != want:
        fail(f"SHA-256 mismatch for pluginval {platform}: got {got}, expected {want}")
    unpack(archive, dest)
    binary = find_binary(dest, platform)
    binary.chmod(binary.stat().st_mode | 0o111)
    if platform == "macOS":
        # "no such attribute" is fine: the archive may carry no quarantine mark.
        subprocess.run(["xattr", "-dr", "com.apple.quarantine", str(dest)], check=False,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    print(os.fspath(binary))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
