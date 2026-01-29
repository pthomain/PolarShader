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

#include "PaletteTransform.h"
#include "renderer/pipeline/ranges/PaletteRange.h"
#include <Arduino.h>

namespace PolarShader {
    struct PaletteTransform::MappedInputs {
        MappedSignal<uint8_t> offsetSignal;
    };

    struct PaletteTransform::State {
        MappedSignal<uint8_t> offsetSignal;
        MappedValue<uint8_t> offsetValue = MappedValue(static_cast<uint8_t>(0));

        explicit State(MappedSignal<uint8_t> s)
            : offsetSignal(std::move(s)) {
        }
    };

    PaletteTransform::MappedInputs PaletteTransform::makeInputs(SFracQ0_16Signal offset) {
        return MappedInputs{
            PaletteRange().mapSignal(std::move(offset))
        };
    }

    PaletteTransform::PaletteTransform(MappedSignal<uint8_t> offsetSignal)
        : state(std::make_shared<State>(std::move(offsetSignal))) {
    }

    PaletteTransform::PaletteTransform(MappedInputs inputs)
        : PaletteTransform(std::move(inputs.offsetSignal)) {
    }

    PaletteTransform::PaletteTransform(SFracQ0_16Signal offset)
        : PaletteTransform(makeInputs(std::move(offset))) {
    }

    void PaletteTransform::advanceFrame(TimeMillis timeInMillis) {
        state->offsetValue = state->offsetSignal(timeInMillis);
        if (context) {
            context->paletteOffset = state->offsetValue.get();
        } else {
            Serial.println("PaletteTransform::advanceFrame context is null.");
        }
    }
}
