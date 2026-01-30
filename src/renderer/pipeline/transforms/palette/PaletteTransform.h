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

#ifndef POLAR_SHADER_TRANSFORMS_PALETTE_PALETTE_TRANSFORM_H
#define POLAR_SHADER_TRANSFORMS_PALETTE_PALETTE_TRANSFORM_H

#include "renderer/pipeline/transforms/base/Transforms.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/units/ScalarUnits.h"
#include "renderer/pipeline/PipelineContext.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include <memory>

namespace PolarShader {
    /**
     * @brief Per-frame palette index offset.
     *
     * Maps a 0..1 signal into a palette index range and stores it in the pipeline context.
     */
    class PaletteTransform : public FrameTransform {
        struct MappedInputs;
        struct State;
        std::shared_ptr<State> state;

        explicit PaletteTransform(MappedSignal<uint8_t> offsetSignal);

        explicit PaletteTransform(MappedInputs inputs);

        static MappedInputs makeInputs(SFracQ0_16Signal offset);

        static MappedInputs makeInputs(
            SFracQ0_16Signal offset,
            SFracQ0_16Signal clipSignal,
            FracQ0_16 feather,
            PipelineContext::PaletteClipPower clipPower,
            PipelineContext::PaletteBrightnessMode brightnessMode
        );

    public:
        explicit PaletteTransform(
            SFracQ0_16Signal offset,
            PipelineContext::PaletteBrightnessMode brightnessMode = PipelineContext::PaletteBrightnessMode::Pattern
        );

        PaletteTransform(
            SFracQ0_16Signal offset,
            SFracQ0_16Signal clipSignal,
            FracQ0_16 feather = perMil(100),
            PipelineContext::PaletteClipPower clipPower = PipelineContext::PaletteClipPower::Square,
            PipelineContext::PaletteBrightnessMode brightnessMode = PipelineContext::PaletteBrightnessMode::Pattern
        );

        void advanceFrame(TimeMillis timeInMillis) override;

        void setContext(std::shared_ptr<PipelineContext> context) override { this->context = std::move(context); }

    private:
        std::shared_ptr<PipelineContext> context;
    };
}

#endif // POLAR_SHADER_TRANSFORMS_PALETTE_PALETTE_TRANSFORM_H
