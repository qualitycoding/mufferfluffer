#!/usr/bin/env python3
"""Planning spike (R6): builds a minimal synthetic repository tree that satisfies tests/py/test_repo.py, to prove
those frozen tests are satisfiable. usage: make_tree.py <repo root> <output dir>"""
import json, pathlib, shutil, subprocess, sys
src, out = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
if out.exists(): shutil.rmtree(out)
out.mkdir(parents=True)
fx = json.loads((src / "tests/fixtures/effects.json").read_text())["effects"]
def w(rel, text): p = out / rel; p.parent.mkdir(parents=True, exist_ok=True); p.write_text(text)
shutil.copy(src / "LICENSE", out / "LICENSE")
w("README.md", "# mufferfluffer\nNot derived from and not affiliated with any manufacturer.\n## Build\nx\n## Install\nx\n## Tests\nx\n## Licence\nAGPL-3.0-only\n")
w("CHANGELOG.md", "# Changelog\n## 0.1.0\n- first version\n")
guide = "# User guide\n"
for e in fx:
    guide += f"\n## {e['name']}\n" + "".join(f"- {x['name']}\n" for x in e["params"] + e["inputs"] + e["outputs"])
w("docs/USER_GUIDE.md", guide)
codes = ["Mfl1", "Mfr1", "Mfp1", "Mfd1", "Mfb1", "Mfo1", "Mfc1"]
cm = "# GIT_TAG d60b7a78030661574a615b586d9325ba7a7fa7dd\n"
for e, c in zip(fx, codes):
    cm += f'juce_add_plugin(Muff{e["enum"]}\n  COMPANY_NAME "Mufferfluffer"\n  PLUGIN_MANUFACTURER_CODE Muff\n  PLUGIN_CODE {c}\n  FORMATS VST3\n  PRODUCT_NAME "{e["name"]}")\n'
w("vst3/CMakeLists.txt", cm)
w("tests/CMakeLists.txt", "# GIT_TAG 08092139210881f01a34e60c5f2fd6afa0ea4024\n")
hdr = "// SPDX-License-Identifier: AGPL-3.0-only\n"
w("vst3/src/processor.cpp", hdr); w("rack/src/plugin.cpp", hdr); w("core/src/effects.cpp", hdr); w("adapters/src/adapters.cpp", hdr)
subprocess.run([sys.executable, str(src / "tools/gen_tables.py"), str(src / "tests/fixtures/effects.json"), str(out / "core/src/generated_tables.inc")], check=True)
tags = {"Lowpass": ["Filter"], "RingMod": ["Ring modulator"], "Phaser": ["Phaser"], "Delay": ["Delay"], "FilterBank": ["Filter", "Sequencer"], "OscBox": ["Oscillator"], "Chorus": ["Chorus", "Flanger"]}
w("rack/plugin.json", json.dumps({"slug": "Mufferfluffer", "name": "Mufferfluffer", "brand": "Mufferfluffer", "version": "2.0.0", "license": "AGPL-3.0-only",
   "author": "qualitycoding", "sourceUrl": "https://github.com/qualitycoding/mufferfluffer",
   "modules": [{"slug": e["enum"], "name": e["name"], "description": "Effect module", "tags": tags[e["enum"]] + ["Effect"]} for e in fx]}, indent=1))
for e in fx:
    w(f"rack/res/{e['enum']}.svg", '<svg xmlns="http://www.w3.org/2000/svg" width="60.96mm" height="128.5mm" viewBox="0 0 60.96 128.5"><path d="M0 0h60.96v128.5H0z"/><path d="M1 1h2"/><path d="M3 3h2"/></svg>')
h = "0" * 64
w("deps.lock.json", json.dumps({"juce": {"version": "9.0.5", "commit": "d60b7a78030661574a615b586d9325ba7a7fa7dd"},
   "catch2": {"version": "3.16.1", "commit": "08092139210881f01a34e60c5f2fd6afa0ea4024"},
   "rack-sdk": {"version": "2.6.6", "sha256": {p: h for p in ("lin-x64", "win-x64", "mac-x64", "mac-arm64")}},
   "pluginval": {"version": "1.0.4", "sha256": {p: h for p in ("Linux", "Windows", "macOS")}}}, indent=1))
jobs = ["repo-checks", "core-linux", "core-sanitizers", "core-linux-arm64", "core-musl", "core-oldest-toolchain", "core-windows", "core-macos", "core-macos-intel",
        "vst3-linux", "vst3-windows", "vst3-macos", "vst3-macos-intel", "rack-linux", "rack-windows", "rack-macos", "rack-macos-intel", "deps-scan"]
ci = "name: ci\non: [push]\npermissions:\n  contents: read\njobs:\n"
for j in jobs: ci += f"  {j}:\n    runs-on: ubuntu-24.04\n    steps:\n      - uses: actions/checkout@{'a' * 40}\n"
ci += "  aggregate:\n    if: always()\n    needs: [" + ", ".join(jobs) + "]\n    runs-on: ubuntu-24.04\n    steps:\n      - run: echo ok\n"
w(".github/workflows/ci.yml", ci)
w(".github/workflows/mutation.yml", "name: mutation\non:\n  schedule:\n    - cron: '0 3 * * 1'\n  workflow_dispatch:\npermissions:\n  contents: read\njobs:\n  scope:\n    runs-on: ubuntu-24.04\n    steps:\n      - run: echo\n  mutate:\n    runs-on: ubuntu-24.04\n    steps:\n      - run: echo\n  mutation-gate:\n    if: always()\n    runs-on: ubuntu-24.04\n    steps:\n      - run: echo\n")
print("tree written to", out)
