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

#include "renderer/pipeline/transforms/PaletteTransform.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#if defined(ARDUINO) || defined(__EMSCRIPTEN__)
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif

namespace PolarShader {
    namespace {
        u0x16 sampleClipMagnitude(const S0x16Signal &signal, TimeMillis elapsedMs) {
            static const MagnitudeRange<u0x16> clipRange{u0x16(0), u0x16(U0X16_MAX)};
            return signal.sample(clipRange, elapsedMs);
        }

        u0x16 scaleClipFeather(u0x16 maxFeather, u0x16 clipMagnitude) {
            const uint16_t maxFeatherRaw = raw(maxFeather);
            const uint16_t clipRaw = raw(clipMagnitude);

            if (maxFeatherRaw == 0 || clipRaw == 0) return u0x16(0);
            if (clipRaw >= U0X16_MAX) return maxFeather;
            if (maxFeatherRaw >= U0X16_MAX) return clipMagnitude;

            uint32_t scaled = static_cast<uint32_t>(maxFeatherRaw) * static_cast<uint32_t>(clipRaw);
            scaled = (scaled + (U0X16_MAX / 2u)) / U0X16_MAX;
            if (scaled > U0X16_MAX) scaled = U0X16_MAX;
            return u0x16(static_cast<uint16_t>(scaled));
        }
    }

    struct PaletteTransform::MappedInputs {
        S0x16Signal offsetSignal;
        MagnitudeRange<uint8_t> offsetRange{0, 255};
        S0x16Signal clipSignal;
        u0x16 maxFeather = u0x16(0);
        bool hasClip = false;
        PipelineContext::PaletteTintMode tintMode = PipelineContext::PaletteTintMode::HueRemap;
    };

    PaletteTransform::MappedInputs PaletteTransform::makeInputs(
        S0x16Signal offset,
        S0x16Signal clipSignal,
        u0x16 maxFeather,
        PipelineContext::PaletteTintMode tintMode
    ) {
        return MappedInputs{
            std::move(offset),
            MagnitudeRange<uint8_t>(0, 255),
            std::move(clipSignal),
            maxFeather,
            true,
            tintMode
        };
    }

    struct PaletteTransform::State {
        S0x16Signal offsetSignal;
        MagnitudeRange<uint8_t> offsetRange;
        uint8_t offsetValue = 0;
        S0x16Signal clipSignal;
        PatternNormU0x16 clipValue = PatternNormU0x16(0);
        bool clipInvert = false;
        u0x16 maxFeather = u0x16(0);
        bool hasClip = false;
        PipelineContext::PaletteTintMode tintMode = PipelineContext::PaletteTintMode::HueRemap;

        explicit State(MappedInputs inputs)
            : offsetSignal(std::move(inputs.offsetSignal)),
              offsetRange(std::move(inputs.offsetRange)),
              clipSignal(std::move(inputs.clipSignal)),
              maxFeather(inputs.maxFeather),
              hasClip(inputs.hasClip),
              tintMode(inputs.tintMode) {
        }
    };

    PaletteTransform::PaletteTransform(
        S0x16Signal offset,
        S0x16Signal clipSignal,
        u0x16 maxFeather,
        PipelineContext::PaletteTintMode tintMode
    ) {
        state = std::make_shared<State>(makeInputs(
            std::move(offset),
            std::move(clipSignal),
            maxFeather,
            tintMode
        ));
    }

    void PaletteTransform::advanceFrame(u0x16 progress, TimeMillis elapsedMs) {
        state->offsetValue = state->offsetSignal.sample(state->offsetRange, elapsedMs);
        if (context) {
            context->paletteOffset = state->offsetValue;
            if (state->hasClip) {
                u0x16 clipRaw = sampleClipMagnitude(state->clipSignal, elapsedMs);
                u0x16 clipFeather = scaleClipFeather(state->maxFeather, clipRaw);
                state->clipInvert = false;
                state->clipValue = PatternNormU0x16(raw(clipRaw));
                context->paletteClip = state->clipValue;
                context->paletteClipFeather = clipFeather;
                context->paletteClipInvert = state->clipInvert;
                context->paletteClipEnabled = true;
                context->paletteTintMode = state->tintMode;
            } else {
                context->paletteClipEnabled = false;
                context->paletteTintMode = PipelineContext::PaletteTintMode::HueRemap;
            }
        } else {
            Serial.println("PaletteTransform::advanceFrame context is null.");
        }
    }
}
