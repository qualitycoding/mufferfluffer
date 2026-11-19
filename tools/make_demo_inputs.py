# SPDX-License-Identifier: AGPL-3.0-only
"""Writes the five dry inputs of the listening demos (S-LISTEN, D-040).

Usage: python3 tools/make_demo_inputs.py <output directory>

Each input is raw little-endian float32, mono, 48 kHz, 25 s, peak 0.5 in core units (1.0 = full scale):
  plucked_tones.f32   single decaying plucked notes, one every 0.75 s, on A2 C3 E3 A3 repeating
  plucked_chords.f32  the same generator playing A2 E3 A3 together, one chord every 1.5 s
  saw_bass.f32        a sawtooth bass line on A1 (55 Hz) in eighth notes at 120 beats per minute
  drum_pattern.f32    a noise-burst drum pattern at 120 beats per minute
  held_chord.f32      three slightly detuned sawtooths on A2, E3 and A3, held throughout

Standard library only and deterministic: every random value comes from random.Random with a fixed seed, and the sample
loops use only addition and multiplication (the few transcendental functions are evaluated once per note), so two runs
write identical bytes.
"""
import array
import math
import pathlib
import random
import sys

SAMPLE_RATE = 48000
SECONDS = 25.0
LENGTH = int(SAMPLE_RATE * SECONDS)
PEAK = 0.5                      # S-LISTEN "Dry inputs": peak 0.5 in core units
SEED = 20261111                 # one fixed seed per generator, offset below, so the inputs never change
BEAT_SECONDS = 60.0 / 120.0     # 120 beats per minute

A1, A2, A3 = 55.0, 110.0, 220.0


def semitones(base_hz, steps):
    return base_hz * 2.0 ** (steps / 12.0)


C3 = semitones(A2, 3)
E3 = semitones(A2, 7)

FILES = ("plucked_tones", "plucked_chords", "saw_bass", "drum_pattern", "held_chord")


def pluck(freq, seconds, rng, decay_seconds=1.6, brightness=0.5):
    """One Karplus-Strong plucked string: a noise burst circulating in a delay loop with a two-point average.

    The loop length is fractional (linear interpolation) so the note is in tune; `decay_seconds` is roughly the time
    to fall by 60 dB; `brightness` sets how much of the burst's top end is kept (one-pole smoothing of the burst)."""
    period = SAMPLE_RATE / freq - 0.5          # the two-point average adds half a sample of delay
    whole = int(period)
    frac = period - whole
    loop_gain = 10.0 ** (-3.0 / (decay_seconds * freq))
    size = whole + 2
    line = [0.0] * size
    smoothed = 0.0
    for k in range(whole):
        smoothed += brightness * ((2.0 * rng.random() - 1.0) - smoothed)
        line[k] = smoothed
    count = int(seconds * SAMPLE_RATE)
    out = [0.0] * count
    write = whole % size
    prev = 0.0
    for n in range(count):
        a = line[(write - whole) % size]
        b = line[(write - whole - 1) % size]
        delayed = a + frac * (b - a)
        y = loop_gain * 0.5 * (delayed + prev)
        prev = delayed
        line[write] = y
        write = (write + 1) % size
        out[n] = y
    fade = int(0.010 * SAMPLE_RATE)              # damp the string in the last 10 ms, as a new pluck would
    for k in range(fade):
        out[count - fade + k] *= (fade - k) / fade
    return out


def mix_into(target, source, start):
    end = min(len(target), start + len(source))
    for k in range(start, end):
        target[k] += source[k - start]


def plucked_tones():
    rng = random.Random(SEED + 1)
    out = [0.0] * LENGTH
    notes = (A2, C3, E3, A3)
    step = 0.75
    for index in range(int(SECONDS / step)):
        mix_into(out, pluck(notes[index % 4], step, rng), int(round(index * step * SAMPLE_RATE)))
    return out


def plucked_chords():
    rng = random.Random(SEED + 2)
    out = [0.0] * LENGTH
    step = 1.5
    for index in range(int(SECONDS / step)):
        start = int(round(index * step * SAMPLE_RATE))
        for freq in (A2, E3, A3):
            mix_into(out, pluck(freq, step, rng), start)
    return out


def poly_blep(phase, dt):
    """Two-sample polynomial band-limited step correction for a sawtooth with a wrap at phase 0."""
    if phase < dt:
        t = phase / dt
        return t + t - t * t - 1.0
    if phase > 1.0 - dt:
        t = (phase - 1.0) / dt
        return t * t + t + t + 1.0
    return 0.0


