#!/usr/bin/env python3
"""Verifies that every frozen file still has the SHA-256 recorded in tests/FROZEN_MANIFEST.sha256.
usage: check_frozen.py            verify (exit 0 intact, 1 on any change, missing file or malformed manifest)
       check_frozen.py --write    planner only: (re)write the manifest from the frozen file list below
Run from the repository root."""
import hashlib, pathlib, subprocess, sys

MANIFEST = pathlib.Path("tests/FROZEN_MANIFEST.sha256")
PATTERNS = ["tests/**/*", "core/include/**/*", "adapters/include/**/*", "vst3/include/**/*",
            "tools/gen_tables.py", "tools/gen_expected.py", "tools/make_effects_fixture.py", "tools/check_frozen.py"]

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()

def frozen_files():
    out = set()
    for pat in PATTERNS:
        for p in pathlib.Path(".").glob(pat):
            if p.is_file() and p != MANIFEST and "__pycache__" not in p.parts and p.suffix != ".pyc": out.add(p.as_posix())
    return sorted(out)

if len(sys.argv) > 1 and sys.argv[1] == "--write":
    MANIFEST.write_text("".join(f"{sha(pathlib.Path(f))}  {f}\n" for f in frozen_files()))
    print(f"manifest written: {len(frozen_files())} files"); sys.exit(0)

if not MANIFEST.is_file(): print("FAIL: manifest missing"); sys.exit(1)
bad, listed = [], set()
for line in MANIFEST.read_text().splitlines():
    parts = line.split("  ", 1)
    if len(parts) != 2 or len(parts[0]) != 64: bad.append(f"malformed line: {line!r}"); continue
    digest, name = parts; listed.add(name); p = pathlib.Path(name)
    if not p.is_file(): bad.append(f"missing: {name}")
    elif sha(p) != digest: bad.append(f"changed: {name}")
for f in frozen_files():
    if f not in listed: bad.append(f"new file in a frozen directory: {f}")
if len(listed) < 10: bad.append("manifest lists fewer than 10 files")
if bad:
    print("FAIL: frozen files do not match the manifest"); [print("  " + b) for b in bad]; sys.exit(1)
print(f"frozen files intact: {len(listed)} files")
