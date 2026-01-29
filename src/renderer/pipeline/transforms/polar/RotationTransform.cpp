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
#include "renderer/pipeline/signals/Accumulators.h"
#include <Arduino.h>

namespace PolarShader {
    struct RotationTransform::State {
        SFracQ0_16Signal angleSignal;
        MappedSignal<FracQ0_16> angleOffset = MappedSignal(FracQ0_16(0));

        explicit State(SFracQ0_16Signal s)
            : angleSignal(std::move(s)) {
        }
    };

    RotationTransform::RotationTransform(SFracQ0_16Signal angle)
        : PolarTransform(PolarRange()),
          state(std::make_shared<State>(std::move(angle))) {
    }

    void RotationTransform::advanceFrame(TimeMillis timeInMillis) {
        if (!context) {
            Serial.println("RotationTransform::advanceFrame context is null.");
        }
        state->angleOffset = mapPolar(state->angleSignal(timeInMillis));
    }

    PolarLayer RotationTransform::operator()(const PolarLayer &layer) const {
        return [state = this->state, layer](FracQ0_16 angle, FracQ0_16 radius) {
            uint16_t angle_raw = raw(angle);
            uint16_t offset_raw = raw(state->angleOffset.get());
            uint16_t new_angle = static_cast<uint16_t>(angle_raw + offset_raw);
            return layer(FracQ0_16(new_angle), radius);
        };
    }
}
