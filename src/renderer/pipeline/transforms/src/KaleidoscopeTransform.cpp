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

#include "renderer/pipeline/transforms/KaleidoscopeTransform.h"
#include "renderer/pipeline/maths/PolarMaths.h"

namespace PolarShader {
    namespace {
        constexpr int32_t UV_PERIOD_RAW = 0x00010000;
        constexpr int32_t UV_MIRROR_PERIOD_RAW = UV_PERIOD_RAW * 2;

        int32_t mirrorUvRaw(int32_t value) {
            int32_t mirrored = value % UV_MIRROR_PERIOD_RAW;
            if (mirrored < 0) {
                mirrored += UV_MIRROR_PERIOD_RAW;
            }
            if (mirrored >= UV_PERIOD_RAW) {
                mirrored = (UV_MIRROR_PERIOD_RAW - 1) - mirrored;
            }
            return mirrored;
        }

        UV mirrorUv(UV uv) {
            return UV(
                sr16(mirrorUvRaw(raw(uv.u))),
                sr16(mirrorUvRaw(raw(uv.v)))
            );
        }
    }

    struct KaleidoscopeTransform::State {
        uint8_t facets;
        bool mirrored;
    };

    KaleidoscopeTransform::KaleidoscopeTransform(uint8_t nbFacets, bool isMirrored)
        : state(std::make_shared<State>(State{nbFacets, isMirrored})) {
    }

    UVMap KaleidoscopeTransform::operator()(const UVMap &layer) const {
        return [state = this->state, layer](UV uv) {
            UV polar_uv = cartesianToPolarUV(mirrorUv(uv));
            
            uint8_t facets = state->facets;
            if (facets > 1u) {
                uint32_t full_turn = ANGLE_FULL_TURN_U32;
                uint32_t sector = full_turn / facets;
                if (sector != 0u) {
                    uint32_t angle_u32 = static_cast<uint32_t>(raw(polar_uv.u));
                    uint32_t index = angle_u32 / sector;
                    uint32_t local = angle_u32 - (index * sector);

                    if (state->mirrored && (index & 1u)) {
                        local = (sector - 1u) - local;
                    }

                    uint32_t scaled = local * facets;
                    polar_uv.u = sr16(static_cast<int32_t>(scaled & 0xFFFFu));
                }
            }

            return layer(polarToCartesianUV(polar_uv));
        };
    }
}
