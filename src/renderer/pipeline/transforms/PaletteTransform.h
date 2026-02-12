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
#include "renderer/pipeline/PipelineContext.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include <memory>

namespace PolarShader {
    /**
     * @brief Per-frame palette index offset.
     *
     * Maps a signed signal through an unsigned range into a palette index range
     * and stores it in the pipeline context.
     */
    class PaletteTransform : public FrameTransform {
    public:
        explicit PaletteTransform(Sf16Signal offset);

        PaletteTransform(
            Sf16Signal offset,
            Sf16Signal clipSignal,
            f16 feather = perMil(100),
            PipelineContext::PaletteClipPower clipPower = PipelineContext::PaletteClipPower::Square
        );

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;

        void setContext(std::shared_ptr<PipelineContext> context) override { this->context = std::move(context); }

    private:
        struct MappedInputs;

        static MappedInputs makeInputs(Sf16Signal offset);

        static MappedInputs makeInputs(
            Sf16Signal offset,
            Sf16Signal clipSignal,
            f16 feather,
            PipelineContext::PaletteClipPower clipPower
        );

        struct State;
        std::shared_ptr<State> state;
        std::shared_ptr<PipelineContext> context;
    };
}

#endif // POLAR_SHADER_TRANSFORMS_PALETTE_PALETTE_TRANSFORM_H
