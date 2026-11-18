// mufferfluffer DSP core: the clocked delay line of D-033 (see clocked_line.hpp).
// SPDX-License-Identifier: AGPL-3.0-only
#include "clocked_line.hpp"

#include <algorithm>

namespace muff::detail {
namespace {

constexpr int kMinCapacity = 8;                 // the smallest line size() allocates
constexpr double kRebaseThreshold = 1.0e6;      // D-033: the phase is rebased before it exceeds 1e6
constexpr double kMinDelaySamples = 1.0;        // D-033: the delay is at least one sample

} // namespace

void ClockedLine::size (int samples) {
    const auto n = static_cast<std::size_t> (std::max (samples, kMinCapacity));
    values_.assign (n, 0.0f);
    phases_.assign (n, 0.0);
    clear();
}

void ClockedLine::clear() noexcept {
    std::fill (values_.begin(), values_.end(), 0.0f);
    std::fill (phases_.begin(), phases_.end(), 0.0);
    // Slot 0 holds the silent sample that D-033 places one clock step before the first real one; write() sets its
    // phase once the first step is known.
    written_ = values_.empty() ? 0 : 1;
    readIndex_ = 0;
    phase_ = 0.0;
}

std::int64_t ClockedLine::oldestIndex() const noexcept {
    return std::max<std::int64_t> (0, written_ - static_cast<std::int64_t> (values_.size()));
}

float ClockedLine::read() noexcept {
    if (written_ < 2) return 0.0f;      // nothing stored yet but the silent start-up sample
    const double target = phase_ - 1.0;
    const std::int64_t newest = written_ - 1;
    readIndex_ = std::max (readIndex_, oldestIndex());
    const std::size_t s0 = slot (readIndex_);
    if (target <= phases_[s0]) return values_[s0];      // at or before the oldest sample still stored
    while (readIndex_ + 1 < newest && phases_[slot (readIndex_ + 1)] <= target) ++readIndex_;
    const std::size_t a = slot (readIndex_), b = slot (readIndex_ + 1);
    const double fraction = std::min (1.0, (target - phases_[a]) / (phases_[b] - phases_[a]));
    return static_cast<float> ((1.0 - fraction) * static_cast<double> (values_[a]) + fraction * static_cast<double> (values_[b]));
}

void ClockedLine::write (float input, double delaySamples) noexcept {
    if (values_.empty()) return;
    const double delay = delaySamples >= kMinDelaySamples ? std::min (delaySamples, static_cast<double> (values_.size()))
                                                          : kMinDelaySamples;     // also catches NaN
    const double step = 1.0 / delay;
    if (written_ == 1) phases_[0] = phase_ - step;
    const std::size_t s = slot (written_);
    values_[s] = input;
    phases_[s] = phase_;
    ++written_;
    phase_ += step;
    if (phase_ > kRebaseThreshold) rebase();
}

void ClockedLine::rebase() noexcept {
    for (double& p : phases_) p -= kRebaseThreshold;
    phase_ -= kRebaseThreshold;
}

} // namespace muff::detail
