#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Checks the VCV Rack package that `make -C rack dist` produced (plan step S-033).

usage: check_rack_package.py <dist-dir> <platform>

platform is lin-x64, win-x64, mac-x64 or mac-arm64, written literally by each CI job, so that a package built for
another architecture than the job states fails. The directory must hold exactly one `*.vcvplugin` file, larger than
50 000 bytes and named `Mufferfluffer-2.0.0-<platform>.vcvplugin`. The package is a zstd-compressed tar archive; it
must contain `Mufferfluffer/plugin.json`, the platform's plug-in library, a `Mufferfluffer/LICENSE*` file and the
seven panels `Mufferfluffer/res/<Enum>.svg`. Prints the package name and size.
"""
import json
import pathlib
import subprocess
import sys

MIN_BYTES = 50_000          # the real package is about 97 kB (DEVIATIONS.md)
LIBRARY = {"lin-x64": "plugin.so", "win-x64": "plugin.dll", "mac-x64": "plugin.dylib", "mac-arm64": "plugin.dylib"}
ROOT = pathlib.Path(__file__).resolve().parents[1]


def fail(message):
    print(f"check_rack_package: {message}", file=sys.stderr)
    sys.exit(1)


def expected_members(platform):
    fixture = json.loads((ROOT / "tests" / "fixtures" / "effects.json").read_text(encoding="utf-8"))
    members = ["Mufferfluffer/plugin.json", f"Mufferfluffer/{LIBRARY[platform]}"]
    members += [f"Mufferfluffer/res/{e['enum']}.svg" for e in fixture["effects"]]
    return members


def list_members(package):
    decompress = subprocess.run(["zstd", "-d", "-c", str(package)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if decompress.returncode != 0:
        fail(f"zstd could not decompress {package.name}: {decompress.stderr.decode(errors='replace')}")
    listing = subprocess.run(["tar", "-t"], input=decompress.stdout, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if listing.returncode != 0:
        fail(f"tar could not list {package.name}: {listing.stderr.decode(errors='replace')}")
    return [line.strip().lstrip("./") for line in listing.stdout.decode().splitlines() if line.strip()]


def main(argv):
    if len(argv) != 2 or argv[1] not in LIBRARY:
        fail(f"usage: check_rack_package.py <dist-dir> <{'|'.join(LIBRARY)}>")
    dist, platform = pathlib.Path(argv[0]), argv[1]
    packages = sorted(dist.glob("*.vcvplugin"))
    if len(packages) != 1:
        fail(f"expected exactly one .vcvplugin file in {dist}, found {len(packages)}")
    package = packages[0]
    want = f"Mufferfluffer-2.0.0-{platform}.vcvplugin"
    if package.name != want:
        fail(f"package is named {package.name}, expected {want}")
    size = package.stat().st_size
    if size <= MIN_BYTES:
        fail(f"package is {size} bytes, not larger than {MIN_BYTES}")
    members = set(list_members(package))
    missing = [m for m in expected_members(platform) if m not in members]
    if not any(m.startswith("Mufferfluffer/LICENSE") for m in members):
        missing.append("Mufferfluffer/LICENSE*")
    if missing:
        fail("package lacks: " + ", ".join(missing))
    print(f"package {package.name}: {size} bytes, contents verified")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
