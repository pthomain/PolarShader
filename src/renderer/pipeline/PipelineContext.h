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

#ifndef POLAR_SHADER_PIPELINE_PIPELINECONTEXT_H
#define POLAR_SHADER_PIPELINE_PIPELINECONTEXT_H

#include "renderer/pipeline/maths/units/Units.h"
#include "renderer/RenderPoint.h"

namespace PolarShader {
    struct PipelineContext {
        // Current elapsed time in milliseconds.
        TimeMillis timeMs = 0u;
        // Current zoom scale in u0x16/s0x16.
        s0x16 zoomScale = s0x16(S0X16_ONE);
        // Palette index offset applied during final palette lookup.
        uint8_t paletteOffset = 0u;
        // Optional low-end clipping for palette lookup.
        PatternNormU0x16 paletteClip = PatternNormU0x16(0);
        // Effective feather width for palette clipping after clip-proportional scaling.
        u0x16 paletteClipFeather = u0x16(0);

        // When true, clip high values instead of low values.
        bool paletteClipInvert = false;
        // Whether palette clipping should be applied.
        bool paletteClipEnabled = false;

        // How the palette turns pattern output into colour.
        enum class PaletteTintMode : uint8_t {
            // Emitted hue (colour patterns) or intensity (scalar patterns)
            // selects a palette entry; paletteOffset is a phase offset into the
            // palette and the emitted value drives brightness.
            HueRemap = 0,
            // Tint the whole scene with the single colour at paletteOffset and
            // use the pattern value (shaped by the clip mask) as alpha.
            ColourMask = 1,
            // Colour patterns render their own emitted hue directly (CHSV),
            // bypassing the palette; paletteOffset shifts the hue. Scalar
            // patterns have no hue and fall back to greyscale intensity.
            Native = 2
        };

        // Default is Native: patterns render their own RGB (colour patterns
        // their emitted hue, scalar patterns greyscale). The palette is only
        // applied when a palette transform opts in by switching this to
        // HueRemap or ColourMask.
        PaletteTintMode paletteTintMode = PaletteTintMode::Native;

        // True when the active palette is the Rainbow palette. A HueRemap of a
        // colour pattern's emitted hue onto the Rainbow palette is redundant
        // (the palette already is the hue wheel), so it is rendered natively.
        bool paletteIsRainbow = false;

        // Logical raster geometry for display-native pixel-grid patterns.
        RasterDisplayInfo rasterDisplay{};

        // Palette brightness is always full when mapping colors.
    };
}

#endif // POLAR_SHADER_PIPELINE_PIPELINECONTEXT_H
