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

#include "HexTilingPattern.h"
#include "renderer/pipeline/maths/CartesianMaths.h"
#include "renderer/pipeline/maths/PatternMaths.h"
#include "renderer/pipeline/signals/ranges/LinearRange.h"
#include <utility>

namespace PolarShader {
    namespace {
        constexpr int32_t kOneThirdQ24_8 = 85; // 0.3333 * 256
        constexpr int32_t kTwoThirdsQ24_8 = 171; // 0.6666 * 256
        constexpr int32_t kSqrt3Over3Q24_8 = 148; // 0.5773 * 256
        constexpr int32_t kFracBits = CARTESIAN_FRAC_BITS;
        constexpr int32_t kHalfUnitRaw = 1 << (CARTESIAN_FRAC_BITS - 1);
        constexpr int32_t kToQ0_16Shift = 16 - CARTESIAN_FRAC_BITS;

        const SQ24_8 kSqrt3Over3 = SQ24_8(kSqrt3Over3Q24_8);
        const SQ24_8 kOneThird = SQ24_8(kOneThirdQ24_8);
        const SQ24_8 kTwoThirds = SQ24_8(kTwoThirdsQ24_8);

        int32_t roundQ24_8_raw(int32_t raw_v) {
            if (raw_v >= 0) {
                return (raw_v + (1 << (kFracBits - 1))) >> kFracBits;
            }
            int32_t abs_v = -raw_v;
            int32_t rounded = (abs_v + ((1 << (kFracBits - 1)) - 1)) >> kFracBits;
            return -rounded;
        }

        uint8_t modPositive(int32_t value, uint8_t mod) {
            int32_t r = value % mod;
            if (r < 0) r += mod;
            return static_cast<uint8_t>(r);
        }

        int32_t abs32(int32_t value) {
            return (value < 0) ? -value : value;
        }

        int32_t max3(int32_t a, int32_t b, int32_t c) {
            int32_t m = (a > b) ? a : b;
            return (m > c) ? m : c;
        }

        int8_t sign(int32_t value) {
            return (value < 0) ? -1 : 1;
        }

        uint16_t mapColorValue(uint8_t index, uint8_t colors) {
            if (colors <= 1) return SQ0_16_MAX;
            uint32_t numerator = static_cast<uint32_t>(index + 1) * SQ0_16_MAX;
            uint16_t value = static_cast<uint16_t>(numerator / colors);
            return value == 0 ? 1 : value;
        }
    }

    // Fixed-point axial/cube coords and their rounded lattice indices.
    struct HexTilingPattern::HexAxial {
        int32_t q_raw;
        int32_t r_raw;
        int32_t s_raw;
        int32_t rx;
        int32_t ry;
        int32_t rz;
    };

    struct HexTilingPattern::UVHexTilingFunctor {
        int32_t hex_radius_raw;
        uint8_t color_count;
        int32_t softness_raw;

        PatternNormU16 operator()(UV uv) const {
            SQ24_8 cx = CartesianMaths::from_uv(uv.u);
            SQ24_8 cy = CartesianMaths::from_uv(uv.v);

            SQ24_8 radius = SQ24_8(hex_radius_raw);
            SQ24_8 x_term = CartesianMaths::mul(cx, kSqrt3Over3);
            SQ24_8 y_term = CartesianMaths::mul(cy, kOneThird);
            SQ24_8 q = CartesianMaths::div(SQ24_8(raw(x_term) - raw(y_term)), radius);
            SQ24_8 r = CartesianMaths::div(CartesianMaths::mul(cy, kTwoThirds), radius);
            HexAxial hex = computeAxial(q, r);

            int32_t axial_q = hex.rx;
            int32_t axial_r = hex.rz;
            uint8_t colors = (color_count < 3) ? 3 : color_count;
            uint8_t color_index = modPositive(axial_q - axial_r, colors);

            uint8_t neighbor_index = color_index;
            uint16_t blend = computeBlend(
                hex,
                softness_raw,
                colors,
                color_index,
                neighbor_index
            );

            uint16_t current_value = mapColorValue(color_index, colors);
            uint16_t neighbor_value = mapColorValue(neighbor_index, colors);
            if (blend == 0) {
                return PatternNormU16(current_value);
            }
            int32_t delta = static_cast<int32_t>(neighbor_value) - static_cast<int32_t>(current_value);
            uint16_t blended = static_cast<uint16_t>(
                static_cast<int32_t>(current_value) + ((delta * blend) >> 16)
            );
            return PatternNormU16(blended);
        }
    };

    HexTilingPattern::HexTilingPattern(uint16_t hexRadius, uint8_t colorCount, uint16_t edgeSoftness)
        : hex_radius_u16(hexRadius),
          color_count(colorCount),
          softness_u16(edgeSoftness) {
        initDerived();
    }

