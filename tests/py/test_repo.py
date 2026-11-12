# FROZEN - DO NOT MODIFY. Repository-level tests (T-901 .. T-907): naming, Rack manifest and panels, generated
# tables, dependency pins and workflow hygiene, licence headers, documentation.
# Run:  python3 -m unittest discover -s tests/py -v        (standard library only)
# The repository root is the directory two levels above this file, or $MUFF_REPO_ROOT if set.
import hashlib, json, os, pathlib, re, subprocess, sys, unittest
import xml.etree.ElementTree as ET

ROOT = pathlib.Path(os.environ.get("MUFF_REPO_ROOT", pathlib.Path(__file__).resolve().parents[2]))
FIXTURE = json.loads((pathlib.Path(__file__).resolve().parents[1] / "fixtures" / "effects.json").read_text())
EFFECTS = FIXTURE["effects"]

# Third-party marks that must not appear in the product (D-004). Case-insensitive.
FORBIDDEN = re.compile(r"moog|\bmurf\b|freq\s?box|cluster\s?flux|\bmf-?1\d\d", re.I)
# Planning and research records may name the hardware; the product may not.
ALLOWED_PREFIXES = ("research/", "plan/", "premortem/", "reviews/", "lessons/", "knowledge/", ".checkpoints/", "evidence/")
ALLOWED_FILES = ("tests/py/test_repo.py", "HANDOFF.md", "LICENSE")
TEXT_SUFFIXES = {".cpp", ".hpp", ".h", ".c", ".inc", ".py", ".json", ".md", ".txt", ".cmake", ".yml", ".yaml", ".svg", ".mk", ".sh", ".toml", ".cfg", ""}

RACK_TAGS = {"Arpeggiator", "Attenuator", "Blank", "Chorus", "Clock generator", "Clock modulator", "Compressor", "Controller", "Delay", "Digital",
             "Distortion", "Drum", "Dual", "Dynamics", "Effect", "Envelope follower", "Envelope generator", "Equalizer", "Expander", "External",
             "Filter", "Flanger", "Function generator", "Granular", "Hardware clone", "Limiter", "Logic", "Low-frequency oscillator", "Low-pass gate",
             "MIDI", "Mixer", "Multiple", "Noise", "Oscillator", "Panning", "Phaser", "Physical modeling", "Polyphonic", "Quad", "Quantizer", "Random",
             "Recording", "Reverb", "Ring modulator", "Sample and hold", "Sampler", "Sequencer", "Slew limiter", "Speech", "Switch", "Synth voice",
             "Tuner", "Utility", "Visual", "Vocoder", "Voltage-controlled amplifier", "Waveshaper"}
REQUIRED_TAGS = {"Lowpass": {"Filter"}, "RingMod": {"Ring modulator"}, "Phaser": {"Phaser"}, "Delay": {"Delay"},
                 "FilterBank": {"Filter", "Sequencer"}, "OscBox": {"Oscillator"}, "Chorus": {"Chorus", "Flanger"}}

# Pins fixed by plan/ENVIRONMENT.md.
PINS = {"juce": {"version": "9.0.5", "commit": "d60b7a78030661574a615b586d9325ba7a7fa7dd"},
        "catch2": {"version": "3.16.1", "commit": "08092139210881f01a34e60c5f2fd6afa0ea4024"},
        "rack-sdk": {"version": "2.6.6"},
        "pluginval": {"version": "1.0.4"}}
CI_JOBS = ["repo-checks", "core-linux", "core-sanitizers", "core-linux-arm64", "core-musl", "core-oldest-toolchain", "core-windows", "core-macos",
           "core-macos-intel", "vst3-linux", "vst3-windows", "vst3-macos", "vst3-macos-intel", "rack-linux", "rack-windows", "rack-macos",
           "rack-macos-intel", "deps-scan", "aggregate"]
LICENSE_SHA256 = "d8a6cc31abc16b6748c7a21f21611f5a1ec33f67d22ca23d7da1c19b95496bee"  # AGPL-3.0 text from SPDX license-list-data, as committed on main


