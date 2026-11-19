// muff_render: renders one effect offline for the listening demos (S-LISTEN, D-040).
// SPDX-License-Identifier: AGPL-3.0-only
//
// Usage: muff_render <slug> <in.f32> <out.f32> <events.txt> <mono|stereo>
//   slug        the effect's slug from tests/fixtures/effects.json (lowpass, ringmod, ...)
//   in.f32      raw little-endian float32, mono, 48 kHz, core units
//   out.f32     raw little-endian float32, stereo interleaved (left, right), as many frames as the input
//   events.txt  one event per line, `<seconds> <param id> <value>`, sorted by time; the value is in the parameter's
//               unit as the fixture states it (stepped parameters: the step index). An event takes effect at the first
//               sample at or after its time. Blank lines and lines starting with '#' are ignored.
//   mono        the runner is asked for one output channel (a mono host: the right output is summed in when the
//               effect says so) and that channel is written to both sides
//   stereo      the runner is asked for two output channels, written left and right
//
// The effect is driven through muff::BlockRunner, the path the VST3 plug-in uses, at 48 kHz in blocks of 64 frames
// with one input channel. A block is split where an event falls inside it, so every event lands on its own sample;
// the runner's result does not depend on how the signal is cut into blocks (adapters.hpp).
// Exit status: 0 on success, 2 on a usage or input error (message on stderr).

#include "muff/adapters.hpp"
#include "muff/core.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr double kSampleRate = 48000.0;
constexpr int kBlockFrames = 64;
constexpr double kEventTimeTolerance = 1.0e-6;  // in samples: absorbs decimal rounding of event times like 0.01 s

struct Event {
    long long frame;
    int param;
    float value;
};

struct UsageError : std::runtime_error {
    explicit UsageError (const std::string& what) : std::runtime_error (what) {}
};

muff::EffectId effectFromSlug (const std::string& slug) {
    for (int k = 0; k < muff::kNumEffects; ++k) {
        const auto id = static_cast<muff::EffectId> (k);
        if (slug == muff::effectInfo (id).slug)
            return id;
    }
    throw UsageError ("unknown effect slug: " + slug);
}

float decodeFloat (const unsigned char* b) {
    const std::uint32_t bits = static_cast<std::uint32_t> (b[0]) | (static_cast<std::uint32_t> (b[1]) << 8)
                               | (static_cast<std::uint32_t> (b[2]) << 16) | (static_cast<std::uint32_t> (b[3]) << 24);
    float f;
    std::memcpy (&f, &bits, sizeof f);
    return f;
}

void encodeFloat (float f, unsigned char* b) {
    std::uint32_t bits;
    std::memcpy (&bits, &f, sizeof bits);
    for (int k = 0; k < 4; ++k)
        b[k] = static_cast<unsigned char> ((bits >> (8 * k)) & 0xFFu);
}

std::vector<float> readInput (const std::string& path) {
    std::ifstream file (path, std::ios::binary);
    if (! file)
        throw UsageError ("cannot open input: " + path);
    const std::vector<unsigned char> bytes ((std::istreambuf_iterator<char> (file)), std::istreambuf_iterator<char>());
    if (bytes.size() % 4 != 0)
        throw UsageError ("input size is not a whole number of float32 samples: " + path);
    std::vector<float> samples (bytes.size() / 4);
    for (std::size_t k = 0; k < samples.size(); ++k)
        samples[k] = decodeFloat (&bytes[4 * k]);
    return samples;
}

void writeOutput (const std::string& path, const std::vector<float>& left, const std::vector<float>& right) {
    std::vector<unsigned char> bytes (left.size() * 8);
    for (std::size_t k = 0; k < left.size(); ++k) {
        encodeFloat (left[k], &bytes[8 * k]);
        encodeFloat (right[k], &bytes[8 * k + 4]);
    }
    std::ofstream file (path, std::ios::binary);
    file.write (reinterpret_cast<const char*> (bytes.data()), static_cast<std::streamsize> (bytes.size()));
    if (! file)
        throw UsageError ("cannot write output: " + path);
}

