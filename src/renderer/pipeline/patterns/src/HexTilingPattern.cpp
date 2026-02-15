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

#include "renderer/pipeline/patterns/HexTilingPattern.h"
#include "renderer/pipeline/maths/CartesianMaths.h"
#include "renderer/pipeline/maths/PatternMaths.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"
#include <utility>

namespace PolarShader {
    namespace {
        uint8_t modPositive(int32_t value, uint8_t mod) {
            int32_t r = value % mod;
            if (r < 0) r += mod;
            return static_cast<uint8_t>(r);
        }

        uint16_t mapColorValue(uint8_t index, uint8_t colors) {
            if (colors <= 1) return SF16_MAX;
            uint32_t numerator = static_cast<uint32_t>(index + 1) * SF16_MAX;
            uint16_t value = static_cast<uint16_t>(numerator / colors);
            return value == 0 ? 1 : value;
        }

        int32_t hexRoundF16(int32_t v_f16) {
            return (v_f16 >= 0) ? (v_f16 + 32768) >> 16 : -(((-v_f16) + 32767) >> 16);
        }

        int32_t abs32(int32_t value) {
            return (value < 0) ? -value : value;
        }

        // L2 distance squared in axial coordinates (60-degree basis)
        // dist^2 = dq^2 + dr^2 + dq*dr
        uint64_t distSqAxial(int32_t dq_f16, int32_t dr_f16) {
            int64_t q = dq_f16;
            int64_t r = dr_f16;
            return static_cast<uint64_t>((q * q + r * r + q * r) >> 16);
        }
    }

    struct HexTilingPattern::UVHexTilingFunctor {
        int32_t hex_radius_raw;
        uint8_t color_count;
        int32_t softness_raw_f16;

        PatternNormU16 operator()(UV uv) const {
            if (hex_radius_raw <= 0) return PatternNormU16(0);

            int32_t x_raw = raw(uv.u);
            int32_t y_raw = raw(uv.v);

            // Flat-top axial conversion (Q16.16)
            int64_t q_num = static_cast<int64_t>(x_raw) * 43691;
            int64_t r_num = static_cast<int64_t>(y_raw) * 37837 - static_cast<int64_t>(x_raw) * 21845;
            
            int32_t q_f16 = static_cast<int32_t>(q_num / hex_radius_raw);
            int32_t r_f16 = static_cast<int32_t>(r_num / hex_radius_raw);
            int32_t s_f16 = -q_f16 - r_f16;

            // Find primary center
            int32_t rx = hexRoundF16(q_f16);
            int32_t rz = hexRoundF16(r_f16);
            int32_t ry = hexRoundF16(s_f16);

            int32_t dq0 = q_f16 - (rx << 16);
            int32_t dr0 = r_f16 - (rz << 16);
            int32_t ds0 = s_f16 - (ry << 16);

            if (abs32(dq0) > abs32(dr0) && abs32(dq0) > abs32(ds0)) {
                rx = -ry - rz;
            } else if (abs32(dr0) > abs32(ds0)) {
                rz = -rx - ry;
            }
            
            dq0 = q_f16 - (rx << 16);
            dr0 = r_f16 - (rz << 16);
            uint64_t d0 = distSqAxial(dq0, dr0);

            // Find second closest center among neighbors
            static constexpr int32_t nq_off[6] = {1, 0, -1, -1, 0, 1};
            static constexpr int32_t nr_off[6] = {0, 1, 1, 0, -1, -1};
            
            uint64_t d1 = 0xFFFFFFFFFFFFFFFFULL;
            int32_t r1x = rx, r1z = rz;

            for (int i = 0; i < 6; ++i) {
                int32_t nx = rx + nq_off[i];
                int32_t nz = rz + nr_off[i];
                uint64_t d = distSqAxial(q_f16 - (nx << 16), r_f16 - (nz << 16));
                if (d < d1) {
                    d1 = d;
                    r1x = nx;
                    r1z = nz;
                }
            }

            uint8_t colors = (color_count < 3) ? 3 : color_count;
            uint16_t c0 = mapColorValue(modPositive(rx - rz, colors), colors);
            uint16_t c1 = mapColorValue(modPositive(r1x - r1z, colors), colors);

            // Edge distance metric (normalized difference of squared distances)
            // Near the edge, (d1 - d0) is linear to the actual distance.
            int32_t diff = static_cast<int32_t>(d1 - d0);
            
            // Antialiasing width (Q16.16 axial squared units)
            // Minimum softness scaled to radius to ensure sub-pixel smoothness.
            int32_t soft = softness_raw_f16 < 1200 ? 1200 : softness_raw_f16;

            if (diff >= soft) return PatternNormU16(c0);
            if (diff <= -soft) return PatternNormU16(c1);

            // Map diff [-soft, soft] to [1.0, 0.0] for high-precision blend
            uint32_t t = ((static_cast<int64_t>(soft) - diff) << 16) / (soft * 2);
            uint16_t mix = raw(patternSmoothstepU16(0, 65535, static_cast<uint16_t>(t)));
            
            int32_t delta = static_cast<int32_t>(c1) - static_cast<int32_t>(c0);
            uint16_t final_val = static_cast<uint16_t>(
                static_cast<int32_t>(c0) + ((static_cast<int64_t>(delta) * mix) >> 16)
            );
            
            return PatternNormU16(final_val);
        }
    };

    HexTilingPattern::HexTilingPattern(uint16_t hexRadius, uint8_t colorCount, uint16_t edgeSoftness)
        : hex_radius_u16(hexRadius == 0 ? 32 : hexRadius),
          color_count(colorCount < 3 ? 3 : colorCount),
          softness_u16(edgeSoftness),
          softness_raw((static_cast<uint32_t>(edgeSoftness) * 20000) >> 16) { 
    }

    HexTilingPattern::HexTilingPattern(
        Sf16Signal hexRadius,
        uint8_t colorCount,
        uint16_t edgeSoftness
    ) : hex_radius_signal(std::move(hexRadius)),
        hex_radius_u16(0),
        color_count(colorCount < 3 ? 3 : colorCount),
        softness_u16(edgeSoftness),
        softness_raw((static_cast<uint32_t>(edgeSoftness) * 20000) >> 16) {
    }

    UVMap HexTilingPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        int32_t radius_raw;
        if (hex_radius_signal) {
            TimeMillis time = context ? context->timeMs : 0;
            MagnitudeRange range(PatternNormU16(0), PatternNormU16(64 << R8_FRAC_BITS));
            PatternNormU16 sampled = hex_radius_signal.sample(range, time);
            radius_raw = raw(sampled);
        } else {
            radius_raw = static_cast<int32_t>(hex_radius_u16) << R8_FRAC_BITS;
        }

        radius_raw /= 10;
        if (radius_raw < (1 << R8_FRAC_BITS)) radius_raw = (1 << R8_FRAC_BITS);
        
        return UVHexTilingFunctor{radius_raw, color_count, softness_raw};
    }
}