def tracked():
    out = subprocess.run(["git", "-C", str(ROOT), "ls-files"], capture_output=True, text=True)
    if out.returncode == 0 and out.stdout.strip():
        return [l for l in out.stdout.split("\n") if l]
    return [str(p.relative_to(ROOT)).replace(os.sep, "/") for p in ROOT.rglob("*") if p.is_file() and ".git/" not in str(p)]


def read(rel):
    p = ROOT / rel
    if not p.is_file():
        raise AssertionError(f"missing deliverable: {rel}")
    return p.read_text(encoding="utf-8")


class T901Naming(unittest.TestCase):
    def test_no_third_party_marks_in_the_product(self):
        # the lint is only meaningful once the product exists
        cm = read("vst3/CMakeLists.txt")
        self.assertEqual(len(re.findall(r"^\s*juce_add_plugin\s*\(", cm, re.M)) + len(re.findall(r"^\s*muff_add_plugin\s*\(", cm, re.M)) >= 7, True,
                         "vst3/CMakeLists.txt must define the seven plug-in targets")
        read("rack/plugin.json")
        hits = []
        for rel in tracked():
            if rel.startswith(ALLOWED_PREFIXES) or rel in ALLOWED_FILES: continue
            p = ROOT / rel
            if FORBIDDEN.search(rel): hits.append(f"{rel}: file name")
            if p.suffix.lower() not in TEXT_SUFFIXES or not p.is_file(): continue
            try: text = p.read_text(encoding="utf-8")
            except UnicodeDecodeError: continue
            lines = [i + 1 for i, line in enumerate(text.split("\n")) if FORBIDDEN.search(line)]
            if rel == "README.md":
                if len(lines) > 1: hits.append(f"README.md: more than one line names the hardware ({lines})")
            elif lines: hits.append(f"{rel}: lines {lines[:5]}")
        self.assertEqual(hits, [])

    def test_product_names_come_from_the_fixture(self):
        cm = read("vst3/CMakeLists.txt")
        for e in EFFECTS:
            self.assertRegex(cm, r'"' + re.escape(e["name"]) + r'"', f"plug-in product name {e['name']!r} missing from vst3/CMakeLists.txt")
        self.assertRegex(cm, r'COMPANY_NAME\s+"Mufferfluffer"')
        self.assertNotRegex(cm, r'(PRODUCT_NAME|COMPANY_NAME)\s+"[^"]*\bVST', "the Steinberg mark must not appear in product or company names")
        codes = re.findall(r"PLUGIN_CODE\s+([A-Za-z0-9]{4})\b", cm) or re.findall(r"muff_add_plugin\s*\(\s*\w+\s+([A-Za-z0-9]{4})\b", cm)
        self.assertEqual(len(set(codes)), 7, "seven distinct four-character plug-in codes are required")


class T902RackManifest(unittest.TestCase):
    def test_manifest(self):
        m = json.loads(read("rack/plugin.json"))
        self.assertEqual(m["slug"], "Mufferfluffer")
        self.assertEqual(m["name"], "Mufferfluffer")
        self.assertEqual(m["brand"], "Mufferfluffer")
        self.assertRegex(m["version"], r"^2\.\d+\.\d+$")
        self.assertEqual(m["license"], "AGPL-3.0-only")
        for key in ("author", "sourceUrl"): self.assertTrue(m.get(key), f"{key} missing")
        self.assertEqual(m["sourceUrl"], "https://github.com/qualitycoding/mufferfluffer")
        self.assertEqual([x["slug"] for x in m["modules"]], [e["enum"] for e in EFFECTS])
        for x, e in zip(m["modules"], EFFECTS):
            self.assertEqual(x["name"], e["name"])
            self.assertTrue(x.get("description"))
            self.assertLessEqual(len(x["description"]), 120)
            tags = set(x["tags"])
            self.assertTrue(tags <= RACK_TAGS, f"unknown Rack tags {tags - RACK_TAGS}")
            self.assertTrue(REQUIRED_TAGS[e["enum"]] <= tags)
            self.assertIn("Hardware clone", tags)
            self.assertIsNone(FORBIDDEN.search(json.dumps(x)))


