// mufferfluffer DSP core: private access to the generated filter-bank pattern table.
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

namespace muff::detail {

// Number of steps (0 to 64) of one channel of one pattern; 0 when the channel never triggers (static pattern) or when an
// index is out of range. `pattern` is 0 to 23 (Bass bank 0 to 11, Mids bank 12 to 23), `channel` is 0 to 7.
int patternLength (int pattern, int channel) noexcept;

// Bit s of the result is 1 when the channel triggers on step s (s below patternLength); 0 for an out-of-range index.
unsigned long long patternMask (int pattern, int channel) noexcept;

} // namespace muff::detail
