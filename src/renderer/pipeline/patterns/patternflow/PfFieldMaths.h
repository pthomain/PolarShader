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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFFIELDMATHS_H
#define POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFFIELDMATHS_H

#include "renderer/pipeline/maths/AngleMaths.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include "renderer/pipeline/maths/units/Units.h"

/**
 * Shared fixed-point helpers for the PatternFlow field primitives.
 *
 * This file is original PolarShader code (GPL-3.0-or-later); it contains no
 * adapted PatternFlow source. It builds only on the existing pipeline maths
 * headers (AngleMaths / ScalarMaths / Units).
 *
 * Phase domain: PatternFlow sources compute sin(theta) with theta in radians.
 * Here phase is expressed in "turns", Q16 fixed point where 65536 == one full
 * turn (== 2*pi rad). A source term sin(freq * coord + phase) becomes
 * pfSinTurns(freqCycles * coordQ16 + phaseTurnsQ16), i.e. a source radian
 * frequency f maps to a turn frequency f / (2*pi) cycles.
 */
namespace PolarShader {
    namespace PfMath {
        // sin/cos of a signed Q16 turns value. The low 16 bits wrap mod one turn.
        // Returns s0x16 in [-1, 1].
        inline s0x16 pfSinTurns(int32_t turnsQ16) {
            return angleSinU0x16(u0x16(static_cast<uint16_t>(turnsQ16)));
        }

        inline s0x16 pfCosTurns(int32_t turnsQ16) {
            return angleCosU0x16(u0x16(static_cast<uint16_t>(turnsQ16)));
        }

        // Scale a turns accumulator by num/den in 64-bit to avoid overflow.
        // Used to give each animated wave its own drift rate.
        inline int32_t pfCoefT(int32_t tTurns, int32_t num, int32_t den) {
            return static_cast<int32_t>((static_cast<int64_t>(tTurns) * num) / den);
        }

        // Map a signed accumulator in [-maxAbsRaw, +maxAbsRaw] (s0x16 raw units)
        // onto the pattern intensity currency [0, 65535], clamped. maxAbsRaw > 0.
        inline PatternNormU0x16 pfSignedToNorm(int32_t valueRaw, int32_t maxAbsRaw) {
            if (maxAbsRaw <= 0) return PatternNormU0x16(0);
            int64_t shifted = static_cast<int64_t>(valueRaw) + maxAbsRaw;
            int64_t out = (shifted * U0X16_MAX) / (2 * static_cast<int64_t>(maxAbsRaw));
            if (out < 0) out = 0;
            else if (out > U0X16_MAX) out = U0X16_MAX;
            return PatternNormU0x16(static_cast<uint16_t>(out));
        }

        // Map a unit value in Q16 [0, 65536] onto [0, 65535], clamped.
        inline PatternNormU0x16 pfUnitToNorm(int32_t unitQ16) {
            if (unitQ16 <= 0) return PatternNormU0x16(0);
            if (unitQ16 >= static_cast<int32_t>(U0X16_MAX)) return PatternNormU0x16(U0X16_MAX);
            return PatternNormU0x16(static_cast<uint16_t>(unitQ16));
        }

        // Smooth symmetric bump window centred on distance 0. |d| >= halfWidth
        // yields 0; d == 0 yields full 65535. Shape is (1 - t^2)^2 with
        // t = |d| / halfWidth, a cheap stand-in for a gaussian lobe.
        // dRaw and halfWidthRaw are in the same (s0x16 raw) magnitude units.
        inline uint16_t pfBump(int32_t dRaw, int32_t halfWidthRaw) {
            if (halfWidthRaw <= 0) return 0;
            int32_t ad = dRaw < 0 ? -dRaw : dRaw;
            if (ad >= halfWidthRaw) return 0;
            // t in Q16 [0, 65536)
            uint32_t t = (static_cast<uint64_t>(ad) << 16) / static_cast<uint32_t>(halfWidthRaw);
            uint32_t t2 = (t * t) >> 16;              // t^2, Q16
            uint32_t one_minus = (t2 < U0X16_MAX) ? (U0X16_MAX - t2) : 0u; // (1 - t^2), Q16
            uint32_t bump = (one_minus * one_minus) >> 16;            // squared, Q16
            return static_cast<uint16_t>(bump > U0X16_MAX ? U0X16_MAX : bump);
        }

        // Quantise a [0, 65535] value into `levels` flat bands spanning the range.
        // levels <= 1 is a passthrough.
        inline uint16_t pfPosterize(uint16_t value, uint8_t levels) {
            if (levels <= 1) return value;
            uint32_t band = (static_cast<uint32_t>(value) * levels) >> 16; // 0..levels-1
            if (band >= levels) band = levels - 1;
            return static_cast<uint16_t>((band * U0X16_MAX) / (levels - 1));
        }

        // Fractional part of a signed Q16 value, as unsigned Q16 [0, 65535].
        inline uint16_t pfFractQ16(int32_t q16) {
            return static_cast<uint16_t>(static_cast<uint32_t>(q16) & U0X16_MAX);
        }

        // Partition a unit coordinate (Q16 [0, 65536)) into `cellCount` cells.
        // Writes the integer cell index [0, cellCount) and the in-cell local
        // coordinate as Q16 [0, 65535].
        inline void pfCell(int32_t coordQ16, uint16_t cellCount,
                           uint16_t &index, uint16_t &localQ16) {
            if (cellCount == 0) { index = 0; localQ16 = 0; return; }
            uint32_t c = static_cast<uint32_t>(coordQ16 < 0 ? 0 : coordQ16);
            if (c > U0X16_MAX) c = U0X16_MAX;
            uint32_t prod = c * cellCount;
            uint16_t idx = static_cast<uint16_t>(prod >> 16);
            if (idx >= cellCount) idx = cellCount - 1;
            index = idx;
            localQ16 = static_cast<uint16_t>(prod & U0X16_MAX);
        }

        // Small deterministic integer hash of two cell coordinates -> [0, 65535].
        inline uint16_t pfHash2(uint32_t a, uint32_t b) {
            uint32_t h = a * 2654435761u + b * 40503u + 0x9E3779B9u;
            h ^= h >> 15;
            h *= 0x2C1B3C6Du;
            h ^= h >> 12;
            return static_cast<uint16_t>(h >> 8);
        }

        // Euclidean length of a (dx, dy) offset given in Q16 units, returned in Q16.
        inline uint32_t pfDistQ16(int32_t dxQ16, int32_t dyQ16) {
            uint64_t x = static_cast<int64_t>(dxQ16) * dxQ16;
            uint64_t y = static_cast<int64_t>(dyQ16) * dyQ16;
            return static_cast<uint32_t>(sqrtU64Raw(x + y));
        }
    } // namespace PfMath
} // namespace PolarShader

#endif // POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFFIELDMATHS_H
