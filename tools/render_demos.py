# SPDX-License-Identifier: AGPL-3.0-only
"""Renders the 23 listening demos of S-LISTEN (gate G-LISTEN, D-040).

Usage:
  python3 tools/render_demos.py --write-events
      writes demos/demo-NN-<name>.txt from the demo table below (those files are committed; never edit them by hand)
  python3 tools/render_demos.py --muff-render build/muff_render --inputs DIR --out DIR
      renders every demo into DIR/demo-NN-<name>.wav and writes DIR/SHA256SUMS and DIR/INDEX.md;
      DIR given to --inputs holds the dry inputs written by tools/make_demo_inputs.py
  python3 tools/render_demos.py --self-test [--muff-render build/muff_render]
      checks the conversions from musical units to the fixture's units on known points; with --muff-render it also
      measures the converted settings through the effects themselves

The table holds every demo in musical units (Hz, ms, a step's name, a pattern number). Event files need the
fixture's units (tests/fixtures/effects.json), so the script converts: a frequency or time on a knob that has no unit of
its own is turned into the knob position by inverting the formula of the effect's decision (D-021 carrier frequency,
D-022 LFO rate, D-023 and D-026 delay time); a parameter whose fixture unit is Hz takes the frequency as it is; a step
name becomes its index in the fixture's step list; pattern p becomes p - 1.

Each demo is 3 s of its dry input followed by the processed signal, as a 48 kHz 24-bit stereo WAV peak-normalised to
-3 dBFS with one gain for the whole file. INDEX.md states each processed section's peak before normalisation
(0 dBFS = 1.0 in core units) and the gain applied. The script fails if a processed section peaks below -40 dBFS or
above +12.1 dBFS (the output limit of D-012 makes that impossible in a correct build), and warns about and marks as
"loud" a peak above +9 dBFS (expected for demos 03, 12 and 23). It also fails unless the mono rendering of demo 14
differs from the left channel of its stereo rendering by at least 10 % in RMS (the right-hand filters are in the
mono demo).
"""
import argparse
import array
import hashlib
import json
import math
import pathlib
import subprocess
import sys
import tempfile
import wave

ROOT = pathlib.Path(__file__).resolve().parents[1]
FIXTURE_PATH = ROOT / "tests" / "fixtures" / "effects.json"
DEMOS_DIR = ROOT / "demos"

SAMPLE_RATE = 48000
DRY_SECONDS = 3.0
SWEEP_STEP_SECONDS = 0.01        # a sweep is one event every 10 ms
ONE_NOTE_SECONDS = 3.0           # "one plucked note": the first 3 s of plucked tones, then silence
TARGET_PEAK_DBFS = -3.0
FAIL_BELOW_DBFS = -40.0
FAIL_ABOVE_DBFS = 12.1           # D-012: every output is within +/-4.0 (+12.04 dBFS)
LOUD_ABOVE_DBFS = 9.0
EXPECTED_LOUD = (3, 12, 23)
MONO_CHECK_DEMO = 14
MONO_CHECK_MIN_RATIO = 0.10
MIN_TOTAL_SECONDS, MAX_TOTAL_SECONDS = 12.0, 25.0
FULL_SCALE_24 = 8388608
DRY_INPUTS = ("plucked_tones", "plucked_chords", "saw_bass", "drum_pattern", "held_chord")


# ---------------------------------------------------------------------------------------------------------------------
# Musical units

class Hz:
    """A frequency in hertz. Sweeps of a frequency move exponentially."""
    def __init__(self, value):
        self.value = float(value)

    def __str__(self):
        return f"{self.value:g} Hz"


class Ms:
    """A time in milliseconds. Sweeps of a time move linearly."""
    def __init__(self, value):
        self.value = float(value)

    def __str__(self):
        return f"{self.value:g} ms"


class Pattern:
    """A filter-bank pattern number, 1 to 12."""
    def __init__(self, number):
        self.number = int(number)

    def __str__(self):
        return str(self.number)


def describe_value(value):
    return f"{value:g}" if isinstance(value, (int, float)) else str(value)


# ---------------------------------------------------------------------------------------------------------------------
# Conversions: the forward laws of the decisions and their inverses. `steps` maps a stepped parameter id to the step
# index in force when the value applies (the range switches decide which law holds).

RING_RANGES_HZ = ((0.6, 80.0), (30.0, 4000.0))          # D-021: base Lo 0.6 (80/0.6)^n, Hi 30 (4000/30)^n
PHASER_LFO_BASE_HZ = (0.01, 1.0)                         # D-022: rate 0.01 x 250^n (Lo) or 1 x 250^n (Hi)
PHASER_LFO_SPAN = 250.0
DELAY_LOW_MS = (40.0, 80.0)                              # D-023: d = lo x 10^n, lo 40 ms (Short) or 80 ms (Long)
DELAY_SPAN = 10.0
CHORUS_RANGES_MS = ((0.6, 10.0), (5.0, 50.0))            # D-026: d = lo (hi/lo)^time, Flange 0.6-10 ms, Chorus 5-50 ms


def ring_frequency_knob(hz, steps):
    lo, hi = RING_RANGES_HZ[steps["range"]]
    return math.log(hz / lo) / math.log(hi / lo)


def phaser_rate_knob(hz, steps):
    return math.log(hz / PHASER_LFO_BASE_HZ[steps["lfo_range"]]) / math.log(PHASER_LFO_SPAN)


