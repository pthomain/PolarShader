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

#ifndef POLAR_SHADER_PIPELINE_TRANSFORMS_SRC_MIRROR_UV_H
#define POLAR_SHADER_PIPELINE_TRANSFORMS_SRC_MIRROR_UV_H

// Shared UV-mirror helpers used by KaleidoscopeTransform and
// RadialKaleidoscopeTransform. The two transforms had byte-identical
// copies of these helpers; consolidating here also enables the WASM
// unity build (web/build_site.py amalgamates project sources into one
// .ino) to compile both transforms in the same translation unit.

#include "renderer/pipeline/maths/units/Units.h"

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
                fl::s16x16::from_raw(mirrorUvRaw(uv.u.raw())),
                fl::s16x16::from_raw(mirrorUvRaw(uv.v.raw()))
            );
        }
    }
}

#endif // POLAR_SHADER_PIPELINE_TRANSFORMS_SRC_MIRROR_UV_H
