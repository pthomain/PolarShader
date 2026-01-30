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
#include "renderer/pipeline/ranges/PatternRange.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include <Arduino.h>

namespace PolarShader {
    struct PaletteTransform::MappedInputs {
        MappedSignal<uint8_t> offsetSignal;
        SFracQ0_16Signal clipSignal;
        FracQ0_16 feather = FracQ0_16(0);
        PipelineContext::PaletteClipPower clipPower = PipelineContext::PaletteClipPower::None;
        PipelineContext::PaletteBrightnessMode brightnessMode = PipelineContext::PaletteBrightnessMode::Pattern;
        bool hasClip = false;
    };

    struct PaletteTransform::State {
        MappedSignal<uint8_t> offsetSignal;
        MappedValue<uint8_t> offsetValue = MappedValue(static_cast<uint8_t>(0));
        SFracQ0_16Signal clipSignal;
        PatternNormU16 clipValue = PatternNormU16(0);
        bool clipInvert = false;
        FracQ0_16 feather = FracQ0_16(0);
        PipelineContext::PaletteClipPower clipPower = PipelineContext::PaletteClipPower::None;
        PipelineContext::PaletteBrightnessMode brightnessMode = PipelineContext::PaletteBrightnessMode::Pattern;
        bool hasClip = false;

        explicit State(MappedInputs inputs)
            : offsetSignal(std::move(inputs.offsetSignal)),
              clipSignal(std::move(inputs.clipSignal)),
              feather(inputs.feather),
              clipPower(inputs.clipPower),
              brightnessMode(inputs.brightnessMode),
              hasClip(inputs.hasClip) {
        }
    };

    PaletteTransform::MappedInputs PaletteTransform::makeInputs(SFracQ0_16Signal offset) {
        return MappedInputs{
            PaletteRange().mapSignal(std::move(offset))
        };
    }

    PaletteTransform::MappedInputs PaletteTransform::makeInputs(
        SFracQ0_16Signal offset,
        SFracQ0_16Signal clipSignal,
        FracQ0_16 feather,
        PipelineContext::PaletteClipPower clipPower,
        PipelineContext::PaletteBrightnessMode brightnessMode
    ) {
        return MappedInputs{
            PaletteRange().mapSignal(std::move(offset)),
            std::move(clipSignal),
            feather,
            clipPower,
            brightnessMode,
            true
        };
    }

    PaletteTransform::PaletteTransform(MappedSignal<uint8_t> offsetSignal)
        : state(std::make_shared<State>(MappedInputs{std::move(offsetSignal)})) {
    }

    PaletteTransform::PaletteTransform(MappedInputs inputs)
        : state(std::make_shared<State>(std::move(inputs))) {
    }

    PaletteTransform::PaletteTransform(
        SFracQ0_16Signal offset,
        PipelineContext::PaletteBrightnessMode brightnessMode
    ) : PaletteTransform(makeInputs(std::move(offset))) {
        state->brightnessMode = brightnessMode;
    }

    PaletteTransform::PaletteTransform(
        SFracQ0_16Signal offset,
        SFracQ0_16Signal clipSignal,
        FracQ0_16 feather,
        PipelineContext::PaletteClipPower clipPower,
        PipelineContext::PaletteBrightnessMode brightnessMode
    ) : PaletteTransform(makeInputs(
        std::move(offset),
        std::move(clipSignal),
        feather,
        clipPower,
        brightnessMode
    )) {
    }

    void PaletteTransform::advanceFrame(TimeMillis timeInMillis) {
        state->offsetValue = state->offsetSignal(timeInMillis);
        if (context) {
            context->paletteOffset = state->offsetValue.get();
            context->paletteBrightnessMode = state->brightnessMode;
            if (state->hasClip) {
                SFracQ0_16 clipRaw = state->clipSignal(timeInMillis);
                int32_t clipRawValue = raw(clipRaw);
                if (clipRawValue == 0) {
                    context->paletteClipEnabled = false;
                    context->paletteClipInvert = false;
                    return;
                }
                state->clipInvert = (clipRawValue < 0);
                int32_t mag = clipRawValue < 0 ? -clipRawValue : clipRawValue;
                uint32_t clipped = clamp_frac_raw(mag);
                PatternRange range;
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
