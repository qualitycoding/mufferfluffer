// mufferfluffer DSP core: the clocked delay line of D-033, shared by the Delay and the Chorus (private header).
// SPDX-License-Identifier: AGPL-3.0-only
//
// The line models the clocked transit of a bucket-brigade device. A clock phase advances by 1 / delaySamples per
// sample; every stored sample keeps the phase at which it entered, and the output is the stored signal at phase - 1,
// linearly interpolated in phase. With a steady delay of d samples the output is the input of exactly d samples ago;
// when d changes, the signal already in the line is replayed faster or slower for one new delay time.
//
// Use, per sample: `read()` first, then `write (input, delaySamples)` (which stores the input and advances the clock).
// The output is 0 until the clock phase has advanced by 1 since `clear()`.
// Declarations and trivial accessors only: the member functions are defined in clocked_line.cpp.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace muff::detail {

class ClockedLine {
public:
    // Allocates room for `samples` stored samples (at least 8) and clears the line. Not real-time safe:
    // call it from prepare(). The capacity bounds the longest delay: a read that would need an older sample is moved
    // to the oldest one still stored.
    void size (int samples);
    // Silences the line and restarts the clock. Real-time safe.
    void clear() noexcept;
    // The delayed signal for this sample. Call once per sample, before write().
    float read() noexcept;
    // Stores `input` at the present clock phase, then advances the phase by 1 / delaySamples. The delay is limited to
    // [1, the size given to size()] samples; a non-finite delay counts as one sample.
    void write (float input, double delaySamples) noexcept;

private:
    std::size_t slot (std::int64_t index) const noexcept { return static_cast<std::size_t> (index % static_cast<std::int64_t> (values_.size())); }
    std::int64_t oldestIndex() const noexcept;
    void rebase() noexcept;

    std::vector<float> values_;     // ring buffer of stored samples
    std::vector<double> phases_;    // the clock phase at which each stored sample entered
    std::int64_t written_ = 0;      // samples stored so far, the silent start-up sample included
    std::int64_t readIndex_ = 0;    // the older of the two stored samples that bracket the last read phase
    double phase_ = 0.0;            // present clock phase
};

} // namespace muff::detail
