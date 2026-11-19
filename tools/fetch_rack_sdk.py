#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Downloads and unpacks the VCV Rack SDK archive of D-003.

usage: fetch_rack_sdk.py <platform> <dest> [--record]

platform is lin-x64, win-x64, mac-x64 or mac-arm64. Without --record the archive's SHA-256 must equal the value in
deps.lock.json (a missing entry or a mismatch fails before anything is unpacked). With --record the check is skipped
and one line `RACK_SDK_RECORD {...}` is printed for tools/apply_rack_record.py; an archive under 1 000 000 bytes is
rejected (an error page or a truncated download). The SDK is unpacked so that <dest>/Rack-SDK/plugin.mk and
<dest>/Rack-SDK/include/rack.hpp exist.
"""
import hashlib
import json
import os
import pathlib
import shutil
import stat
import sys
import time
import urllib.request
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
PLATFORMS = ("lin-x64", "win-x64", "mac-x64", "mac-arm64")
URL = "https://vcvrack.com/downloads/Rack-SDK-{version}-{platform}.zip"
ATTEMPTS = 3
WAIT_SECONDS = 30
MIN_BYTES = 1_000_000  # the plan said 5 000 000; the real win-x64 archive is 3 154 102 bytes (DEVIATIONS.md)


def fail(message):
    print(f"fetch_rack_sdk: {message}", file=sys.stderr)
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


def safe_target(dest, name):
    target = (dest / name).resolve()
    if dest.resolve() != target and dest.resolve() not in target.parents:
        fail(f"archive member {name!r} would leave the destination directory")
    return target


def unpack(archive, dest):
    with zipfile.ZipFile(archive) as zf:
        for info in zf.infolist():
            target = safe_target(dest, info.filename)
            mode = info.external_attr >> 16
            if info.is_dir():
                target.mkdir(parents=True, exist_ok=True)
            elif stat.S_ISLNK(mode):
                link_target = zf.read(info).decode("utf-8")
                safe_target(target.parent, link_target)
                target.parent.mkdir(parents=True, exist_ok=True)
                if target.is_symlink() or target.exists():
                    target.unlink()
                os.symlink(link_target, target)
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                with zf.open(info) as src, open(target, "wb") as out:
                    shutil.copyfileobj(src, out)
                if mode & 0o111:
                    target.chmod(target.stat().st_mode | 0o111)
    check_links(dest)


def check_links(dest):
    # A link may point at a path that does not exist yet, or through a link made later, so the targets are only known once
    # everything is unpacked: every link must then resolve inside the destination (S-REVIEW B2-02).
    root = pathlib.Path(os.path.realpath(dest))
    for path in sorted(dest.rglob("*")):
        if path.is_symlink():
            resolved = pathlib.Path(os.path.realpath(path))
            if resolved != root and root not in resolved.parents:
                fail(f"symbolic link {path} resolves outside the destination directory")


def locate_sdk(dest):
    found = sorted({p.parent for p in dest.rglob("plugin.mk")})
    if len(found) != 1:
        fail(f"expected exactly one directory with plugin.mk under {dest}, found {len(found)}")
    return found[0]


def main(argv):
    args = [a for a in argv if not a.startswith("--")]
    record = "--record" in argv
    if len(args) != 2 or args[0] not in PLATFORMS:
        fail(f"usage: fetch_rack_sdk.py <{'|'.join(PLATFORMS)}> <dest> [--record]")
    platform, dest = args[0], pathlib.Path(args[1]).resolve()
    lock = json.loads((ROOT / "deps.lock.json").read_text(encoding="utf-8"))["rack-sdk"]
    url = URL.format(version=lock["version"], platform=platform)
    dest.mkdir(parents=True, exist_ok=True)
    archive = dest / f"Rack-SDK-{platform}.zip"
    download(url, archive)
    size = archive.stat().st_size
    digest = sha256_of(archive)
    if record:
        if size < MIN_BYTES:
            fail(f"archive is {size} bytes, under the {MIN_BYTES}-byte floor: not an SDK archive")
        print("RACK_SDK_RECORD " + json.dumps({"platform": platform, "sha256": digest, "bytes": size}))
    else:
        want = lock.get("sha256", {}).get(platform)
        if not want:
            fail(f"deps.lock.json has no rack-sdk hash for {platform}")
        if digest != want:
            fail(f"SHA-256 mismatch for {platform}: got {digest}, expected {want}")
    staging = dest / "unpacked"
    if staging.exists():
        shutil.rmtree(staging)
    staging.mkdir()
    unpack(archive, staging)
    sdk = locate_sdk(staging)
    final = dest / "Rack-SDK"
    if final.exists():
        shutil.rmtree(final)
    shutil.move(str(sdk), str(final))
    shutil.rmtree(staging, ignore_errors=True)
    for needed in ("plugin.mk", "include/rack.hpp"):
        if not (final / needed).is_file():
            fail(f"{final / needed} is missing after unpacking")
    print(f"Rack SDK for {platform} ready at {final}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
