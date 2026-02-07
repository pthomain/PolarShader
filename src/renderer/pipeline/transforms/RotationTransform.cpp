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
#include "renderer/pipeline/ranges/PolarRange.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/maths/PolarMaths.h"
#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif

namespace PolarShader {
    struct RotationTransform::MappedInputs {
        MappedSignal<FracQ0_16> angleSignal;
    };

    RotationTransform::MappedInputs RotationTransform::makeInputs(SFracQ0_16Signal angle) {
        return MappedInputs{
            resolveMappedSignal(PolarRange().mapSignal(std::move(angle)))
        };
    }

    struct RotationTransform::State {
        MappedSignal<FracQ0_16> angleSignal;
        MappedValue<FracQ0_16> angleOffset = MappedValue(FracQ0_16(0));

        explicit State(MappedSignal<FracQ0_16> s)
            : angleSignal(std::move(s)) {
        }
    };

    RotationTransform::RotationTransform(SFracQ0_16Signal angle) {
        auto inputs = makeInputs(std::move(angle));
        state = std::make_shared<State>(std::move(inputs.angleSignal));
    }

    void RotationTransform::advanceFrame(FracQ0_16 progress, TimeMillis elapsedMs) {
        if (!context) {
            Serial.println("RotationTransform::advanceFrame context is null.");
        }
        state->angleOffset = state->angleSignal(progress, elapsedMs);
    }

    UVMap RotationTransform::operator()(const UVMap &layer) const {
        return [state = this->state, layer](UV uv) {
            // Convert to Polar UV (Angle=U, Radius=V)
            UV polar_uv = cartesianToPolarUV(uv);
            
            // Apply rotation to U (angle)
            uint16_t angle_raw = static_cast<uint16_t>(raw(polar_uv.u));
            uint16_t offset_raw = raw(state->angleOffset.get());
            polar_uv.u = FracQ16_16(static_cast<int32_t>(static_cast<uint16_t>(angle_raw + offset_raw)));

            // Convert back to Cartesian UV
            UV rotated_uv = polarToCartesianUV(polar_uv);
            return layer(rotated_uv);
        };
    }
}
