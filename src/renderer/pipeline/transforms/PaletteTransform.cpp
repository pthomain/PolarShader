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
#include "renderer/pipeline/ranges/LinearRange.h"


#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include <Arduino.h>

namespace PolarShader {
    struct PaletteTransform::MappedInputs {
        MappedSignal<uint8_t> offsetSignal;
        MappedSignal<SFracQ0_16> clipSignal;
        FracQ0_16 feather = FracQ0_16(0);
        PipelineContext::PaletteClipPower clipPower = PipelineContext::PaletteClipPower::None;
        bool hasClip = false;
    };

    PaletteTransform::MappedInputs PaletteTransform::makeInputs(SFracQ0_16Signal offset) {
        return MappedInputs{
            resolveMappedSignal(LinearRange<uint8_t>(0, 255).mapSignal(std::move(offset)))
        };
    }

    PaletteTransform::MappedInputs PaletteTransform::makeInputs(
        SFracQ0_16Signal offset,
        SFracQ0_16Signal clipSignal,
        FracQ0_16 feather,
        PipelineContext::PaletteClipPower clipPower
    ) {
        return MappedInputs{
            resolveMappedSignal(LinearRange<uint8_t>(0, 255).mapSignal(std::move(offset))),
            resolveMappedSignal(LinearRange(SFracQ0_16(Q0_16_MIN), SFracQ0_16(Q0_16_MAX)).mapSignal(std::move(clipSignal))),
            feather,
            clipPower,
            true
        };
    }

    struct PaletteTransform::State {
        MappedSignal<uint8_t> offsetSignal;
        MappedValue<uint8_t> offsetValue = MappedValue(static_cast<uint8_t>(0));
        MappedSignal<SFracQ0_16> clipSignal;
        PatternNormU16 clipValue = PatternNormU16(0);
        bool clipInvert = false;
        FracQ0_16 feather = FracQ0_16(0);
        PipelineContext::PaletteClipPower clipPower = PipelineContext::PaletteClipPower::None;
        bool hasClip = false;

        explicit State(MappedInputs inputs)
            : offsetSignal(std::move(inputs.offsetSignal)),
              clipSignal(std::move(inputs.clipSignal)),
              feather(inputs.feather),
              clipPower(inputs.clipPower),
              hasClip(inputs.hasClip) {
        }
    };

    PaletteTransform::PaletteTransform(SFracQ0_16Signal offset) {
        state = std::make_shared<State>(makeInputs(std::move(offset)));
    }

    PaletteTransform::PaletteTransform(
        SFracQ0_16Signal offset,
        SFracQ0_16Signal clipSignal,
        FracQ0_16 feather,
        PipelineContext::PaletteClipPower clipPower
    ) {
        state = std::make_shared<State>(makeInputs(
            std::move(offset),
            std::move(clipSignal),
            feather,
            clipPower
        ));
    }

    void PaletteTransform::advanceFrame(FracQ0_16 progress, TimeMillis elapsedMs) {
        state->offsetValue = state->offsetSignal(progress, elapsedMs);
        if (context) {
            context->paletteOffset = state->offsetValue.get();
            if (state->hasClip) {
            SFracQ0_16 clipRaw = state->clipSignal(progress, elapsedMs).get();
                int32_t clipRawValue = raw(clipRaw);
                if (clipRawValue == 0) {
                    context->paletteClipEnabled = false;
                    context->paletteClipInvert = false;
                    return;
                }
                state->clipInvert = (clipRawValue < 0);
                int32_t mag = clipRawValue < 0 ? -clipRawValue : clipRawValue;
                uint32_t clipped = clamp_frac_raw(mag);
                LinearRange range(PatternNormU16(0), PatternNormU16(FRACT_Q0_16_MAX));
                state->clipValue = range.map(SFracQ0_16(static_cast<int32_t>(clipped))).get();
                context->paletteClip = state->clipValue;
                context->paletteClipFeather = state->feather;
                context->paletteClipPower = state->clipPower;
                context->paletteClipInvert = state->clipInvert;
                context->paletteClipEnabled = true;
            } else {
                context->paletteClipEnabled = false;
            }
        } else {
            Serial.println("PaletteTransform::advanceFrame context is null.");
        }
    }
}
