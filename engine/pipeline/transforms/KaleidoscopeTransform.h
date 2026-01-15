//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2023 Pierre Thomain

/*
 * This file is part of LED Segments.
 *
 * LED Segments is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LED Segments is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LED Segments. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LED_SEGMENTS_TRANSFORMS_KALEIDOSCOPETRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_KALEIDOSCOPETRANSFORM_H

#include "base/Transforms.h"
#include "../utils/MathUtils.h"

namespace LEDSegments {
    /**
     * @class KaleidoscopeTransform
     * @brief A stateless Polar transform that folds the angular space to create symmetry.
     *
     * The number of segments is controlled by a ValueMapper, which provides a
     * normalized 0-65535 value that this transform maps to the segment count.
     *
     * Phase domain is PhaseTurnsUQ16_16 (wraps at 2^32). In kaleidoscope mode, phase is folded within
     * each segment; in mandala mode, phase is multiplied by segment count and wraps. Mirroring treats
     * the folded boundary deterministically. Use when you need hard angular symmetry; beware of abrupt
     * folds if your source has discontinuities at 0/2π.
     */
    class KaleidoscopeTransform : public PolarTransform {
        uint8_t nbSegments;
        bool isMandala;
        bool isMirroring;

        static constexpr uint8_t MAX_SEGMENTS = 8;

        static Units::PhaseTurnsUQ16_16 foldPhaseKaleidoscope(
            Units::PhaseTurnsUQ16_16 phase_q16,
            fl::u8 nbSegments,
            bool isMirroring
        ) {
            if (nbSegments <= 1) return phase_q16;

            const uint32_t segmentPhase = static_cast<uint32_t>(Units::PHASE_FULL_TURN / nbSegments);
            uint32_t foldedPhase = phase_q16 % segmentPhase;

            bool isOdd = (static_cast<uint64_t>(phase_q16) / segmentPhase) & 1u;
            if (isMirroring && isOdd) {
                uint32_t mirrored = segmentPhase - foldedPhase;
                // Handle the boundary case where foldedPhase is 0.
                return (mirrored == segmentPhase) ? 0 : mirrored;
            }

            return foldedPhase;
        }

    public:
        explicit KaleidoscopeTransform(
            uint8_t nbSegments,
            bool isMandala = false,
            bool isMirroring = true
        ) : nbSegments(nbSegments), isMandala(isMandala), isMirroring(isMirroring) {
        }

        PolarLayer operator()(const PolarLayer &layer) const override {
            return [nbSegments = this->nbSegments, isMandala = this->isMandala, isMirroring = this->isMirroring, layer](Units::PhaseTurnsUQ16_16 angle_q16, Units::FractQ0_16 radius, Units::TimeMillis timeInMillis) {
                uint8_t segments = constrain(nbSegments, (uint8_t)1, MAX_SEGMENTS);

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
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_KALEIDOSCOPETRANSFORM_H