def delay_time_knob(ms, steps):
    return math.log10(ms / DELAY_LOW_MS[steps["range"]]) / math.log10(DELAY_SPAN)


def chorus_time_knob(ms, steps):
    lo, hi = CHORUS_RANGES_MS[steps["range"]]
    return math.log(ms / lo) / math.log(hi / lo)


# (effect slug, parameter id) -> (musical unit class, inverse law)
KNOB_LAWS = {
    ("ringmod", "frequency"): (Hz, ring_frequency_knob),
    ("phaser", "lfo_rate"): (Hz, phaser_rate_knob),
    ("delay", "time"): (Ms, delay_time_knob),
    ("chorus", "time"): (Ms, chorus_time_knob),
}


# ---------------------------------------------------------------------------------------------------------------------
# The demo table

class Fixed:
    """A setting held for the whole processed section."""
    def __init__(self, param, value):
        self.param, self.value = param, value

    def events(self, seconds):
        return [(0.0, self.param, self.value)]

    def describe(self, seconds):
        return f"{self.param} {describe_value(self.value)}"


class Then:
    """`A then B [then C]`: the processed time is cut into equal parts (two values: the switch is at half time)."""
    def __init__(self, param, *values):
        self.param, self.values = param, values

    def events(self, seconds):
        part = seconds / len(self.values)
        return [(round(k * part, 3), self.param, v) for k, v in enumerate(self.values)]

    def describe(self, seconds):
        return f"{self.param} " + " then ".join(describe_value(v) for v in self.values)


class Each:
    """A list of values, `for N s each`, starting at `start`."""
    def __init__(self, param, values, every, start=0.0):
        self.param, self.values, self.every, self.start = param, values, every, start

    def events(self, seconds):
        return [(round(self.start + k * self.every, 3), self.param, v) for k, v in enumerate(self.values)]

    def describe(self, seconds):
        where = f" from {self.start:g} s" if self.start else ""
        return f"{self.param} " + ", ".join(describe_value(v) for v in self.values) + f" for {self.every:g} s each{where}"


class At:
    """A single change at a given time."""
    def __init__(self, time, param, value):
        self.time, self.param, self.value = time, param, value

    def events(self, seconds):
        return [(self.time, self.param, self.value)]

    def describe(self, seconds):
        return f"{self.param} {describe_value(self.value)} at {self.time:g} s"


class Sweep:
    """`A to B` from t0 to t1, one event every 10 ms: exponential for frequencies, linear otherwise."""
    def __init__(self, param, start, end, t0, t1):
        self.param, self.start, self.end, self.t0, self.t1 = param, start, end, t0, t1

    def events(self, seconds):
        count = int(round((self.t1 - self.t0) / SWEEP_STEP_SECONDS))
        out = []
        for k in range(count + 1):
            frac = k / count
            if isinstance(self.start, Hz):
                value = Hz(self.start.value * (self.end.value / self.start.value) ** frac)
            elif isinstance(self.start, Ms):
                value = Ms(self.start.value + (self.end.value - self.start.value) * frac)
            else:
                value = self.start + (self.end - self.start) * frac
            time = round(self.t0 + k * SWEEP_STEP_SECONDS, 3)
            if time < seconds:          # a sweep that ends with the demo has no use for an event at its very end
                out.append((time, self.param, value))
        return out

    def describe(self, seconds):
        return f"{self.param} {describe_value(self.start)} to {describe_value(self.end)} over {self.t0:g}-{self.t1:g} s"


class Demo:
    def __init__(self, number, name, slug, source, seconds, settings, stereo=False):
        self.number, self.name, self.slug, self.source = number, name, slug, source
        self.seconds, self.settings, self.stereo = seconds, settings, stereo

    @property
    def stem(self):
        return f"demo-{self.number:02d}-{self.name}"

    def source_text(self):
        names = self.source if isinstance(self.source, tuple) else (self.source,)
        return " then ".join(n.replace("_", " ") for n in names)


def smile(index):
    """Slider level of filter `index` (1 to 8) on a smile curve: 1.0 at both ends, about 0.31 in the middle."""
    return round(0.3 + 0.7 * ((index - 4.5) / 3.5) ** 2, 4)


