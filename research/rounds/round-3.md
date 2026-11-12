# Round 3 (depth: remaining figures; adversarial pass)

## Filter-array figures (`round3` strand, claims C-R3-nnn)
Six figures that round 2 could not re-read were checked against the manual again (11 fetches):
- Confirmed: Mids centres (second publisher for the end points only); Bass voicing (second publisher for the 110 Hz low-pass and the span); 24 patterns in two banks with patterns 1 and 13 static; odd/even stereo split, mono summing and left-only bypass; the Envelope control's landmarks at 0, about 2, about 5, about 6 and about 8.
- Not found anywhere: the free-running Rate range. D-024 keeps 0.5 to 32 steps per second as a design choice (D-027 item 8).
- Wording differences worth recording: the manual calls filters 2 to 8 of the Bass voicing "resonant" rather than band-pass (D-024 models them as band-pass; listening question L-5 covers the filter bank); the manual gives no description of the top Envelope setting (the table's value at 10 is a design choice); the two passes paraphrase the setting near 8 differently (one says a smooth attack with an abrupt end, the other a fast attack with an abrupt end); "sounds backwards" implies the slow attack, which D-024's table uses.
- One review states that patterns 1 and 12 are static, contradicting the manual's 1 and 13; the manual is followed.

## Adversarial pass (R5)
A fresh-context agent of Opus tier attacked the central decisions. Its report is `research/rounds/R5-adversarial.md`; the disposition of every Critical and High item is in the section "Disposition" appended to that file by the planner.

## Termination check (protocol R6/R7 criteria)
- Minimum of three rounds: done.
- Every leaf of the question tree is answered or closed by a decision (`research/QUESTIONS.md`, status column).
- Load-bearing claims that remain `single-source` or `unverified` after three rounds are carried as risks: R-001 (manual figures, one publisher), R-003 (patterns), R-004 (no references), R-005 (undocumented constants), R-006 and R-007 (aliasing methods), R-010 (Mull), R-011 (pluginval), R-014 (paper licences).
- Spikes: build (`juce-vst3-build`) and satisfiability (`red-verification`, `oracle`, `oracle_vst3`, `oracle_repo`).
