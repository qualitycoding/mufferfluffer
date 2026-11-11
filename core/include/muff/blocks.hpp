// mufferfluffer DSP core — small pure building blocks whose behaviour is frozen (D-014, D-015, D-037).
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

namespace muff {

// Soft limiter used by every Drive stage and feedback path: 2 * tanh(x / 2). Unity slope at 0, limits at +/-2.
float softClip (float x) noexcept;

enum class LfoWave : int { Sine = 0, Triangle = 1, Square = 2, Sawtooth = 3, Ramp = 4, RandomStep = 5 };

// Value in [-1, 1] of a low-frequency oscillator at `phase` in [0, 1):
//   Sine       sin(2*pi*phase)
//   Triangle   1 - 4*|phase - 0.5|            (-1 at phase 0, +1 at phase 0.5)
//   Square     +1 for phase < 0.5, else -1
//   Sawtooth   2*phase - 1                    (rising)
//   Ramp       1 - 2*phase                    (falling)
//   RandomStep `held` (the caller draws a new uniform value in [-1, 1] once per cycle)
// A phase outside [0, 1) is wrapped; a non-finite phase gives 0.
float lfoShape (LfoWave wave, float phase, float held) noexcept;

// Tap-tempo estimator (D-014). Times are in seconds on any monotonic clock.
// The third tap of a run makes a period available: the mean interval between the taps of the run.
// Every further tap updates that mean. A tap more than 5.0 s after the previous one starts a new run
// (the old period stays available until the new run has three taps).
class TapTempo {
public:
    void reset() noexcept;                      // forgets taps and period
    bool tap (double timeSeconds) noexcept;     // true if periodSeconds() changed on this tap
    double periodSeconds() const noexcept;      // 0 until a period is available
private:
    double last_ = -1.0, sum_ = 0.0, period_ = 0.0;
    int count_ = 0;
};

// Attack and decay times of the filter-bank envelopes, in sequencer steps, for an Envelope setting in [0, 10]
// (D-024): piecewise linear through
//   setting  0     2     5    6    8     10
//   attack   0.01  0.01  1.0  2.0  1.0   2.0
//   decay    0.25  1.0   1.0  2.0  0.02  0.02
// Settings outside [0, 10] are clamped; a non-finite setting is treated as 2.
void filterBankEnvelopeTimes (float envelope, float& attackSteps, float& decaySteps) noexcept;

} // namespace muff
