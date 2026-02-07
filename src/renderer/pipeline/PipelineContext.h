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


namespace PolarShader {
    struct PipelineContext {
        // Current zoom scale in Q0.16.
        SFracQ0_16 zoomScale = SFracQ0_16(Q0_16_ONE);
        // Normalized zoom scale in 0..1 (Q0.16), based on the zoom transform's range.
        SFracQ0_16 zoomNormalized = SFracQ0_16(Q0_16_ONE);
        // Current depth value in unsigned Q24.8 domain.
        uint32_t depth = 0u;
        // Palette index offset applied during final palette lookup.
        uint8_t paletteOffset = 0u;
        // Optional low-end clipping for palette lookup.
        PatternNormU16 paletteClip = PatternNormU16(0);
        // Feather width for palette clipping (Q0.16).
        FracQ0_16 paletteClipFeather = FracQ0_16(0);

        // Power curve applied to the clip input.
        enum class PaletteClipPower : uint8_t {
            // No shaping; threshold uses the raw pattern value.
            None = 1,
            // Square the clip input to emphasize peaks and thin out mid values.
            Square = 2,
            // Quartic (square twice) for very peaky, bubble-like masks.
            Quartic = 4
        };

        PaletteClipPower paletteClipPower = PaletteClipPower::None;
        // When true, clip high values instead of low values.
        bool paletteClipInvert = false;
        // Whether palette clipping should be applied.
        bool paletteClipEnabled = false;

        // Palette brightness is always full when mapping colors.
    };
}

#endif // POLAR_SHADER_PIPELINE_PIPELINECONTEXT_H
