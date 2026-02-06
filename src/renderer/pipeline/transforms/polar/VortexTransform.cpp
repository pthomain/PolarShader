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

#include "VortexTransform.h"
#include "renderer/pipeline/ranges/SFracRange.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/maths/PolarMaths.h"
#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif

namespace PolarShader {
    struct VortexTransform::MappedInputs {
        MappedSignal<SFracQ0_16> strengthSignal;
    };

    VortexTransform::MappedInputs VortexTransform::makeInputs(SFracQ0_16Signal strength) {
        return MappedInputs{
            resolveMappedSignal(SFracRange().mapSignal(std::move(strength)))
        };
    }

    struct VortexTransform::State {
        MappedSignal<SFracQ0_16> strengthSignal;
        MappedValue<SFracQ0_16> strengthValue = MappedValue(SFracQ0_16(0));

        explicit State(MappedSignal<SFracQ0_16> s)
            : strengthSignal(std::move(s)) {
        }
    };

    VortexTransform::VortexTransform(MappedSignal<SFracQ0_16> strength)
        : state(std::make_shared<State>(std::move(strength))) {
    }

    VortexTransform::VortexTransform(MappedInputs inputs)
        : VortexTransform(std::move(inputs.strengthSignal)) {
    }

    VortexTransform::VortexTransform(SFracQ0_16Signal strength)
        : VortexTransform(makeInputs(std::move(strength))) {
    }

    void VortexTransform::advanceFrame(TimeMillis timeInMillis) {
        if (!context) {
            Serial.println("VortexTransform::advanceFrame context is null.");
        }
        state->strengthValue = state->strengthSignal(timeInMillis);
    }

    PolarLayer VortexTransform::operator()(const PolarLayer &layer) const {
        return [state = this->state, layer](FracQ0_16 angle, FracQ0_16 radius) {
            int32_t strength_raw = raw(state->strengthValue.get());
            uint32_t radius_raw = raw(radius);
            int32_t scaled = static_cast<int32_t>((static_cast<int64_t>(strength_raw) * radius_raw) >> 16);
            int32_t new_angle = static_cast<int32_t>(raw(angle)) + scaled;
            uint16_t wrapped = static_cast<uint16_t>(new_angle);
            return layer(FracQ0_16(wrapped), radius);
        };
    }

    UVLayer VortexTransform::operator()(const UVLayer &layer) const {
        return [state = this->state, layer](UV uv) {
            UV polar_uv = cartesianToPolarUV(uv);
            
            int32_t strength_raw = raw(state->strengthValue.get());
            uint32_t radius_raw = static_cast<uint32_t>(raw(polar_uv.v));
            int32_t scaled = static_cast<int32_t>((static_cast<int64_t>(strength_raw) * radius_raw) >> 16);
            int32_t new_angle = static_cast<int32_t>(raw(polar_uv.u)) + scaled;
            polar_uv.u = FracQ16_16(static_cast<int32_t>(static_cast<uint16_t>(new_angle)));

            return layer(polarToCartesianUV(polar_uv));
        };
    }
}
