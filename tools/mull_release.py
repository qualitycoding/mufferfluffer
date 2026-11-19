#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Chooses, records and verifies the two Mull release packages of plan step S-MUTATE (D-060, D-072 item 2).

usage: mull_release.py lock-tag [--lock deps.lock.json]
       mull_release.py resolve --listing <releases.json> [--lock deps.lock.json] --out selection.json
       mull_release.py verify --selection selection.json --dir <download dir> [--lock deps.lock.json]
                              [--github-output <file>]
       mull_release.py self-test

lock-tag  prints the release tag recorded under "mull" in deps.lock.json and exits 0; exits 1 when there is no
          block, 2 when the block is malformed.
resolve   reads a GitHub REST listing (`gh api repos/mull-project/mull/releases`, a JSON list; or one release object
          from `.../releases/tags/<tag>`). With a "mull" block in the lock file it selects exactly the two recorded
          asset names from the recorded tag; without one it selects the newest release (drafts and pre-releases are
          skipped) that has both a runner package `Mull-<version>-ubuntu-amd64-24.04.deb` and an LLVM 19 plug-in
          package `Mull-19-<version>-*ubuntu-amd64-24.04.deb`. When no release matches those patterns exactly, the
          same three facts are used (Ubuntu 24.04, amd64, runner or LLVM 19 plug-in; never `Mull-Extras-`) and the
          names found are printed. Writes the selection (tag, names, download URLs, any `digest`).
verify    computes the SHA-256 of both downloaded files. Where the listing gave a `digest`, the hash must equal it.
          With a lock block the hashes must equal the recorded ones. Without one, the block is written into the lock
          file of this checkout and printed as one line `MULL_RECORD {...}` (the value of "mull", ready to copy into
          deps.lock.json), and the run carries on with the files just checked. Writes runner/plugin names and
          hashes to --github-output.