DEMOS = [
    Demo(1, "lowpass-sweep", "lowpass", "saw_bass", 20.0, [
        Fixed("resonance", 5), Sweep("cutoff", Hz(60), Hz(8000), 0.0, 10.0), Sweep("cutoff", Hz(8000), Hz(60), 10.0, 20.0)]),
    Demo(2, "lowpass-envelope", "lowpass", "plucked_chords", 18.0, [
        Fixed("cutoff", Hz(150)), Fixed("env_amount", 4), Then("env_speed", "Fast", "Smooth"), Fixed("resonance", 6)]),
    Demo(3, "lowpass-selfosc", "lowpass", ("silence", "drum_pattern"), 20.0, [
        Fixed("resonance", 10), Each("cutoff", [Hz(110), Hz(220), Hz(440), Hz(880)], 2.5),
        At(10.0, "resonance", 9), Each("poles", ["2-pole", "4-pole"], 5.0, start=10.0)]),
    Demo(4, "ring-tremolo", "ringmod", "held_chord", 20.0, [
        Fixed("range", "Lo"), Sweep("frequency", Hz(4), Hz(30), 0.0, 20.0)]),
    Demo(5, "ring-bells", "ringmod", "plucked_tones", 18.0, [
        Fixed("range", "Hi"), Fixed("frequency", Hz(350)), Then("mix", 0.5, 1.0)]),
    Demo(6, "ring-lfo", "ringmod", "plucked_tones", 18.0, [
        Fixed("frequency", Hz(500)), Then("lfo_wave", "Sine", "Square"), Then("lfo_rate", Hz(0.5), Hz(6)),
        Then("lfo_amount", 1, 3)]),
    Demo(7, "phase-slow", "phaser", "held_chord", 20.0, [
        Then("stages", "6-stage", "12-stage"), Fixed("lfo_range", "Lo"), Fixed("lfo_rate", Hz(0.2)),
        Fixed("lfo_amount", 0.6), Fixed("resonance", 3)]),
    Demo(8, "phase-resonant", "phaser", "saw_bass", 18.0, [
        Fixed("stages", "12-stage"), Fixed("resonance", 9), Fixed("lfo_rate", Hz(0.6)), Fixed("lfo_amount", 0.8)]),
    Demo(9, "phase-fast", "phaser", "plucked_tones", 18.0, [
        Fixed("lfo_range", "Hi"), Then("lfo_rate", Hz(8), Hz(120)), Fixed("lfo_amount", 0.4)], stereo=True),
    Demo(10, "delay-short", "delay", "drum_pattern", 16.0, [
        Fixed("range", "Short"), Fixed("time", Ms(120)), Fixed("feedback", 4), Fixed("mix", 0.4)]),
    Demo(11, "delay-long-dark", "delay", "plucked_tones", 18.0, [
        Fixed("range", "Long"), Fixed("time", Ms(600)), Fixed("feedback", 6.5), Fixed("mix", 0.5)]),
    Demo(12, "delay-runaway", "delay", "one_plucked_note", 16.0, [
        Fixed("range", "Short"), Fixed("time", Ms(300)), Sweep("feedback", 7.5, 10.0, 0.0, 10.0),
        At(10.0, "feedback", 3), Sweep("time", Ms(300), Ms(150), 10.0, 12.0)]),
    Demo(13, "delay-lfo", "delay", "held_chord", 18.0, [
        Fixed("time", Ms(250)), Then("lfo_wave", "Sine", "Triangle", "Random step"), Then("lfo_rate", Hz(0.8), Hz(5), Hz(4)),
        Then("lfo_amount", 3, 2, 4)]),
    Demo(14, "bank-static", "filterbank", "drum_pattern", 16.0, [
        Then("voicing", "Mids", "Bass"), Fixed("pattern", Pattern(1))]
        + [Fixed(f"level{k}", smile(k)) for k in range(1, 9)]),
    Demo(15, "bank-patterns", "filterbank", "held_chord", 20.0, [
        Fixed("voicing", "Mids"), Each("pattern", [Pattern(2), Pattern(4), Pattern(9), Pattern(11)], 5.0),
        Fixed("rate", Hz(6)), Fixed("envelope", 2)]),
    Demo(16, "bank-envelopes", "filterbank", "held_chord", 20.0, [
        Fixed("pattern", Pattern(12)), Fixed("rate", Hz(3)), Each("envelope", [0, 2, 5, 6, 8], 4.0)], stereo=True),
    Demo(17, "bank-lfo", "filterbank", "saw_bass", 18.0, [
        Fixed("voicing", "Bass"), Fixed("pattern", Pattern(2)), Fixed("lfo", "On"), Fixed("sweep", 0.3)]),
    Demo(18, "osc-follow", "oscbox", "plucked_tones", 18.0, [
        Fixed("frequency", Hz(220)), Fixed("wave", 0.33), Fixed("mix", 0.6), Fixed("env_amount", 0)]),
    Demo(19, "osc-sync-sweep", "oscbox", "saw_bass", 20.0, [
        Fixed("sync", "On"), Sweep("frequency", Hz(100), Hz(1600), 0.0, 20.0), Fixed("wave", 0.66), Fixed("mix", 1)]),
    Demo(20, "osc-fm-env", "oscbox", "drum_pattern", 16.0, [
        Fixed("fm_amount", 0.7), Fixed("env_amount", 3), Fixed("frequency", Hz(80)), Fixed("wave", 0)]),
    Demo(21, "swarm-chorus", "chorus", "plucked_chords", 18.0, [
        Fixed("range", "Chorus"), Fixed("time", 0.4), Fixed("lfo_rate", Hz(0.6)), Fixed("lfo_amount", 0.3),
        Fixed("feedback", 0.2), Fixed("mix", 0.5)], stereo=True),
    Demo(22, "swarm-flange", "chorus", ("drum_pattern", "held_chord"), 20.0, [
        Fixed("range", "Flange"), Fixed("lfo_rate", Hz(0.15)), Fixed("lfo_amount", 0.9), Then("feedback", 0.7, -0.7),
        Fixed("mix", 0.5)], stereo=True),
    Demo(23, "drive", "lowpass", "plucked_chords", 20.0, [
        Fixed("cutoff", Hz(12000)), Fixed("resonance", 0), Fixed("mix", 1), Each("drive", [0, 9, 18, 28], 5.0)]),
]


# ---------------------------------------------------------------------------------------------------------------------
# From the table to event files

class TableError(Exception):
    pass


def load_fixture():
    data = json.loads(FIXTURE_PATH.read_text(encoding="utf-8"))
    return {e["slug"]: e for e in data["effects"]}