    HexTilingPattern::HexTilingPattern(
        SQ0_16Signal hexRadius,
        uint8_t colorCount,
        SQ0_16Signal edgeSoftness
    ) : hex_radius_u16(sampleSignal(std::move(hexRadius))),
        color_count(colorCount),
        softness_u16(sampleSignal(std::move(edgeSoftness))) {
        initDerived();
    }

    UVMap HexTilingPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        return UVHexTilingFunctor{hex_radius_raw, color_count, softness_raw};
    }

    void HexTilingPattern::initDerived() {
        if (hex_radius_u16 == 0) {
            hex_radius_u16 = 1;
        }
        hex_radius_raw = static_cast<int32_t>(hex_radius_u16) << CARTESIAN_FRAC_BITS;
        if (hex_radius_raw < (1 << CARTESIAN_FRAC_BITS)) {
            hex_radius_raw = (1 << CARTESIAN_FRAC_BITS);
        }
        if (color_count < 3) {
            color_count = 3;
        }

        int32_t softness_cart_raw = static_cast<int32_t>(softness_u16) << CARTESIAN_FRAC_BITS;
        SQ24_8 softness_axial = CartesianMaths::div(
            SQ24_8(softness_cart_raw),
            SQ24_8(hex_radius_raw)
        );
        softness_raw = raw(softness_axial);
        if (softness_raw < 0) softness_raw = 0;
        if (softness_raw > kMaxSoftnessQ24_8) softness_raw = kMaxSoftnessQ24_8;
    }

    uint16_t HexTilingPattern::sampleSignal(SQ0_16Signal signal) {
        if (!signal) return 0;
        LinearRange range(PatternNormU16(0), PatternNormU16(SQ0_16_MAX));
        PatternNormU16 value = signal.sample(range, 0);
        return raw(value);
    }

    HexTilingPattern::HexAxial HexTilingPattern::computeAxial(SQ24_8 q, SQ24_8 r) {
        HexAxial h{};
        h.q_raw = raw(q);
        h.r_raw = raw(r);
        h.s_raw = -h.q_raw - h.r_raw;
        h.rx = roundQ24_8_raw(h.q_raw);
        h.rz = roundQ24_8_raw(h.r_raw);
        h.ry = roundQ24_8_raw(h.s_raw);

        int32_t dx = abs32((h.rx << kFracBits) - h.q_raw);
        int32_t dy = abs32((h.ry << kFracBits) - h.s_raw);
        int32_t dz = abs32((h.rz << kFracBits) - h.r_raw);

        if (dx > dy && dx > dz) {
            h.rx = -h.ry - h.rz;
        } else if (dy > dz) {
            h.ry = -h.rx - h.rz;
        } else {
            h.rz = -h.rx - h.ry;
        }
        return h;
    }

    uint16_t HexTilingPattern::computeBlend(
        const HexAxial &hex,
        int32_t softnessRaw,
        uint8_t colors,
        uint8_t colorIndex,
        uint8_t &neighborIndex
    ) {
        if (softnessRaw <= 0) {
            neighborIndex = colorIndex;
            return 0;
        }

        int32_t qf_raw = hex.q_raw - (hex.rx << kFracBits);
        int32_t rf_raw = hex.r_raw - (hex.rz << kFracBits);
        int32_t sf_raw = hex.s_raw - (hex.ry << kFracBits);

        int32_t aq = abs32(qf_raw);
        int32_t ar = abs32(rf_raw);
        int32_t as = abs32(sf_raw);

        int32_t nq = hex.rx;
        int32_t nr = hex.rz;
        int8_t dir = 0;
        if (aq >= ar && aq >= as) {
            dir = sign(qf_raw);
            nq = hex.rx + dir;
        } else if (ar >= as) {
            dir = sign(rf_raw);
            nr = hex.rz + dir;
        } else {
            dir = sign(sf_raw);
            nq = hex.rx - dir;
            nr = hex.rz + dir;
        }

        neighborIndex = modPositive(nq - nr, colors);

        int32_t d = max3(aq, ar, as);
        int32_t edge_dist = kHalfUnitRaw - d;
        if (edge_dist <= 0) {
            return SQ0_16_MAX;
        }
        if (edge_dist >= softnessRaw) {
            return 0;
        }

        uint16_t x = static_cast<uint16_t>(edge_dist << kToQ0_16Shift);
        uint16_t edge1 = static_cast<uint16_t>(softnessRaw << kToQ0_16Shift);
        uint16_t mask = raw(patternSmoothstepU16(0, edge1, x));
        return static_cast<uint16_t>(SQ0_16_MAX - mask);
    }
}
