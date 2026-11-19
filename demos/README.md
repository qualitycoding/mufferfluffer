# Listening demos

This directory holds the 23 event files of the listening demos (step S-LISTEN, gate G-LISTEN). Each file
`demo-NN-<name>.txt` lists the parameter changes of one demo, one per line: `<seconds> <parameter id> <value>`, in the
units of `tests/fixtures/effects.json`. They are generated from the demo table in `tools/render_demos.py`, where the
settings are written in musical units (Hz, ms, step names, pattern numbers); do not edit them by hand.

No audio is kept on this branch. The rendered demos are published by the `evidence` workflow on the branch
`evidence/G-LISTEN`, together with `INDEX.md` (the effect and settings of each demo, the peak of its processed section
before normalisation and the gain applied) and `SHA256SUMS`.

## Regenerating

After changing the demo table:

    python3 tools/render_demos.py --self-test
    python3 tools/render_demos.py --write-events

and commit the changed event files with the table.

## Rendering locally

    cmake -S . -B build/tools -DCMAKE_BUILD_TYPE=Release -DMUFF_BUILD_TESTS=OFF -DMUFF_BUILD_TOOLS=ON
    cmake --build build/tools --target muff_render
    python3 tools/render_demos.py --self-test --muff-render build/tools/muff_render
    python3 tools/make_demo_inputs.py /tmp/muff-demos/inputs
    python3 tools/render_demos.py --muff-render build/tools/muff_render --inputs /tmp/muff-demos/inputs --out /tmp/muff-demos/bundle

Keep the output outside the repository: WAV and float files are never committed. Rendering is deterministic, so two
runs give the same `SHA256SUMS`. The script stops with an error if a processed section peaks below -40 dBFS or above
+12.1 dBFS, and marks a peak above +9 dBFS as "loud".

## Listening

Use headphones. Every demo starts with three seconds of the unprocessed input, then the same material through the
effect, and each file is normalised with a single gain, so the level change between the two parts is the effect's own.
Demos 09, 16, 21 and 22 are stereo (the effect's two outputs on the left and right); all others carry what a mono host
hears, on both sides. `INDEX.md` in the bundle gives each demo's settings.