def to_fixture_value(effect, param, value, steps):
    """Converts one musical value to the fixture's unit for `param`, failing on anything out of range."""
    spec = next((p for p in effect["params"] if p["id"] == param), None)
    if spec is None:
        raise TableError(f"{effect['slug']} has no parameter {param}")
    law = KNOB_LAWS.get((effect["slug"], param))
    if spec["taper"] == "stepped":
        if param == "pattern" and isinstance(value, Pattern):
            result = value.number - 1
        elif isinstance(value, str) and value in spec["steps"]:
            result = spec["steps"].index(value)
        else:
            raise TableError(f"{effect['slug']}.{param}: {value!s} is not one of {spec['steps']}")
    elif law is not None and isinstance(value, law[0]):
        result = law[1](value.value, steps)
    elif isinstance(value, Hz):
        if spec["unit"] != "Hz":
            raise TableError(f"{effect['slug']}.{param} is not in Hz in the fixture")
        result = value.value
    elif isinstance(value, (int, float)):
        result = float(value)           # already in the fixture's unit (for a knob without a unit: its position)
    else:
        raise TableError(f"{effect['slug']}.{param}: no conversion for {value!s}")
    tolerance = 1e-9 * max(1.0, abs(spec["max"]))
    if not spec["min"] - tolerance <= result <= spec["max"] + tolerance:
        raise TableError(f"{effect['slug']}.{param}: {value!s} gives {result}, outside [{spec['min']}, {spec['max']}]")
    return min(max(result, spec["min"]), spec["max"])


def demo_events(demo, effects):
    """The demo's events in the fixture's units, sorted by time; at equal times stepped switches come first, so a range
    change applies before a value that depends on it."""
    effect = effects[demo.slug]
    stepped = {p["id"] for p in effect["params"] if p["taper"] == "stepped"}
    raw = []
    for setting in demo.settings:
        raw.extend(setting.events(demo.seconds))
    order = sorted(range(len(raw)), key=lambda k: (raw[k][0], 0 if raw[k][1] in stepped else 1, k))
    steps = {p["id"]: int(p["def"]) for p in effect["params"] if p["taper"] == "stepped"}
    events = []
    for k in order:
        time, param, value = raw[k]
        if not 0.0 <= time < demo.seconds:
            raise TableError(f"{demo.stem}: event at {time} s is outside the processed section")
        fixture_value = to_fixture_value(effect, param, value, steps)
        if param in stepped:
            steps[param] = int(fixture_value)
        events.append((time, param, fixture_value, param in stepped))
    return events


def event_text(demo, effects):
    lines = [f"# {demo.stem}: generated by tools/render_demos.py --write-events from its demo table; do not edit"]
    for time, param, value, is_step in demo_events(demo, effects):
        lines.append(f"{time:.3f} {param} {int(value) if is_step else format(value, '.9g')}")
    return "\n".join(lines) + "\n"


def write_events(effects, directory):
    directory.mkdir(parents=True, exist_ok=True)
    for demo in DEMOS:
        (directory / f"{demo.stem}.txt").write_text(event_text(demo, effects), encoding="utf-8")
    print(f"wrote {len(DEMOS)} event files to {directory}")


# ---------------------------------------------------------------------------------------------------------------------
# Audio helpers

def read_f32(path):
    data = array.array("f")
    data.frombytes(pathlib.Path(path).read_bytes())
    if sys.byteorder == "big":
        data.byteswap()
    return data


def write_f32(path, values):
    data = array.array("f", values)
    if sys.byteorder == "big":
        data.byteswap()
    pathlib.Path(path).write_bytes(data.tobytes())


def peak_of(*channels):
    return max(max(map(abs, c)) if len(c) else 0.0 for c in channels)


def rms_of(values):
    return math.sqrt(math.fsum(x * x for x in values) / len(values)) if len(values) else 0.0


def dbfs(level):
    return 20.0 * math.log10(level) if level > 0.0 else float("-inf")


def source_samples(name, inputs, start, count):
    """`count` samples of a named source from sample `start`; the dry inputs are 25 s long."""
    if name == "silence":
        return array.array("f", bytes(4 * count))
    if name == "one_plucked_note":
        tones = inputs["plucked_tones"]
        note = int(ONE_NOTE_SECONDS * SAMPLE_RATE)
        return array.array("f", (tones[k] if k < note else 0.0 for k in range(start, start + count)))
    data = inputs[name]
    if start + count > len(data):
        raise TableError(f"input {name} is shorter than {(start + count) / SAMPLE_RATE} s")
    return data[start:start + count]


def demo_input(demo, inputs):
    """The signal fed to the effect: one source, or several sources each taking an equal part of the processed time
    (each starting from its own beginning)."""
    total = int(round(demo.seconds * SAMPLE_RATE))
    names = demo.source if isinstance(demo.source, tuple) else (demo.source,)
    out = array.array("f")
    for k, name in enumerate(names):
        begin = total * k // len(names)
        end = total * (k + 1) // len(names)
        out.extend(source_samples(name, inputs, 0, end - begin))
    return out


def run_render(muff_render, slug, samples, events_path, mode, scratch):
    in_path = scratch / "in.f32"
    out_path = scratch / "out.f32"
    write_f32(in_path, samples)
    subprocess.run([str(muff_render), slug, str(in_path), str(out_path), str(events_path), mode], check=True)
    out = read_f32(out_path)
    if len(out) != 2 * len(samples):
        raise RuntimeError(f"muff_render wrote {len(out)} values for {len(samples)} frames")
    return out[0::2], out[1::2]