class T903RackPanels(unittest.TestCase):
    def test_panels(self):
        for e in EFFECTS:
            rel = f"rack/res/{e['enum']}.svg"
            root = ET.fromstring(read(rel))
            self.assertTrue(root.tag.endswith("svg"), rel)
            w, h = root.get("width", ""), root.get("height", "")
            self.assertRegex(w, r"^[0-9.]+mm$", rel); self.assertRegex(h, r"^[0-9.]+mm$", rel)
            self.assertAlmostEqual(float(h[:-2]), 128.5, places=2, msg=rel)
            hp = float(w[:-2]) / 5.08
            self.assertAlmostEqual(hp, round(hp), places=2, msg=f"{rel}: width is not a whole number of HP")
            self.assertTrue(4 <= round(hp) <= 40, rel)
            vb = [float(v) for v in root.get("viewBox", "").split()]
            self.assertEqual(len(vb), 4, f"{rel}: viewBox")
            self.assertAlmostEqual(vb[2], float(w[:-2]), places=2); self.assertAlmostEqual(vb[3], 128.5, places=2)
            banned = {"text", "tspan", "image", "style", "filter", "foreignObject", "script", "use"}
            found = {el.tag.split("}")[-1] for el in root.iter()} & banned
            self.assertEqual(found, set(), f"{rel}: elements Rack's renderer does not support: {found}")
            self.assertGreaterEqual(sum(1 for el in root.iter() if el.tag.endswith("path")), 3, f"{rel}: labels must be paths")
        dark = [f"rack/res/{e['enum']}-dark.svg" for e in EFFECTS]
        for rel in dark:
            if (ROOT / rel).exists(): ET.fromstring(read(rel))


class T904Generated(unittest.TestCase):
    def test_core_tables_are_generated_from_the_fixture(self):
        have = read("core/src/generated_tables.inc")
        tmp = ROOT / "build" / "_gen_tables_check.inc"; tmp.parent.mkdir(exist_ok=True)
        gen = pathlib.Path(__file__).resolve().parents[2] / "tools" / "gen_tables.py"
        subprocess.run([sys.executable, str(gen), str(pathlib.Path(__file__).resolve().parents[1] / "fixtures" / "effects.json"), str(tmp)], check=True)
        self.assertEqual(have, tmp.read_text(), "core/src/generated_tables.inc is stale: run tools/gen_tables.py")
        self.assertFalse((ROOT / "core/src/stubs.cpp").exists(), "the stub core must be gone")
        self.assertFalse((ROOT / "adapters/src/stubs.cpp").exists())
        self.assertFalse((ROOT / "vst3/src/stubs.cpp").exists())


