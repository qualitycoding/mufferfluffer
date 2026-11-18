#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Generates the VCV Rack front panels and the panel layout header from tests/fixtures/effects.json.

usage: gen_panels.py [--check]

Writes rack/res/<Enum>.svg (seven files) and rack/src/generated_layout.hpp. With --check nothing is written:
the script exits 1 if a committed file differs from the regenerated one (or is missing), and 0 otherwise.

Every label is drawn as <path> elements from the single-stroke letter table below: no <text>, no font, no style
element (Rack's SVG renderer supports neither fonts nor style sheets). The layout rule is the same for every effect
(D-006): positions are centres in millimetres from the panel's top-left corner.
"""
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "tests" / "fixtures" / "effects.json"
PANEL_HEIGHT = 128.5
HP_MM = 5.08
HP_WIDTH = {"Lowpass": 12, "RingMod": 12, "Phaser": 12, "OscBox": 12, "Delay": 16, "Chorus": 16, "FilterBank": 20}

TITLE_HEIGHT = 4.0
TITLE_X = 3.0
TITLE_BASELINE = 9.0
LIGHT_Y = 14.0
CONTROL_ROWS = (26.0, 42.0, 58.0, 74.0)
CONTROL_X0, CONTROL_PITCH = 7.62, 15.24
CONTROL_LABEL_DY = 9.0
PORT_ROWS = (95.0, 108.0, 121.0)
PORT_X0, PORT_PITCH = 5.08, 10.16
PORT_LABEL_DY = -6.5
OUTPUT_SQUARE = 9.0
LABEL_START, LABEL_MIN, LABEL_MIN_TWO_LINES, LABEL_STEP = 2.0, 1.2, 0.9, 0.1
LABEL_MARGIN = 1.0
LINE_GAP = 1.4          # in letter heights
ADVANCE = 0.7           # glyph advance in letter heights, the space included

# Single-stroke letters on a grid 3 wide and 6 high (y upwards from the baseline). Each glyph is a list of polylines.
_O = [(1, 0), (0, 1), (0, 5), (1, 6), (2, 6), (3, 5), (3, 1), (2, 0), (1, 0)]
_P = [(0, 0), (0, 6), (2, 6), (3, 5), (3, 4), (2, 3), (0, 3)]
GLYPHS = {
    "A": [[(0, 0), (0, 4), (1.5, 6), (3, 4), (3, 0)], [(0, 3), (3, 3)]],
    "B": [[(0, 0), (0, 6), (2, 6), (3, 5), (3, 4), (2, 3), (0, 3)], [(2, 3), (3, 2), (3, 1), (2, 0), (0, 0)]],
    "C": [[(3, 5), (2, 6), (1, 6), (0, 5), (0, 1), (1, 0), (2, 0), (3, 1)]],
    "D": [[(0, 0), (0, 6), (1.5, 6), (3, 4.5), (3, 1.5), (1.5, 0), (0, 0)]],
    "E": [[(3, 6), (0, 6), (0, 0), (3, 0)], [(0, 3), (2, 3)]],
    "F": [[(3, 6), (0, 6), (0, 0)], [(0, 3), (2, 3)]],
    "G": [[(3, 5), (2, 6), (1, 6), (0, 5), (0, 1), (1, 0), (2, 0), (3, 1), (3, 3), (1.5, 3)]],
    "H": [[(0, 0), (0, 6)], [(3, 0), (3, 6)], [(0, 3), (3, 3)]],
    "I": [[(0.5, 6), (2.5, 6)], [(1.5, 6), (1.5, 0)], [(0.5, 0), (2.5, 0)]],
    "J": [[(3, 6), (3, 1), (2, 0), (1, 0), (0, 1)]],
    "K": [[(0, 0), (0, 6)], [(3, 6), (0, 3), (3, 0)]],
    "L": [[(0, 6), (0, 0), (3, 0)]],
    "M": [[(0, 0), (0, 6), (1.5, 3), (3, 6), (3, 0)]],
    "N": [[(0, 0), (0, 6), (3, 0), (3, 6)]],
    "O": [_O],
    "P": [_P],
    "Q": [_O, [(2, 1.5), (3, 0)]],
    "R": [_P, [(1.5, 3), (3, 0)]],
    "S": [[(3, 5), (2, 6), (1, 6), (0, 5), (0, 4), (1, 3), (2, 3), (3, 2), (3, 1), (2, 0), (1, 0), (0, 1)]],
    "T": [[(0, 6), (3, 6)], [(1.5, 6), (1.5, 0)]],
    "U": [[(0, 6), (0, 1), (1, 0), (2, 0), (3, 1), (3, 6)]],
    "V": [[(0, 6), (1.5, 0), (3, 6)]],
    "W": [[(0, 6), (0.75, 0), (1.5, 4), (2.25, 0), (3, 6)]],
    "X": [[(0, 6), (3, 0)], [(0, 0), (3, 6)]],
    "Y": [[(0, 6), (1.5, 3), (3, 6)], [(1.5, 3), (1.5, 0)]],
    "Z": [[(0, 6), (3, 6), (0, 0), (3, 0)]],
    "0": [_O],
    "1": [[(0.5, 5), (1.5, 6), (1.5, 0)], [(0.5, 0), (2.5, 0)]],
    "2": [[(0, 5), (1, 6), (2, 6), (3, 5), (3, 4), (0, 0), (3, 0)]],
    "3": [[(0, 5), (1, 6), (2, 6), (3, 5), (3, 4), (2, 3), (1, 3)],
          [(2, 3), (3, 2), (3, 1), (2, 0), (1, 0), (0, 1)]],
    "4": [[(2.5, 0), (2.5, 6), (0, 2), (3, 2)]],
    "5": [[(3, 6), (0, 6), (0, 3), (2, 3), (3, 2), (3, 1), (2, 0), (1, 0), (0, 1)]],
    "6": [[(3, 5), (2, 6), (1, 6), (0, 5), (0, 1), (1, 0), (2, 0), (3, 1), (3, 2), (2, 3), (0, 3)]],
    "7": [[(0, 6), (3, 6), (1, 0)]],
    "8": [[(1, 3), (0, 4), (0, 5), (1, 6), (2, 6), (3, 5), (3, 4), (2, 3), (1, 3), (0, 2), (0, 1), (1, 0), (2, 0),
           (3, 1), (3, 2), (2, 3)]],
    "9": [[(0, 1), (1, 0), (2, 0), (3, 1), (3, 5), (2, 6), (1, 6), (0, 5), (0, 4), (1, 3), (3, 3)]],
    "/": [[(0, 0), (3, 6)]],
    "-": [[(0.5, 3), (2.5, 3)]],
    "+": [[(0.5, 3), (2.5, 3)], [(1.5, 1.5), (1.5, 4.5)]],
    ".": [[(1.3, 0), (1.7, 0), (1.7, 0.4), (1.3, 0.4), (1.3, 0)]],
    " ": [],
}
GLYPH_GRID_H = 6.0

BACKGROUND = "#2e3035"
OUTPUT_FILL = "#1b1c1f"
INK = "#e9e6dc"
RING = "#6f737b"


class LayoutError(Exception):
    pass


def num(x):
    s = f"{x:.3f}".rstrip("0").rstrip(".")
    return "0" if s in ("-0", "") else s


def flt(x):
    """A C++ float literal: always has a decimal point."""
    s = num(x)
    return (s if "." in s else s + ".0") + "f"


def tenths(h):
    return round(h * 10)


def label_width(text, height):
    return len(text) * ADVANCE * height


def fit_lines(text, pitch):
    """Returns (lines, height) following the shrink rule of the plan, or raises LayoutError."""
    text = text.upper()
    limit = pitch - LABEL_MARGIN

    def shrink(line, lowest):
        t = tenths(LABEL_START)
        while t >= tenths(lowest):
            if label_width(line, t / 10) <= limit + 1e-9:
                return t / 10
            t -= tenths(LABEL_STEP)
        return None

    h = shrink(text, LABEL_MIN)
    if h is not None:
        return [text], h
    spaces = [i for i, c in enumerate(text) if c == " "]
    if spaces:
        mid = len(text) / 2
        cut = min(spaces, key=lambda i: (abs(i - mid), i))
        lines = [text[:cut].strip(), text[cut + 1:].strip()]
        longest = max(lines, key=len)
        h = shrink(longest, LABEL_MIN_TWO_LINES)
        if h is not None:
            return lines, h
    raise LayoutError(f"label {text!r} does not fit a column pitch of {pitch} mm")


def stroke_path(text, centre_x, centre_y, height, stroke_width):
    """The path data of one line of text centred on (centre_x, centre_y)."""
    s = height / GLYPH_GRID_H
    total = label_width(text, height)
    x0 = centre_x - total / 2
    baseline = centre_y + height / 2
    parts = []
    for i, ch in enumerate(text):
        gx = x0 + i * ADVANCE * height
        for line in GLYPHS[ch]:
            pts = [(gx + px * s, baseline - py * s) for px, py in line]
            parts.append("M" + " L".join(f"{num(x)} {num(y)}" for x, y in pts))
    return " ".join(parts)


def text_elements(lines, height, centre_x, centre_y, direction):
    """Path elements for a label of one or two lines. Direction +1 (controls): the label grows downwards from
    centre_y, so the second line is the one further from the control. Direction -1 (ports): the label grows upwards,
    the last line sits at centre_y, and the first line is the upper one, so that the label reads from top to bottom."""
    sw = max(0.12, round(height * 0.11, 2))
    out = []
    for n, line in enumerate(lines):
        offset = n if direction > 0 else n - (len(lines) - 1)
        cy = centre_y + offset * LINE_GAP * height
        d = stroke_path(line, centre_x, cy, height, sw)
        if d:
            out.append(f'<path d="{d}" fill="none" stroke="{INK}" stroke-width="{num(sw)}" '
                       f'stroke-linecap="round" stroke-linejoin="round"/>')
    return out


def effect_layout(e):
    enum = e["enum"]
    hp = HP_WIDTH[enum]
    width = hp * HP_MM
    controls, ports = [], []
    cols = hp // 3
    for k, p in enumerate(e["params"]):
        row, col = divmod(k, cols)
        if row >= len(CONTROL_ROWS):
            raise LayoutError(f"{enum}: control row {row} would pass y = {CONTROL_ROWS[-1]}")
        controls.append((CONTROL_X0 + CONTROL_PITCH * col, CONTROL_ROWS[row], p["name"]))
    pcols = hp // 2
    entries = [(q["name"], False) for q in e["inputs"]] + [(q["name"], True) for q in e["outputs"]]
    for k, (name, is_out) in enumerate(entries):
        row, col = divmod(k, pcols)
        if row >= len(PORT_ROWS):
            raise LayoutError(f"{enum}: port row {row} would pass y = {PORT_ROWS[-1]}")
        ports.append((PORT_X0 + PORT_PITCH * col, PORT_ROWS[row], name, is_out))
    lights = [(width - HP_MM - HP_MM * k, LIGHT_Y) for k in range(len(e["meters"]))]
    return hp, width, controls, ports, lights


def title_fits(name, width):
    return TITLE_X + label_width(name.upper(), TITLE_HEIGHT) <= width - TITLE_X


def panel_svg(e):
    hp, width, controls, ports, lights = effect_layout(e)
    if not title_fits(e["name"], width):
        raise LayoutError(f"{e['enum']}: title does not fit the panel")
    w, h = num(width), num(PANEL_HEIGHT)
    out = ['<?xml version="1.0" encoding="UTF-8"?>',
           f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}mm" height="{h}mm" viewBox="0 0 {w} {h}">',
           f'<rect x="0" y="0" width="{w}" height="{h}" fill="{BACKGROUND}"/>']
    # title: letters 4 mm high, left edge at x = 3, baseline at y = 9
    title = e["name"].upper()
    centre_x = TITLE_X + label_width(title, TITLE_HEIGHT) / 2
    out += text_elements([title], TITLE_HEIGHT, centre_x, TITLE_BASELINE - TITLE_HEIGHT / 2, 1)
    for x, y in lights:
        out.append(f'<circle cx="{num(x)}" cy="{num(y)}" r="1.6" fill="none" stroke="{RING}" stroke-width="0.25"/>')
    for x, y, name in controls:
        out.append(f'<circle cx="{num(x)}" cy="{num(y)}" r="5.5" fill="none" stroke="{RING}" stroke-width="0.3"/>')
        lines, height = fit_lines(name, CONTROL_PITCH)
        out += text_elements(lines, height, x, y + CONTROL_LABEL_DY, 1)
    for x, y, name, is_out in ports:
        if is_out:
            half = OUTPUT_SQUARE / 2
            out.append(f'<rect x="{num(x - half)}" y="{num(y - half)}" width="{num(OUTPUT_SQUARE)}" '
                       f'height="{num(OUTPUT_SQUARE)}" fill="{OUTPUT_FILL}"/>')
        out.append(f'<circle cx="{num(x)}" cy="{num(y)}" r="4.2" fill="none" stroke="{RING}" stroke-width="0.3"/>')
        lines, height = fit_lines(name, PORT_PITCH)
        out += text_elements(lines, height, x, y + PORT_LABEL_DY, -1)
    out.append("</svg>")
    return "\n".join(out) + "\n"


def layout_header(effects):
    def arr(name, pts):
        body = ", ".join(f"{{{flt(x)}, {flt(y)}}}" for x, y in pts)
        return f"inline constexpr Vec2 {name}[] = {{{body}}};"

    out = ["// Generated by tools/gen_panels.py from tests/fixtures/effects.json. Do not edit by hand.",
           "// Positions are centres in millimetres from the panel's top-left corner, in table order:",
           "// inputs first, then outputs, for the ports.",
           "#pragma once", "", "namespace muff {", "namespace layout {", "",
           "struct Vec2 { float x; float y; };", "",
           "struct Panel {",
           "    int hp;",
           "    float widthMm;",
           "    int numParams;  const Vec2* params;",
           "    int numInputs;  const Vec2* inputs;",
           "    int numOutputs; const Vec2* outputs;",
           "    int numLights;  const Vec2* lights;",
           "};", ""]
    entries = []
    for e in effects:
        hp, width, controls, ports, lights = effect_layout(e)
        n = e["enum"]
        ins = [(x, y) for x, y, _, o in ports if not o]
        outs = [(x, y) for x, y, _, o in ports if o]
        out.append(arr(f"k{n}Params", [(x, y) for x, y, _ in controls]))
        out.append(arr(f"k{n}Inputs", ins))
        out.append(arr(f"k{n}Outputs", outs))
        out.append(arr(f"k{n}Lights", lights))
        out.append("")
        entries.append(f"    {{{hp}, {num(width)}f, {len(controls)}, k{n}Params, {len(ins)}, k{n}Inputs, "
                       f"{len(outs)}, k{n}Outputs, {len(lights)}, k{n}Lights}},  // {n}")
    out.append(f"// One entry per effect, in the order of muff::EffectId.")
    out.append(f"inline constexpr Panel kPanels[{len(effects)}] = {{")
    out += entries
    out += ["};", "", "}  // namespace layout", "}  // namespace muff", ""]
    return "\n".join(out)


def main(argv):
    check = "--check" in argv
    effects = json.loads(FIXTURE.read_text(encoding="utf-8"))["effects"]
    files = {}
    try:
        for e in effects:
            files[f"rack/res/{e['enum']}.svg"] = panel_svg(e)
        files["rack/src/generated_layout.hpp"] = layout_header(effects)
    except LayoutError as err:
        print(f"gen_panels: {err}", file=sys.stderr)
        return 2
    stale = []
    for rel, text in files.items():
        path = ROOT / rel
        if check:
            if not path.is_file() or path.read_bytes() != text.encode("utf-8"):
                stale.append(rel)
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(text.encode("utf-8"))
    if check:
        if stale:
            print("gen_panels: out of date: " + ", ".join(stale), file=sys.stderr)
            return 1
        print(f"panels up to date: {len(files)} files")
    else:
        print(f"wrote {len(files)} files")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