def write_wav24(path, left, right, gain):
    frames = len(left)
    ints = array.array("i", bytes(4 * 2 * frames))
    limit_hi, limit_lo = FULL_SCALE_24 - 1, -FULL_SCALE_24
    for k in range(frames):
        for c, channel in ((0, left), (1, right)):
            v = int(round(channel[k] * gain * FULL_SCALE_24))
            ints[2 * k + c] = (min(max(v, limit_lo), limit_hi)) * 256   # the 24-bit value in the top three bytes
    if sys.byteorder == "big":
        ints.byteswap()
    raw = ints.tobytes()
    packed = bytearray(3 * 2 * frames)
    packed[0::3], packed[1::3], packed[2::3] = raw[1::4], raw[2::4], raw[3::4]
    with wave.open(str(path), "wb") as w:
        w.setnchannels(2)
        w.setsampwidth(3)
        w.setframerate(SAMPLE_RATE)
        w.writeframes(bytes(packed))


# ---------------------------------------------------------------------------------------------------------------------
# Rendering the bundle

def check_event_file(demo, effects):
    path = DEMOS_DIR / f"{demo.stem}.txt"
    if not path.is_file() or path.read_text(encoding="utf-8") != event_text(demo, effects):
        raise TableError(f"{path} is missing or differs from the demo table: run tools/render_demos.py --write-events")
    return path


def render_all(muff_render, inputs_dir, out_dir):
    effects = load_fixture()
    inputs = {name: read_f32(pathlib.Path(inputs_dir) / f"{name}.f32") for name in DRY_INPUTS}
    out_dir = pathlib.Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    dry_count = int(DRY_SECONDS * SAMPLE_RATE)
    failures, rows, sums = [], [], []
    with tempfile.TemporaryDirectory() as tmp:
        scratch = pathlib.Path(tmp)
        for demo in DEMOS:
            total_seconds = DRY_SECONDS + demo.seconds
            if not MIN_TOTAL_SECONDS <= total_seconds <= MAX_TOTAL_SECONDS:
                raise TableError(f"{demo.stem}: {total_seconds} s is outside 12 to 25 s")
            events_path = check_event_file(demo, effects)
            samples = demo_input(demo, inputs)
            left, right = run_render(muff_render, demo.slug, samples, events_path, "stereo" if demo.stereo else "mono", scratch)
            processed_peak = peak_of(left, right)
            peak_db = dbfs(processed_peak)
            dry = samples[:dry_count]
            full_left, full_right = dry + left, dry + right
            gain = 10.0 ** (TARGET_PEAK_DBFS / 20.0) / peak_of(full_left, full_right) if processed_peak > 0.0 else 1.0
            wav_name = f"{demo.stem}.wav"
            write_wav24(out_dir / wav_name, full_left, full_right, gain)
            sums.append(f"{hashlib.sha256((out_dir / wav_name).read_bytes()).hexdigest()}  {wav_name}")
            note = ""
            if peak_db > FAIL_ABOVE_DBFS:
                failures.append(f"{demo.stem}: processed peak {peak_db:+.2f} dBFS is above {FAIL_ABOVE_DBFS:+.1f} dBFS "
                                "(the effect breaks the output limit of D-012)")
                note = "FAIL: above the D-012 limit"
            elif peak_db < FAIL_BELOW_DBFS:
                failures.append(f"{demo.stem}: processed peak {peak_db:+.2f} dBFS is below {FAIL_BELOW_DBFS:+.0f} dBFS "
                                "(check the demo table's conversion first, then the effect)")
                note = "FAIL: too quiet"
            elif peak_db > LOUD_ABOVE_DBFS:
                print(f"warning: {demo.stem}: processed peak {peak_db:+.2f} dBFS is above {LOUD_ABOVE_DBFS:+.0f} dBFS"
                      + (" (expected)" if demo.number in EXPECTED_LOUD else ""), file=sys.stderr)
                note = "loud"
            elif demo.number in EXPECTED_LOUD:
                note = f"expected above {LOUD_ABOVE_DBFS:+.0f} dBFS by the plan; not reached"
            if demo.number == MONO_CHECK_DEMO:
                note = (note + "; " if note else "") + mono_self_check(muff_render, demo, samples, events_path, left, scratch, failures)
            rows.append((demo, peak_db, 20.0 * math.log10(gain), note))
            print(f"{demo.stem}: processed peak {peak_db:+.2f} dBFS, gain {20.0 * math.log10(gain):+.2f} dB {note}".rstrip())
    (out_dir / "SHA256SUMS").write_text("\n".join(sorted(sums, key=lambda s: s.split("  ")[1])) + "\n", encoding="utf-8")
    (out_dir / "INDEX.md").write_text(index_text(rows, effects), encoding="utf-8")
    for message in failures:
        print(f"error: {message}", file=sys.stderr)
    return 1 if failures else 0


