//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#include "KaleidoscopeTransform.h"
#include <cstdint>

namespace LEDSegments {
    PolarLayer KaleidoscopeTransform::operator()(const PolarLayer &layer) const {
        return [nbSegments = this->nbSegments, isMandala = this->isMandala, isMirroring = this->isMirroring, layer](Units::PhaseTurnsUQ16_16 angle_q16, Units::FractQ0_16 radius, Units::TimeMillis timeInMillis) {
            auto clamp_u8 = [](uint8_t v, uint8_t lo, uint8_t hi) -> uint8_t {
                if (v < lo) return lo;
                if (v > hi) return hi;
                return v;
            };
            uint8_t segments = clamp_u8(nbSegments, (uint8_t)1, MAX_SEGMENTS);

            Units::PhaseTurnsUQ16_16 newAngle_q16;
            if (isMandala) {
                // Multiply the angle by the number of segments. The uint32_t accumulator
                // will naturally wrap, creating the intended mandala effect.
                newAngle_q16 = angle_q16 * segments;
            } else {
                newAngle_q16 = foldPhaseKaleidoscope(angle_q16, segments, isMirroring);
            }

            return layer(newAngle_q16, radius, timeInMillis);
        };
    }
}
