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

#ifndef POLAR_SHADER_TRANSFORMS_TRANSLATIONTRANSFORM_H
#define POLAR_SHADER_TRANSFORMS_TRANSLATIONTRANSFORM_H

#include "renderer/pipeline/signals/Accumulators.h"
#include "renderer/pipeline/transforms/base/Transforms.h"
#include <memory>

namespace PolarShader {
    /**
     * Cartesian translation using direction and speed signals.
     *
     * Direction is a full-turn phase in Q0.16.
     * Velocity is a 0..1 scalar in Q0.16 mapped to a max speed (Q24.8 units).
     */
    class TranslationTransform : public CartesianTransform, public UVTransform {
        struct MappedInputs;
        struct State;
        std::shared_ptr<State> state;

        explicit TranslationTransform(
            MappedSignal<FracQ0_16> direction,
            MappedSignal<int32_t> speed
        );

        explicit TranslationTransform(MappedInputs inputs);

        static MappedInputs makeInputs(
            SFracQ0_16Signal direction,
            SFracQ0_16Signal speed
        );

    public:
        TranslationTransform(
            SFracQ0_16Signal direction,
            SFracQ0_16Signal speed
        );

        void advanceFrame(TimeMillis timeInMillis) override;

        CartesianLayer operator()(const CartesianLayer &layer) const override;

        UVLayer operator()(const UVLayer &layer) const override;

        void setContext(std::shared_ptr<PipelineContext> context) override {
            this->context = context;
            CartesianTransform::setContext(context);
        }
    };
}

#endif // POLAR_SHADER_TRANSFORMS_TRANSLATIONTRANSFORM_H
