//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

/*
 * This file is part of PolarShader.
 *
 * PolarShader is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PolarShader is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PolarShader. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef POLAR_SHADER_TRANSFORMS_KALEIDOSCOPETRANSFORM_H
#define POLAR_SHADER_TRANSFORMS_KALEIDOSCOPETRANSFORM_H

#include "base/Transforms.h"

namespace PolarShader {
    /**
     * @class KaleidoscopeTransform
     * @brief A stateless Polar transform that folds the angular space to create symmetry.
     *
     * The number of segments is controlled by a ValueMapper, which provides a
     * normalized 0-65535 value that this transform maps to the segment count.
     *
     * Phase domain is AngleTurnsUQ16_16 (wraps at 2^32). In kaleidoscope mode, phase is folded within
     * each segment; in mandala mode, phase is multiplied by segment count and wraps. Mirroring treats
     * the folded boundary deterministically. Use when you need hard angular symmetry; beware of abrupt
     * folds if your source has discontinuities at 0/2π.
     *
     * Parameters: nbSegments (1-8), isMandala (multiply vs fold), isMirroring (flip odd sectors).
     * Recommended order: last polar transform before palette sampling; apply after rotation/vortex.
     */
    class KaleidoscopeTransform : public PolarTransform {
        uint8_t nbSegments;
        bool isMandala;
        bool isMirroring;

        static constexpr uint8_t MAX_SEGMENTS = 8;

        static UnboundedAngle foldPhaseKaleidoscope(
            UnboundedAngle phase_q16,
            fl::u8 nbSegments,
            bool isMirroring
        ) {
            if (nbSegments <= 1) return phase_q16;

            const uint32_t segmentPhase = static_cast<uint32_t>(PHASE_FULL_TURN / nbSegments);
            uint32_t phase_raw = raw(phase_q16);
            uint32_t foldedPhase = phase_raw % segmentPhase;

            bool isOdd = (static_cast<uint64_t>(phase_raw) / segmentPhase) & 1u;
            if (isMirroring && isOdd) {
                uint32_t mirrored = segmentPhase - foldedPhase;
                // Handle the boundary case where foldedPhase is 0.
                return (mirrored == segmentPhase) ? UnboundedAngle(0)
                                                  : UnboundedAngle(mirrored);
            }

            return UnboundedAngle(foldedPhase);
        }

    public:
        explicit KaleidoscopeTransform(
            uint8_t nbSegments,
            bool isMandala = false,
            bool isMirroring = true
        ) : nbSegments(nbSegments), isMandala(isMandala), isMirroring(isMirroring) {
        }

        PolarLayer operator()(const PolarLayer &layer) const override;
    };
}

#endif //POLAR_SHADER_TRANSFORMS_KALEIDOSCOPETRANSFORM_H