class T905PinsAndWorkflows(unittest.TestCase):
    def test_dependency_lock(self):
        lock = json.loads(read("deps.lock.json"))
        for name, want in PINS.items():
            self.assertIn(name, lock)
            for k, v in want.items(): self.assertEqual(lock[name].get(k), v, f"{name}.{k}")
        for plat in ("lin-x64", "win-x64", "mac-x64", "mac-arm64"):
            self.assertRegex(lock["rack-sdk"]["sha256"][plat], r"^[0-9a-f]{64}$", f"Rack SDK archive hash for {plat}")
        for plat in ("Linux", "Windows", "macOS"):
            self.assertRegex(lock["pluginval"]["sha256"][plat], r"^[0-9a-f]{64}$", f"pluginval archive hash for {plat}")
        cm = read("vst3/CMakeLists.txt") + read("tests/CMakeLists.txt")
        self.assertIn(PINS["juce"]["commit"], cm); self.assertIn(PINS["catch2"]["commit"], cm)

    def test_workflows(self):
        wf_dir = ROOT / ".github" / "workflows"
        self.assertTrue((wf_dir / "ci.yml").is_file(), "missing deliverable: .github/workflows/ci.yml")
        self.assertTrue((wf_dir / "mutation.yml").is_file(), "missing deliverable: .github/workflows/mutation.yml")
        for wf in sorted(wf_dir.glob("*.yml")):
            text = wf.read_text()
            for use in re.findall(r"^\s*-?\s*uses:\s*(\S+)", text, re.M):
                if use.startswith("./"): continue
                self.assertRegex(use, r"@[0-9a-f]{40}$", f"{wf.name}: action {use} is not pinned to a commit")
            self.assertNotRegex(text, r"continue-on-error:\s*true", f"{wf.name}: failures must not be swallowed")
            self.assertNotRegex(text, r"\|\|\s*true", f"{wf.name}: failures must not be swallowed")
            self.assertRegex(text, r"(?m)^permissions:\s*\n\s+contents:\s*read\s*$", f"{wf.name}: top-level permissions must be contents: read")
            self.assertNotRegex(text, r"pull_request_target", wf.name)
        ci = (wf_dir / "ci.yml").read_text()
        for job in CI_JOBS: self.assertRegex(ci, rf"(?m)^  {re.escape(job)}:\s*$", f"ci.yml: job {job} missing")
        self.assertRegex(ci, r"(?s)\n  aggregate:.*?if:\s*always\(\)", "the aggregate job must run always")
        self.assertNotIn("MUFF_TEST_FILTER", ci, "the temporary test filter of the build-up steps must be gone: CI runs the whole suite")
        self.assertEqual(len(re.findall(r"strategy:", ci)), len(re.findall(r"fail-fast:\s*false", ci)), "every matrix must set fail-fast: false")
        self.assertNotRegex(ci, r"runs-on:\s*\S*-latest", "runner labels must be versioned")
        mut = (wf_dir / "mutation.yml").read_text()
        for job in ("scope", "mutate", "mutation-gate"): self.assertRegex(mut, rf"(?m)^  {job}:\s*$", f"mutation.yml: job {job} missing")
        self.assertRegex(mut, r"schedule:"); self.assertRegex(mut, r"workflow_dispatch:")


class T906Licence(unittest.TestCase):
    def test_spdx_headers_and_licence_text(self):
        self.assertEqual(hashlib.sha256((ROOT / "LICENSE").read_bytes()).hexdigest(), LICENSE_SHA256)
        files = [r for r in tracked() if r.startswith(("core/", "adapters/", "vst3/", "rack/src/")) and r.endswith((".cpp", ".hpp", ".h")) and "generated" not in r]
        self.assertTrue(any(r.startswith("rack/src/") for r in files), "missing deliverable: rack/src")
        self.assertTrue(any(r.startswith("vst3/src/") and not r.endswith("stubs.cpp") for r in files), "missing deliverable: vst3/src")
        bad = [r for r in files if "SPDX-License-Identifier: AGPL-3.0-only" not in "\n".join(read(r).split("\n")[:6])]
        self.assertEqual(bad, [])


class T907Docs(unittest.TestCase):
    def test_readme_and_user_guide(self):
        readme = read("README.md")
        for heading in ("## Build", "## Install", "## Tests", "## Licence"): self.assertIn(heading, readme)
        self.assertIn("not affiliated", readme)
        guide = read("docs/USER_GUIDE.md")
        for e in EFFECTS:
            self.assertRegex(guide, rf"(?m)^## {re.escape(e['name'])}\s*$")
            section = guide.split(f"## {e['name']}")[1].split("\n## ")[0]
            for p in e["params"]: self.assertIn(p["name"], section, f"user guide: {e['name']} does not describe {p['name']}")
            for q in e["inputs"] + e["outputs"]: self.assertIn(q["name"], section, f"user guide: {e['name']} does not describe {q['name']}")
        self.assertIsNone(FORBIDDEN.search(guide))
        self.assertTrue(read("CHANGELOG.md").strip())


if __name__ == "__main__":
    unittest.main()
