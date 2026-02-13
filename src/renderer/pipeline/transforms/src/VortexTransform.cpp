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

#include "renderer/pipeline/transforms/VortexTransform.h"
#include "renderer/pipeline/signals/ranges/BipolarRange.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/maths/PolarMaths.h"
#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif

namespace PolarShader {
    struct VortexTransform::MappedInputs {
        Sf16Signal strengthSignal;
        BipolarRange<sf16> strengthRange;
    };

    VortexTransform::MappedInputs VortexTransform::makeInputs(Sf16Signal strength) {
        return MappedInputs{
            std::move(strength),
            BipolarRange(sf16(SF16_MIN), sf16(SF16_MAX))
        };
    }

    struct VortexTransform::State {
        Sf16Signal strengthSignal;
        BipolarRange<sf16> strengthRange;
        sf16 strengthValue = sf16(0);

        explicit State(MappedInputs inputs)
            : strengthSignal(std::move(inputs.strengthSignal)),
              strengthRange(std::move(inputs.strengthRange)) {
        }
    };

    VortexTransform::VortexTransform(Sf16Signal strength) {
        auto inputs = makeInputs(std::move(strength));
        state = std::make_shared<State>(std::move(inputs));
    }

    void VortexTransform::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        if (!context) {
            Serial.println("VortexTransform::advanceFrame context is null.");
        }
        state->strengthValue = state->strengthSignal.sample(state->strengthRange, elapsedMs);
    }

    UVMap VortexTransform::operator()(const UVMap &layer) const {
        return [state = this->state, layer](UV uv) {
            UV polar_uv = cartesianToPolarUV(uv);
            
            int32_t strength_raw = raw(state->strengthValue);
            uint32_t radius_raw = static_cast<uint32_t>(raw(polar_uv.v));
            int32_t scaled = static_cast<int32_t>((static_cast<int64_t>(strength_raw) * radius_raw) >> 16);
            int32_t new_angle = raw(polar_uv.u) + scaled;
            polar_uv.u = sr16(static_cast<uint16_t>(new_angle));

            return layer(polarToCartesianUV(polar_uv));
        };
    }
}
