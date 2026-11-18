// mufferfluffer DSP core: createEffect (D-011). Each effect's step replaces only its own `case` line.
// SPDX-License-Identifier: AGPL-3.0-only
#include "factories.hpp"

#include "muff/core.hpp"

#include <stdexcept>

namespace muff {

std::unique_ptr<Effect> createEffect (EffectId id) {
    switch (id) {
        case EffectId::Lowpass: return detail::makeLowpass();
        case EffectId::RingMod: return detail::makeRingMod();
        case EffectId::Phaser: return detail::makePhaser();
        case EffectId::Delay: return detail::makeDelay();
        case EffectId::FilterBank: return detail::makeFilterBank();
        case EffectId::OscBox: return detail::makeOscBox();
        case EffectId::Chorus: throw NotImplemented ("Chorus");
        default: throw std::out_of_range ("createEffect: invalid effect id");
    }
}

} // namespace muff
