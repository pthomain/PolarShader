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

#include "renderer/pipeline/transforms/RotationTransform.h"
#include "renderer/pipeline/signals/ranges/AngleRange.h"
#include "renderer/pipeline/signals/ranges/BipolarRange.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/maths/PolarMaths.h"
#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif

namespace PolarShader {
    struct RotationTransform::MappedInputs {
        Sf16Signal angleSignal;
        bool isAngleTurn;
    };

    RotationTransform::MappedInputs RotationTransform::makeInputs(Sf16Signal angle, bool isAngleTurn) {
        return MappedInputs{
            std::move(angle),
            isAngleTurn
        };
    }

    struct RotationTransform::State {
        Sf16Signal angleSignal;
        bool isAngleTurn;
        f16 angleOffset = f16(0);
        std::unique_ptr<PhaseAccumulator> accumulator;

        explicit State(MappedInputs inputs)
            : angleSignal(std::move(inputs.angleSignal)),
              isAngleTurn(inputs.isAngleTurn) {
            if (!isAngleTurn) {
                accumulator = std::make_unique<PhaseAccumulator>(
                    [this](TimeMillis elapsedMs) {
                        return angleSignal.sample(bipolarRange(), elapsedMs);
                    }
                );
            }
        }
    };

    RotationTransform::RotationTransform(Sf16Signal angle, bool isAngleTurn) {
        auto inputs = makeInputs(std::move(angle), isAngleTurn);
        state = std::make_shared<State>(std::move(inputs));
    }

    void RotationTransform::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        if (state->isAngleTurn) {
            static AngleRange angleRange;
            state->angleOffset = state->angleSignal.sample(angleRange, elapsedMs);
        } else {
            state->angleOffset = state->accumulator->advance(elapsedMs);
        }
    }

    UVMap RotationTransform::operator()(const UVMap &layer) const {
        return [state = this->state, layer](UV uv) {
            // Convert to Polar UV (Angle=U, Radius=V)
            UV polar_uv = cartesianToPolarUV(uv);
            
            // Apply rotation to U (angle)
            uint16_t angle_raw = static_cast<uint16_t>(raw(polar_uv.u));
            uint16_t offset_raw = raw(state->angleOffset);
            polar_uv.u = sr16(static_cast<uint16_t>(angle_raw + offset_raw));

            // Convert back to Cartesian UV
            UV rotated_uv = polarToCartesianUV(polar_uv);
            return layer(rotated_uv);
        };
    }
}
