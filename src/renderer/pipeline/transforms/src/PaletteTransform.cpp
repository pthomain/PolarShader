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
#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif

namespace PolarShader {
    namespace {
        f16 sampleClipMagnitude(const Sf16Signal &signal, TimeMillis elapsedMs) {
            static const MagnitudeRange<f16> clipRange{f16(0), f16(F16_MAX)};
            return signal.sample(clipRange, elapsedMs);
        }

        f16 scaleClipFeather(f16 maxFeather, f16 clipMagnitude) {
            const uint16_t maxFeatherRaw = raw(maxFeather);
            const uint16_t clipRaw = raw(clipMagnitude);

            if (maxFeatherRaw == 0 || clipRaw == 0) return f16(0);
            if (clipRaw >= F16_MAX) return maxFeather;
            if (maxFeatherRaw >= F16_MAX) return clipMagnitude;

            uint32_t scaled = static_cast<uint32_t>(maxFeatherRaw) * static_cast<uint32_t>(clipRaw);
            scaled = (scaled + (F16_MAX / 2u)) / F16_MAX;
            if (scaled > F16_MAX) scaled = F16_MAX;
            return f16(static_cast<uint16_t>(scaled));
        }
    }

    struct PaletteTransform::MappedInputs {
        Sf16Signal offsetSignal;
        MagnitudeRange<uint8_t> offsetRange{0, 255};
        Sf16Signal clipSignal;
        f16 maxFeather = f16(0);
        PipelineContext::PaletteClipPower clipPower = PipelineContext::PaletteClipPower::None;
        bool hasClip = false;
    };

    PaletteTransform::MappedInputs PaletteTransform::makeInputs(Sf16Signal offset) {
        return MappedInputs{
            std::move(offset)
        };
    }

    PaletteTransform::MappedInputs PaletteTransform::makeInputs(
        Sf16Signal offset,
        Sf16Signal clipSignal,
        f16 maxFeather,
        PipelineContext::PaletteClipPower clipPower
    ) {
        return MappedInputs{
            std::move(offset),
            MagnitudeRange<uint8_t>(0, 255),
            std::move(clipSignal),
            maxFeather,
            clipPower,
            true
        };
    }

    struct PaletteTransform::State {
        Sf16Signal offsetSignal;
        MagnitudeRange<uint8_t> offsetRange;
        uint8_t offsetValue = 0;
        Sf16Signal clipSignal;
        PatternNormU16 clipValue = PatternNormU16(0);
        bool clipInvert = false;
        f16 maxFeather = f16(0);
        PipelineContext::PaletteClipPower clipPower = PipelineContext::PaletteClipPower::None;
        bool hasClip = false;

        explicit State(MappedInputs inputs)
            : offsetSignal(std::move(inputs.offsetSignal)),
              offsetRange(std::move(inputs.offsetRange)),
              clipSignal(std::move(inputs.clipSignal)),
              maxFeather(inputs.maxFeather),
              clipPower(inputs.clipPower),
              hasClip(inputs.hasClip) {
        }
    };

    PaletteTransform::PaletteTransform(Sf16Signal offset) {
        state = std::make_shared<State>(makeInputs(std::move(offset)));
    }

    PaletteTransform::PaletteTransform(
        Sf16Signal offset,
        Sf16Signal clipSignal,
        f16 maxFeather,
        PipelineContext::PaletteClipPower clipPower
    ) {
        state = std::make_shared<State>(makeInputs(
            std::move(offset),
            std::move(clipSignal),
            maxFeather,
            clipPower
        ));
    }

    void PaletteTransform::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        state->offsetValue = state->offsetSignal.sample(state->offsetRange, elapsedMs);
        if (context) {
            context->paletteOffset = state->offsetValue;
            if (state->hasClip) {
                f16 clipRaw = sampleClipMagnitude(state->clipSignal, elapsedMs);
                f16 clipFeather = scaleClipFeather(state->maxFeather, clipRaw);
                state->clipInvert = false;
                state->clipValue = PatternNormU16(raw(clipRaw));
                context->paletteClip = state->clipValue;
                context->paletteClipFeather = clipFeather;
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