"""
import argparse
import hashlib
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
LLVM_MAJOR = "19"
SUFFIX = "ubuntu-amd64-24.04.deb"


class Fail(Exception):
    pass


def load_lock(path):
    return json.loads(pathlib.Path(path).read_text(encoding="utf-8"))


def lock_block(lock):
    block = lock.get("mull")
    if block is None:
        return None
    assets = block.get("assets") if isinstance(block, dict) else None
    if not (isinstance(block.get("version"), str) and isinstance(assets, list) and len(assets) == 2
            and all(re.fullmatch(r"[0-9a-f]{64}", a.get("sha256", "")) and a.get("name") for a in assets)):
        raise Fail(f"deps.lock.json has a malformed mull block: {block!r}")
    return block


def roles_strict(release):
    version = release["tag_name"].lstrip("v")
    v = re.escape(version)
    runner = [a for a in release["assets"] if re.fullmatch(rf"Mull-{v}-{re.escape(SUFFIX)}", a["name"])]
    plugin = [a for a in release["assets"] if re.fullmatch(rf"Mull-{LLVM_MAJOR}-{v}-.*{re.escape(SUFFIX)}", a["name"])]
    return runner, plugin


def roles_loose(release):
    def fits(n):
        low = n.lower()
        return (low.endswith(".deb") and not n.startswith("Mull-Extras-") and "ubuntu" in low and "24.04" in low
                and ("amd64" in low or "x86_64" in low))
    debs = [a for a in release["assets"] if fits(a["name"])]
    runner = [a for a in debs if re.match(r"Mull-v?\d+\.\d+", a["name"])]
    plugin = [a for a in debs if re.match(rf"Mull-{LLVM_MAJOR}-", a["name"])]
    return runner, plugin


def as_releases(listing):
    return listing if isinstance(listing, list) else [listing]


def pick(release, runner, plugin, how):
    def item(a, role):
        return {"role": role, "name": a["name"], "url": a.get("browser_download_url", ""), "digest": a.get("digest") or ""}
    return {"version": release["tag_name"], "how": how, "assets": [item(runner, "runner"), item(plugin, "plugin")],
            "all_asset_names": [a["name"] for a in release["assets"]]}


def resolve(listing, block):
    releases = as_releases(listing)
    if block:
        for rel in releases:
            if rel.get("tag_name") == block["version"]:
                by_name = {a["name"]: a for a in rel.get("assets", [])}
                missing = [a["name"] for a in block["assets"] if a["name"] not in by_name]
                if missing:
                    raise Fail(f"release {block['version']} has no asset named {missing}")
                runner, plugin = (by_name[a["name"]] for a in block["assets"])
                sel = pick(rel, runner, plugin, "deps.lock.json")
                for got, want in zip(sel["assets"], block["assets"]):
                    got["sha256"] = want["sha256"]
                return sel
        raise Fail(f"the listing has no release tagged {block['version']}")
    for rel in releases:
        if rel.get("draft") or rel.get("prerelease"):
            continue
        rel.setdefault("assets", [])
        for how, finder in (("exact file-name patterns", roles_strict), ("Ubuntu 24.04 / amd64 / role facts", roles_loose)):
            runner, plugin = finder(rel)
            if len(runner) == 1 and len(plugin) == 1:
                return pick(rel, runner[0], plugin[0], how)
            if len(runner) > 1 or len(plugin) > 1:
                raise Fail(f"release {rel['tag_name']}: ambiguous assets: runner {[a['name'] for a in runner]}, "
                           f"plug-in {[a['name'] for a in plugin]}")
    raise Fail("no release in the listing has both a runner and an LLVM 19 plug-in package for Ubuntu 24.04 amd64")


def sha256_of(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def verify(selection, directory, lock_path, github_output=None):
    lock = load_lock(lock_path)
    block = lock_block(lock)
    computed = []
    for a in selection["assets"]:
        p = pathlib.Path(directory) / a["name"]
        if not p.is_file() or p.stat().st_size == 0:
            raise Fail(f"downloaded file missing or empty: {p}")
        digest = sha256_of(p)
        print(f"{a['role']}: {a['name']} {p.stat().st_size} bytes sha256 {digest}")
        if a.get("digest"):
            algo, _, value = a["digest"].partition(":")
            if algo != "sha256":
                raise Fail(f"{a['name']}: digest of unknown kind {a['digest']!r}")
            if value != digest:
                raise Fail(f"{a['name']}: SHA-256 {digest} differs from the release's digest {value}")
            print(f"  equals the release listing's digest")
        else:
            print(f"  the release listing gives no digest for this asset")
        if block:
            want = a.get("sha256")
            if digest != want:
                raise Fail(f"{a['name']}: SHA-256 {digest} differs from deps.lock.json {want}")
            print(f"  equals deps.lock.json")
        computed.append({"name": a["name"], "sha256": digest})
    if not block:
        record = {"version": selection["version"], "assets": computed}
        lock["mull"] = record
        pathlib.Path(lock_path).write_text(json.dumps(lock, indent=2) + "\n", encoding="utf-8")
        print("deps.lock.json had no mull block: recorded in this checkout only; copy the next line's JSON into it")
        print("MULL_RECORD " + json.dumps(record))
    if github_output:
        with open(github_output, "a", encoding="utf-8") as out:
            out.write(f"version={selection['version']}\n")
            for a, c in zip(selection["assets"], computed):
                out.write(f"{a['role']}={c['name']}\n{a['role']}_sha256={c['sha256']}\n")
    return 0


def self_test():
    import tempfile
    failures = []

    def expect_fail(name, fn):
        try:
            fn(); failures.append(f"{name}: no failure")
        except Fail:
            pass

    def asset(name, digest=None):
        return {"name": name, "browser_download_url": f"https://example.invalid/{name}", "digest": digest}

    listing = [
        {"tag_name": "0.38.0", "prerelease": True, "assets": [asset("Mull-0.38.0-ubuntu-amd64-24.04.deb"), asset("Mull-19-0.38.0-LLVM-19.1-ubuntu-amd64-24.04.deb")]},
        {"tag_name": "0.37.0", "assets": [asset("Mull-0.37.0-ubuntu-amd64-24.04.deb"), asset("Mull-18-0.37.0-LLVM-18.1-ubuntu-amd64-24.04.deb")]},
        {"tag_name": "0.36.1", "assets": [asset("Mull-0.36.1-ubuntu-amd64-22.04.deb"), asset("Mull-Extras-0.36.1-ubuntu-amd64-24.04.deb"),
                                          asset("Mull-0.36.1-ubuntu-amd64-24.04.deb"), asset("Mull-19-0.36.1-LLVM-19.1-ubuntu-amd64-24.04.deb"),
                                          asset("Mull-19-0.36.1-LLVM-19.1-ubuntu-arm64-24.04.deb")]},
        {"tag_name": "0.36.0", "assets": [asset("Mull-0.36.0-ubuntu-amd64-24.04.deb"), asset("Mull-19-0.36.0-ubuntu-amd64-24.04.deb")]},
    ]
    sel = resolve(listing, None)
    if (sel["version"], [a["name"] for a in sel["assets"]]) != ("0.36.1", ["Mull-0.36.1-ubuntu-amd64-24.04.deb", "Mull-19-0.36.1-LLVM-19.1-ubuntu-amd64-24.04.deb"]):
        failures.append(f"newest complete release: got {sel['version']} {sel['assets']}")
    loose = resolve([{"tag_name": "v0.40.0", "assets": [asset("Mull-0.40.0-ubuntu-24.04-x86_64.deb"), asset("Mull-19-0.40.0-ubuntu-24.04-x86_64.deb")]}], None)
    if loose["how"] != "Ubuntu 24.04 / amd64 / role facts":
        failures.append(f"loose match: {loose['how']}")
    expect_fail("no match", lambda: resolve(listing[:2], None))
    expect_fail("ambiguous", lambda: resolve([{"tag_name": "1.0.0", "assets": [asset("Mull-1.0.0-ubuntu-amd64-24.04.deb"),
                 asset("Mull-19-1.0.0-a-ubuntu-amd64-24.04.deb"), asset("Mull-19-1.0.0-b-ubuntu-amd64-24.04.deb")]}], None))

    with tempfile.TemporaryDirectory() as tmp:
        tmp = pathlib.Path(tmp)
        (tmp / "debs").mkdir()
        runner, plugin = b"runner package bytes", b"plugin package bytes"
        (tmp / "debs" / "Mull-0.36.1-ubuntu-amd64-24.04.deb").write_bytes(runner)
        (tmp / "debs" / "Mull-19-0.36.1-LLVM-19.1-ubuntu-amd64-24.04.deb").write_bytes(plugin)
        hr, hp = hashlib.sha256(runner).hexdigest(), hashlib.sha256(plugin).hexdigest()
        lock = tmp / "deps.lock.json"
        lock.write_text(json.dumps({"juce": {"version": "x"}}))
        # state 1: no block, digests given and right -> record written
        listing[2]["assets"][2]["digest"] = "sha256:" + hr
        listing[2]["assets"][3]["digest"] = "sha256:" + hp
        out = tmp / "gh_output"
        verify(resolve(listing, None), tmp / "debs", lock, out)
        rec = json.loads(lock.read_text())["mull"]
        if rec != {"version": "0.36.1", "assets": [{"name": "Mull-0.36.1-ubuntu-amd64-24.04.deb", "sha256": hr},
                                                   {"name": "Mull-19-0.36.1-LLVM-19.1-ubuntu-amd64-24.04.deb", "sha256": hp}]}:
            failures.append(f"recorded block: {rec}")
        if f"plugin_sha256={hp}" not in out.read_text():
            failures.append("github output lacks the plug-in hash")
        # state 2: block present -> exactly those assets, hashes verified
        sel = resolve(listing, lock_block(json.loads(lock.read_text())))
        if [a["name"] for a in sel["assets"]] != [r["name"] for r in rec["assets"]]:
            failures.append("lock state did not select the recorded names")
        verify(sel, tmp / "debs", lock)
        (tmp / "debs" / "Mull-0.36.1-ubuntu-amd64-24.04.deb").write_bytes(b"tampered")
        expect_fail("hash mismatch with lock", lambda: verify(sel, tmp / "debs", lock))
        # wrong digest with no lock block
        lock.write_text(json.dumps({}))
        expect_fail("digest mismatch", lambda: verify(resolve(listing, None), tmp / "debs", lock))
        expect_fail("missing file", lambda: verify(resolve(listing, None), tmp / "nothing", lock))
        expect_fail("tag not in listing", lambda: resolve(listing, {"version": "9.9.9", "assets": rec["assets"]}))
        expect_fail("malformed block", lambda: lock_block({"mull": {"version": "1", "assets": []}}))
    for f in failures:
        print("FAIL:", f)
    if failures:
        return 1
    print("mull_release self-test: all checks passed")
    return 0


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("lock-tag"); p.add_argument("--lock", default=str(ROOT / "deps.lock.json"))
    p = sub.add_parser("resolve"); p.add_argument("--listing", required=True); p.add_argument("--lock", default=str(ROOT / "deps.lock.json")); p.add_argument("--out", required=True)
    p = sub.add_parser("verify"); p.add_argument("--selection", required=True); p.add_argument("--dir", required=True)
    p.add_argument("--lock", default=str(ROOT / "deps.lock.json")); p.add_argument("--github-output")
    sub.add_parser("self-test")
    a = ap.parse_args(argv)
    try:
        if a.cmd == "self-test":
            return self_test()
        if a.cmd == "lock-tag":
            block = lock_block(load_lock(a.lock))
            if not block:
                print("deps.lock.json has no mull block", file=sys.stderr)
                return 1
            print(block["version"])
            return 0
        if a.cmd == "resolve":
            sel = resolve(json.loads(pathlib.Path(a.listing).read_text()), lock_block(load_lock(a.lock)))
            print(f"release {sel['version']} chosen by {sel['how']}; its assets: {sel['all_asset_names']}")
            for asset in sel["assets"]:
                print(f"  {asset['role']}: {asset['name']} digest {asset['digest'] or 'none'}")
            pathlib.Path(a.out).write_text(json.dumps(sel, indent=2) + "\n")
            return 0
        sel = json.loads(pathlib.Path(a.selection).read_text())
        return verify(sel, a.dir, a.lock, a.github_output)
    except Fail as e:
        print(f"FAIL: {e}", file=sys.stderr)
        return 2 if a.cmd == "lock-tag" else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
