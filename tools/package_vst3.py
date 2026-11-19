#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Pack the seven built .vst3 bundles and LICENSE into one zip archive (S-040).

usage: package_vst3.py <out.zip> <build-dir> <licence-file>

Fails unless there are exactly seven bundles, each holding a regular file, and the archive is larger than 100 kB.
File permissions are stored so that the executables stay executable after unpacking on Linux and macOS.
"""
import os
import pathlib
import sys
import zipfile

MIN_BYTES = 100_000
BUNDLES = 7


def fail(message):
    sys.exit(f"package_vst3: {message}")


def main(argv):
    if len(argv) != 3:
        fail("usage: package_vst3.py <out.zip> <build-dir> <licence-file>")
    out, build, licence = pathlib.Path(argv[0]), pathlib.Path(argv[1]), pathlib.Path(argv[2])
    if not licence.is_file():
        fail(f"{licence} is not a file")
    bundles = sorted(p for p in build.rglob("*.vst3") if p.is_dir())
    if len(bundles) != BUNDLES:
        fail(f"expected {BUNDLES} .vst3 bundles under {build}, found {len(bundles)}")
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as zf:
        zf.write(licence, "LICENSE")
        for bundle in bundles:
            members = 0
            for path in sorted(bundle.rglob("*")):
                arcname = f"{bundle.name}/{path.relative_to(bundle).as_posix()}"
                if path.is_dir():
                    info = zipfile.ZipInfo(arcname + "/")
                    info.external_attr = (0o40755 << 16) | 0x10
                    zf.writestr(info, b"")
                elif path.is_file():
                    info = zipfile.ZipInfo.from_file(path, arcname)
                    info.compress_type = zipfile.ZIP_DEFLATED
                    info.external_attr = (path.stat().st_mode & 0xFFFF) << 16
                    with open(path, "rb") as handle:
                        zf.writestr(info, handle.read())
                    members += 1
            if members == 0:
                fail(f"bundle {bundle} holds no file")
    size = out.stat().st_size
    if size <= MIN_BYTES:
        fail(f"{out} is only {size} bytes")
    print(f"{out.name}: {size} bytes, {BUNDLES} bundles")


if __name__ == "__main__":
    main(sys.argv[1:])
