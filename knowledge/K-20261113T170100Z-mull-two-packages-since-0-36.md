---
id: K-20261113T170100Z-mull-two-packages-since-0-36
statement: "Since Mull 0.36.0 the runner and reporter are in one Debian package (file name Mull-<version>-<os>-<arch>-<os version>.deb) and each LLVM major's compiler plug-in is in another (Mull-<llvm major>-<version>-...deb) that depends on the first; both must be installed together, for example with apt-get install ./a.deb ./b.deb."
status: current
supersedes: [K-20261112T120000Z-c-planner-012, K-20261027T123200Z-mull-mutation-testing]
tags: [tech:mull, tech:github-actions, subject:mufferfluffer-build-and-ci, phase:research]
applies_to_version: "Mull 0.36.0 to 0.38.0"
source: {"url": "https://github.com/mull-project/mull/blob/0.38.0/docs/Installation.rst", "doi": "", "title": "Mull docs/Installation.rst and mull_package.bzl at tag 0.38.0", "locator": "Installation.rst section on GitHub releases; mull_package.bzl package definitions", "accessed": "2026-11-13", "tier": 1}
confidence: single-source
derived_from: []
discovered_by: assumption audit (fresh-context reviewer), confirmed by planner
run: gen-20261111T151412Z-mufferfluffer-vst-vcv
recorded_at: 2026-11-13T17:00:00Z
---
Checked by: the auditor read the files at tag 0.38.0 (and the dependency at 0.37.0); not installed in planning (release assets unreachable from the planning sandbox). File names place the architecture between the system name and its version (for example ...-ubuntu-amd64-24.04.deb). Packages named Mull-Extras-... do not contain the runner. An install recipe written for one .deb per LLVM version fails on the unmet dependency.