def mono_self_check(muff_render, demo, samples, events_path, mono, scratch, failures):
    """Demo 14 is also rendered in stereo: the mono rendering (Left + Right of the bank) must differ from the stereo
    left channel by an RMS of at least 10 % of that channel's RMS, which shows that filters 2, 4, 6 and 8 are in it."""
    stereo_left, _ = run_render(muff_render, demo.slug, samples, events_path, "stereo", scratch)
    difference = rms_of([a - b for a, b in zip(mono, stereo_left)])
    reference = rms_of(stereo_left)
    ratio = difference / reference if reference > 0.0 else 0.0
    if ratio < MONO_CHECK_MIN_RATIO:
        failures.append(f"{demo.stem}: mono rendering differs from the stereo left channel by {100 * ratio:.1f} % RMS, "
                        f"less than {100 * MONO_CHECK_MIN_RATIO:.0f} % (the right-hand filters are missing from mono)")
    return f"mono vs stereo left: {100 * ratio:.0f} % RMS difference"


def index_text(rows, effects):
    lines = [
        "# Listening demos for gate G-LISTEN",
        "",
        "Rendered by `tools/render_demos.py` with `muff_render` (the effect driven through `muff::BlockRunner`, the path of",
        "the VST3 plug-in) from the dry inputs of `tools/make_demo_inputs.py`. Every file is 48 kHz, 24-bit, stereo, and",
        f"starts with {DRY_SECONDS:g} s of the dry input followed by the processed signal. Each file is peak-normalised to",
        f"{TARGET_PEAK_DBFS:g} dBFS with one gain for the whole file, so the dry and processed sections keep their level",
        "relation. \"Processed peak\" is the peak of the processed section before normalisation, with 0 dBFS = 1.0 in core",
        f"units; above {LOUD_ABOVE_DBFS:+.0f} dBFS it is marked loud (expected for demos 03, 12 and 23). Settings not named are",
        "at their defaults. Mono demos carry what a mono host hears on both sides; stereo demos carry the effect's left and",
        "right outputs.",
        "",
        "| Demo | Effect | Input | Output | Settings | Processed peak (dBFS) | Gain applied (dB) | Note |",
        "|---|---|---|---|---|---|---|---|",
    ]
    for demo, peak_db, gain_db, note in rows:
        settings = "; ".join(s.describe(demo.seconds) for s in demo.settings)
        lines.append(f"| {demo.stem} | {effects[demo.slug]['name']} | {demo.source_text()} | "
                     f"{'stereo' if demo.stereo else 'mono'} | {settings} | {peak_db:+.2f} | {gain_db:+.2f} | {note} |")
    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------------------------------------------------------------
# Self-test

def check(condition, message, problems):
    if not condition:
        problems.append(message)


def close(a, b, rel=1e-12):
    return abs(a - b) <= rel * max(abs(a), abs(b))


def formula_self_test(problems):
    """Known points of each law, written out from the decision texts, and round trips through the inverse."""
    # D-021 base Lo 0.6 (80/0.6)^n, Hi 30 (4000/30)^n
    forward_ring = lambda n, r: (0.6 * (80.0 / 0.6) ** n) if r == 0 else (30.0 * (4000.0 / 30.0) ** n)
    # D-022 rate 0.01 x 250^n Hz (Lo) or 1 x 250^n Hz (Hi)
    forward_phaser = lambda n, r: (0.01 if r == 0 else 1.0) * 250.0 ** n
    # D-023 d = lo x 10^n, lo = 40 ms (Short) or 80 ms (Long)
    forward_delay = lambda n, r: (40.0 if r == 0 else 80.0) * 10.0 ** n
    # D-026 d = lo (hi/lo)^time: Flange 0.6-10 ms, Chorus 5-50 ms (LFO amount 0)
    forward_chorus = lambda n, r: (0.6 * (10.0 / 0.6) ** n) if r == 0 else (5.0 * (50.0 / 5.0) ** n)
    known = [
        (ring_frequency_knob, "range", [(0.6, 0, 0.0), (80.0, 0, 1.0), (math.sqrt(0.6 * 80.0), 0, 0.5),
                                         (30.0, 1, 0.0), (4000.0, 1, 1.0), (math.sqrt(30.0 * 4000.0), 1, 0.5)]),
        (phaser_rate_knob, "lfo_range", [(0.01, 0, 0.0), (2.5, 0, 1.0), (0.01 * math.sqrt(250.0), 0, 0.5),
                                         (1.0, 1, 0.0), (250.0, 1, 1.0), (math.sqrt(250.0), 1, 0.5)]),
        (delay_time_knob, "range", [(40.0, 0, 0.0), (400.0, 0, 1.0), (40.0 * math.sqrt(10.0), 0, 0.5),
                                    (80.0, 1, 0.0), (800.0, 1, 1.0), (80.0 * math.sqrt(10.0), 1, 0.5)]),
        (chorus_time_knob, "range", [(0.6, 0, 0.0), (10.0, 0, 1.0), (math.sqrt(6.0), 0, 0.5),
                                     (5.0, 1, 0.0), (50.0, 1, 1.0), (math.sqrt(250.0), 1, 0.5)]),
    ]
    for inverse, key, points in known:
        for value, step, knob in points:
            got = inverse(value, {key: step})
            check(abs(got - knob) < 1e-12, f"{inverse.__name__}({value}, {key}={step}) = {got}, expected {knob}", problems)
    for inverse, forward, key in ((ring_frequency_knob, forward_ring, "range"), (phaser_rate_knob, forward_phaser, "lfo_range"),
                                  (delay_time_knob, forward_delay, "range"), (chorus_time_knob, forward_chorus, "range")):
        for step in (0, 1):
            for k in range(11):
                n = k / 10.0
                back = inverse(forward(n, step), {key: step})
                check(abs(back - n) < 1e-12, f"{inverse.__name__}: round trip of knob {n} ({key}={step}) gave {back}", problems)
    effects = load_fixture()
    check(to_fixture_value(effects["filterbank"], "pattern", Pattern(1), {}) == 0, "pattern 1 is index 0", problems)
    check(to_fixture_value(effects["filterbank"], "pattern", Pattern(12), {}) == 11, "pattern 12 is index 11", problems)
    check(to_fixture_value(effects["filterbank"], "voicing", "Mids", {}) == 1, "Mids is voicing step 1", problems)
    check(to_fixture_value(effects["delay"], "lfo_wave", "Random step", {}) == 5, "Random step is LFO wave 5", problems)
    check(to_fixture_value(effects["lowpass"], "cutoff", Hz(880), {}) == 880.0, "cutoff passes through in Hz", problems)
    check(to_fixture_value(effects["delay"], "time", Ms(300), {"range": 0}) == math.log10(7.5), "300 ms Short", problems)
    for bad in ((effects["delay"], "time", Ms(500), {"range": 0}), (effects["ringmod"], "frequency", Hz(100), {"range": 0}),
                (effects["lowpass"], "cutoff", Hz(20000), {}), (effects["lowpass"], "poles", "3-pole", {})):
        try:
            to_fixture_value(*bad)
            problems.append(f"{bad[0]['slug']}.{bad[1]} accepted the out-of-range value {bad[2]}")
        except TableError:
            pass
    for demo in DEMOS:
        try:
            demo_events(demo, effects)
        except TableError as error:
            problems.append(str(error))
        check(MIN_TOTAL_SECONDS <= DRY_SECONDS + demo.seconds <= MAX_TOTAL_SECONDS, f"{demo.stem}: length", problems)
    check([d.number for d in DEMOS] == list(range(1, 24)), "the table holds demos 1 to 23 in order", problems)