def saw_bass():
    # Eighth notes at 120 beats per minute; the line moves around A1 (55 Hz): semitone offsets per eighth note.
    line = (0, 0, 12, 0, 10, 0, 7, 3, 0, 0, 12, 0, 5, 7, 10, 12)
    note_seconds = BEAT_SECONDS / 2.0
    note_len = int(round(note_seconds * SAMPLE_RATE))
    attack = int(0.003 * SAMPLE_RATE)
    release = int(0.020 * SAMPLE_RATE)
    decay = 10.0 ** (-1.0 / (0.6 * SAMPLE_RATE))   # 20 dB per 0.6 s, a bass note that sags a little
    out = [0.0] * LENGTH
    phase = 0.0
    for index in range(LENGTH // note_len + 1):
        dt = semitones(A1, line[index % len(line)]) / SAMPLE_RATE
        start = index * note_len
        level = 1.0
        for k in range(note_len):
            n = start + k
            if n >= LENGTH:
                break
            if k < attack:
                env = k / attack
            elif k >= note_len - release:
                env = (note_len - k) / release
            else:
                env = 1.0
            out[n] = env * level * (2.0 * phase - 1.0 - poly_blep(phase, dt))
            level *= decay
            phase += dt
            if phase >= 1.0:
                phase -= 1.0
    return out


def two_pole_resonator(freq, decay_seconds):
    """Coefficients of a resonant two-pole filter that rings at freq and falls 60 dB in decay_seconds."""
    radius = 10.0 ** (-3.0 / (decay_seconds * SAMPLE_RATE))
    return 2.0 * radius * math.cos(2.0 * math.pi * freq / SAMPLE_RATE), -radius * radius


def drum_hit(kind, rng):
    """One noise burst. kick: a short burst ringing a low resonator; snare: a resonator plus a decaying noise burst;
    hat: high-passed noise with a fast decay."""
    count = int(0.25 * SAMPLE_RATE)
    out = [0.0] * count
    if kind == "kick":
        b1, b2 = two_pole_resonator(55.0, 0.30)
        noise_decay, noise_len, res_gain, noise_gain, hp = 10.0 ** (-3.0 / (0.004 * SAMPLE_RATE)), 240, 0.08, 0.0, False
    elif kind == "snare":
        b1, b2 = two_pole_resonator(190.0, 0.12)
        noise_decay, noise_len, res_gain, noise_gain, hp = 10.0 ** (-3.0 / (0.18 * SAMPLE_RATE)), count, 0.03, 0.6, False
    else:
        b1, b2 = 0.0, 0.0
        noise_decay, noise_len, res_gain, noise_gain, hp = 10.0 ** (-3.0 / (0.05 * SAMPLE_RATE)), count, 0.0, 0.45, True
    y1 = y2 = 0.0
    env = 1.0
    prev_noise = 0.0
    for n in range(count):
        noise = (2.0 * rng.random() - 1.0) * env if n < noise_len else 0.0
        env *= noise_decay
        y = res_gain * noise + b1 * y1 + b2 * y2
        y2, y1 = y1, y
        burst = noise - prev_noise if hp else noise
        prev_noise = noise
        out[n] = y + noise_gain * burst
    return out


def drum_pattern():
    # Sixteenth-note grid at 120 beats per minute, one bar of 16 steps repeated.
    kick = (1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0)
    snare = (0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1)
    hat = (1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0)
    rng = random.Random(SEED + 4)
    out = [0.0] * LENGTH
    step_seconds = BEAT_SECONDS / 4.0
    for index in range(int(SECONDS / step_seconds)):
        start = int(round(index * step_seconds * SAMPLE_RATE))
        position = index % 16
        for kind, grid in (("kick", kick), ("snare", snare), ("hat", hat)):
            if grid[position]:
                mix_into(out, drum_hit(kind, rng), start)
    return out


def held_chord():
    rng = random.Random(SEED + 5)
    cents = (+3.0, -4.0, +2.0)                    # small fixed detuning of the three sawtooths
    voices = []
    for freq, detune in zip((A2, E3, A3), cents):
        voices.append([rng.random(), freq * 2.0 ** (detune / 1200.0) / SAMPLE_RATE])
    fade = int(0.020 * SAMPLE_RATE)
    out = [0.0] * LENGTH
    for n in range(LENGTH):
        total = 0.0
        for voice in voices:
            phase, dt = voice
            total += 2.0 * phase - 1.0 - poly_blep(phase, dt)
            phase += dt
            if phase >= 1.0:
                phase -= 1.0
            voice[0] = phase
        out[n] = total * (n / fade if n < fade else 1.0)
    return out


GENERATORS = {"plucked_tones": plucked_tones, "plucked_chords": plucked_chords, "saw_bass": saw_bass,
              "drum_pattern": drum_pattern, "held_chord": held_chord}


def normalised(samples):
    peak = max(abs(x) for x in samples)
    scale = PEAK / peak
    return array.array("f", (x * scale for x in samples))


def write_f32(path, values):
    data = array.array("f", values)
    if sys.byteorder == "big":
        data.byteswap()
    pathlib.Path(path).write_bytes(data.tobytes())


def main(argv):
    if len(argv) != 2:
        print(__doc__.strip().split("\n\n")[1], file=sys.stderr)
        return 2
    out_dir = pathlib.Path(argv[1])
    out_dir.mkdir(parents=True, exist_ok=True)
    for name in FILES:
        samples = normalised(GENERATORS[name]())
        assert len(samples) == LENGTH
        write_f32(out_dir / f"{name}.f32", samples)
        print(f"{name}.f32: {len(samples)} samples, peak {max(abs(x) for x in samples):.4f}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