long long frameOfTime (double seconds) {
    return static_cast<long long> (std::ceil (seconds * kSampleRate - kEventTimeTolerance));
}

std::vector<Event> readEvents (const std::string& path, const muff::EffectInfo& info) {
    std::ifstream file (path);
    if (! file)
        throw UsageError ("cannot open event file: " + path);
    std::vector<Event> events;
    std::string line;
    int lineNumber = 0;
    double previousTime = 0.0;
    while (std::getline (file, line)) {
        ++lineNumber;
        if (line.empty() || line[0] == '#' || line.find_first_not_of (" \t\r") == std::string::npos)
            continue;
        std::istringstream fields (line);
        double seconds = 0.0;
        std::string paramId;
        double value = 0.0;
        std::string extra;
        const std::string where = path + ":" + std::to_string (lineNumber);
        if (! (fields >> seconds >> paramId >> value) || (fields >> extra))
            throw UsageError (where + ": expected `<seconds> <param id> <value>`");
        if (! std::isfinite (seconds) || seconds < 0.0 || ! std::isfinite (value))
            throw UsageError (where + ": time and value must be finite and the time not negative");
        if (seconds < previousTime)
            throw UsageError (where + ": events are not sorted by time");
        const int param = muff::findParam (info, paramId.c_str());
        if (param < 0)
            throw UsageError (where + ": " + info.slug + " has no parameter " + paramId);
        previousTime = seconds;
        events.push_back (Event { frameOfTime (seconds), param, static_cast<float> (value) });
    }
    return events;
}

// Runs the whole input through the runner. Events due at frame 0 are applied before the effect's state is reset, so
// the smoothers start at those values (D-017) and the first sample is already at the demo's settings.
void render (muff::BlockRunner& runner, const std::vector<float>& input, const std::vector<Event>& events, int numOut,
             std::vector<float>& left, std::vector<float>& right) {
    muff::Effect& fx = runner.effect();
    runner.prepare (kSampleRate);
    std::size_t next = 0;
    while (next < events.size() && events[next].frame <= 0) {
        fx.setParam (events[next].param, events[next].value);
        ++next;
    }
    fx.reset();

    const long long total = static_cast<long long> (input.size());
    left.assign (input.size(), 0.0f);
    right.assign (input.size(), 0.0f);
    long long frame = 0;
    while (frame < total) {
        while (next < events.size() && events[next].frame <= frame) {
            fx.setParam (events[next].param, events[next].value);
            ++next;
        }
        long long end = std::min (total, (frame / kBlockFrames + 1) * kBlockFrames);
        if (next < events.size() && events[next].frame < end)
            end = events[next].frame;
        const int count = static_cast<int> (end - frame);
        const float* in[1] = { input.data() + frame };
        float* out[2] = { left.data() + frame, right.data() + frame };
        runner.process (in, 1, out, numOut, count);
        frame = end;
    }
    if (numOut == 1)
        right = left;
}

int run (int argc, char** argv) {
    if (argc != 6)
        throw UsageError ("usage: muff_render <slug> <in.f32> <out.f32> <events.txt> <mono|stereo>");
    const std::string mode = argv[5];
    if (mode != "mono" && mode != "stereo")
        throw UsageError ("the fifth argument must be `mono` or `stereo`, not " + mode);
    const muff::EffectId id = effectFromSlug (argv[1]);
    const std::vector<float> input = readInput (argv[2]);
    muff::BlockRunner runner (id);
    const std::vector<Event> events = readEvents (argv[4], runner.effect().info());
    std::vector<float> left;
    std::vector<float> right;
    render (runner, input, events, mode == "mono" ? 1 : 2, left, right);
    writeOutput (argv[3], left, right);
    return 0;
}

} // namespace

int main (int argc, char** argv) {
    try {
        return run (argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "muff_render: " << e.what() << '\n';
        return 2;
    }
}