def write_events_file(path, lines):
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def rising_crossing_rate(values, skip):
    """Frequency of a signal from its interpolated rising zero crossings after `skip` samples."""
    times = []
    for k in range(skip + 1, len(values)):
        a, b = values[k - 1], values[k]
        if a < 0.0 <= b:
            times.append(k - 1 + a / (a - b))
    if len(times) < 3:
        return 0.0
    return (len(times) - 1) * SAMPLE_RATE / (times[-1] - times[0])


def echo_position(values, start):
    """Sample index of the largest absolute value from `start` on, refined by a parabola through its neighbours."""
    k = max(range(start, len(values) - 1), key=lambda i: abs(values[i]))
    a, b, c = abs(values[k - 1]), abs(values[k]), abs(values[k + 1])
    denominator = a - 2.0 * b + c
    return k + (0.5 * (a - c) / denominator if denominator else 0.0)


def envelope_period(values, expected, block):
    """Period in seconds of the slow amplitude modulation of `values`: block mean squares, then the strongest
    autocorrelation peak between 0.5 and 1.6 times the expected period."""
    energy = [math.fsum(x * x for x in values[k:k + block]) / block for k in range(0, len(values) - block, block)]
    mean = math.fsum(energy) / len(energy)
    e = [x - mean for x in energy]
    rate = SAMPLE_RATE / block
    lo, hi = int(0.5 * expected * rate), int(1.6 * expected * rate)

    def acf(lag):
        return math.fsum(e[k] * e[k + lag] for k in range(len(e) - lag)) / (len(e) - lag)

    scores = {lag: acf(lag) for lag in range(max(lo - 1, 1), hi + 2)}
    best = max(range(lo, hi + 1), key=lambda lag: scores[lag])
    a, b, c = scores[best - 1], scores[best], scores[best + 1]
    denominator = a - 2.0 * b + c
    refined = best + (0.5 * (a - c) / denominator if denominator else 0.0)
    return refined / rate


