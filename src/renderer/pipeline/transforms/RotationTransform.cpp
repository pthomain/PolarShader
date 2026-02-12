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

#include "RotationTransform.h"
#include "renderer/pipeline/signals/ranges/PolarRange.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/signals/accumulators/SignalAccumulators.h"
#include "renderer/pipeline/maths/PolarMaths.h"
#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif

namespace PolarShader {
    struct RotationTransform::MappedInputs {
        SQ0_16Signal angleSignal;
        PolarRange angleRange;
    };

    RotationTransform::MappedInputs RotationTransform::makeInputs(SQ0_16Signal angle) {
        return MappedInputs{
            std::move(angle),
            PolarRange()
        };
    }

    struct RotationTransform::State {
        SQ0_16Signal angleSignal;
        PolarRange angleRange;
        UQ0_16 angleOffset = UQ0_16(0);

        explicit State(MappedInputs inputs)
            : angleSignal(std::move(inputs.angleSignal)),
              angleRange(std::move(inputs.angleRange)) {
        }
    };

    RotationTransform::RotationTransform(SQ0_16Signal angle) {
        auto inputs = makeInputs(std::move(angle));
        state = std::make_shared<State>(std::move(inputs));
    }

    void RotationTransform::advanceFrame(UQ0_16 progress, TimeMillis elapsedMs) {
        if (!context) {
            Serial.println("RotationTransform::advanceFrame context is null.");
        }
        state->angleOffset = state->angleSignal.sample(state->angleRange, elapsedMs);
    }

    UVMap RotationTransform::operator()(const UVMap &layer) const {
        return [state = this->state, layer](UV uv) {
            // Convert to Polar UV (Angle=U, Radius=V)
            UV polar_uv = cartesianToPolarUV(uv);
            
            // Apply rotation to U (angle)
            uint16_t angle_raw = static_cast<uint16_t>(raw(polar_uv.u));
            uint16_t offset_raw = raw(state->angleOffset);
            polar_uv.u = SQ16_16(static_cast<uint16_t>(angle_raw + offset_raw));

            // Convert back to Cartesian UV
            UV rotated_uv = polarToCartesianUV(polar_uv);
            return layer(rotated_uv);
        };
    }
}