def measured_self_test(muff_render, problems):
    """Reads converted settings back through the effects: the carrier frequency of Fluff Ring, the echo time of Fluff
    Delay and Fluff Swarm, and the LFO period of Fluff Phase."""
    effects = load_fixture()
    with tempfile.TemporaryDirectory() as tmp:
        scratch = pathlib.Path(tmp)
        events = scratch / "events.txt"

        # Fluff Ring: a constant input times the carrier; mix 1. The output's crossings give the carrier frequency.
        for range_name, hz in (("Lo", 4.0), ("Lo", 30.0), ("Hi", 350.0), ("Hi", 500.0)):
            step = int(to_fixture_value(effects["ringmod"], "range", range_name, {}))
            knob = ring_frequency_knob(hz, {"range": step})
            write_events_file(events, [f"0 range {step}", f"0 frequency {knob!r}", "0 mix 1"])
            seconds = 4.0 if hz < 10.0 else 1.0
            out, _ = run_render(muff_render, "ringmod", array.array("f", [0.5] * int(seconds * SAMPLE_RATE)), events, "mono", scratch)
            got = rising_crossing_rate(out, 0)
            check(abs(got / hz - 1.0) < 2e-3, f"Fluff Ring {range_name} {hz} Hz: measured {got:.4f} Hz", problems)
            print(f"self-test: Fluff Ring {range_name} {hz:g} Hz -> knob {knob:.6f} -> measured {got:.4f} Hz")

        # Fluff Delay: an impulse, feedback 0, mix 1. The echo's position, less the wet filter's fixed offset (measured
        # as the difference between two settings), gives the delay time.
        for range_name, times in (("Short", (120.0, 150.0, 250.0, 300.0)), ("Long", (600.0, 200.0))):
            step = int(to_fixture_value(effects["delay"], "range", range_name, {}))
            positions = []
            for ms in times:
                knob = delay_time_knob(ms, {"range": step})
                write_events_file(events, [f"0 range {step}", f"0 time {knob!r}", "0 feedback 0", "0 mix 1"])
                signal = array.array("f", [0.0] * int(1.0 * SAMPLE_RATE))
                signal[100] = 1.0
                out, _ = run_render(muff_render, "delay", signal, events, "mono", scratch)
                positions.append((ms, echo_position(out, 200) - 100))
            base_ms, base_pos = positions[0]
            for ms, pos in positions:
                offset_ms = 1000.0 * pos / SAMPLE_RATE - ms
                check(0.0 <= offset_ms < 0.6, f"Fluff Delay {range_name} {ms} ms: echo at {1000 * pos / SAMPLE_RATE:.3f} ms", problems)
                relative = (pos - base_pos) - (ms - base_ms) * SAMPLE_RATE / 1000.0
                check(abs(relative) < 0.5, f"Fluff Delay {range_name} {ms} ms: {relative:.2f} samples off relative to {base_ms} ms", problems)
                print(f"self-test: Fluff Delay {range_name} {ms:g} ms -> echo at {1000 * pos / SAMPLE_RATE:.3f} ms "
                      f"(filter offset {offset_ms:.3f} ms, relative error {relative:+.2f} samples)")

        # Fluff Swarm: the same with the LFO amount at 0 (left output = wet at mix 1). Its wet filter tracks the delay
        # (D-036), so the filter's offset differs between settings and only the absolute position is checked.
        for range_name, times in (("Flange", (2.0, 6.0)), ("Chorus", (15.0, 40.0))):
            step = int(to_fixture_value(effects["chorus"], "range", range_name, {}))
            positions = []
            for ms in times:
                knob = chorus_time_knob(ms, {"range": step})
                write_events_file(events, [f"0 range {step}", f"0 time {knob!r}", "0 lfo_amount 0", "0 feedback 0", "0 mix 1"])
                signal = array.array("f", [0.0] * int(0.2 * SAMPLE_RATE))
                signal[100] = 1.0
                out, _ = run_render(muff_render, "chorus", signal, events, "stereo", scratch)
                positions.append((ms, echo_position(out, 101) - 100))
            for ms, pos in positions:
                offset_ms = 1000.0 * pos / SAMPLE_RATE - ms
                check(0.0 <= offset_ms < 0.1, f"Fluff Swarm {range_name} {ms} ms: echo at {1000 * pos / SAMPLE_RATE:.3f} ms", problems)
                print(f"self-test: Fluff Swarm {range_name} {ms:g} ms -> echo at {1000 * pos / SAMPLE_RATE:.3f} ms")

        # Fluff Phase: a steady 2 kHz sine; the swept notches modulate its level once per LFO cycle.
        for range_name, hz in (("Lo", 0.2), ("Lo", 0.6), ("Hi", 8.0)):
            step = int(to_fixture_value(effects["phaser"], "lfo_range", range_name, {}))
            knob = phaser_rate_knob(hz, {"lfo_range": step})
            write_events_file(events, [f"0 lfo_range {step}", f"0 lfo_rate {knob!r}", "0 lfo_amount 1"])
            seconds = max(2.0, 3.2 / hz)
            count = int(seconds * SAMPLE_RATE)
            tone = array.array("f", (0.3 * math.sin(2.0 * math.pi * 2000.0 * k / SAMPLE_RATE) for k in range(count)))
            out, _ = run_render(muff_render, "phaser", tone, events, "mono", scratch)
            period = envelope_period(out, 1.0 / hz, 24)
            check(abs(period * hz - 1.0) < 0.01, f"Fluff Phase {range_name} {hz} Hz: measured period {period:.4f} s", problems)
            print(f"self-test: Fluff Phase {range_name} {hz:g} Hz -> knob {knob:.6f} -> measured {1.0 / period:.4f} Hz")


def self_test(muff_render):
    problems = []
    formula_self_test(problems)
    if muff_render:
        measured_self_test(muff_render, problems)
    for message in problems:
        print(f"self-test failure: {message}", file=sys.stderr)
    print(f"self-test: {'FAILED' if problems else 'passed'}" + ("" if muff_render else " (formulas only; give --muff-render to measure)"))
    return 1 if problems else 0


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--write-events", action="store_true", help="write demos/demo-NN-<name>.txt from the table")
    parser.add_argument("--self-test", action="store_true", help="check the musical-unit conversions")
    parser.add_argument("--muff-render", help="path of the muff_render executable")
    parser.add_argument("--inputs", help="directory holding the dry inputs (tools/make_demo_inputs.py)")
    parser.add_argument("--out", help="output directory for the WAV files, SHA256SUMS and INDEX.md")
    args = parser.parse_args(argv)
    try:
        if args.write_events:
            write_events(load_fixture(), DEMOS_DIR)
            return 0
        if args.self_test:
            return self_test(args.muff_render)
        if not (args.muff_render and args.inputs and args.out):
            parser.error("rendering needs --muff-render, --inputs and --out")
        return render_all(args.muff_render, args.inputs, args.out)
    except TableError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
