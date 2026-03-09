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
#include "renderer/pipeline/signals/ranges/BipolarRange.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include <Arduino.h>

namespace PolarShader {
    namespace {
        f16 sampleClipMagnitude(const Sf16Signal &signal, TimeMillis elapsedMs) {
            static const BipolarRange<sf16> signedRange{sf16(SF16_MIN), sf16(SF16_MAX)};
            const int32_t signedRaw = raw(signal.sample(signedRange, elapsedMs));
            const uint32_t magnitude = (signedRaw < 0)
                ? static_cast<uint32_t>(-(static_cast<int64_t>(signedRaw)))
                : static_cast<uint32_t>(signedRaw);
            const uint32_t clamped = (magnitude > F16_MAX) ? F16_MAX : magnitude;
            return f16(static_cast<uint16_t>(clamped));
        }
    }

    struct PaletteTransform::MappedInputs {
        Sf16Signal offsetSignal;
        MagnitudeRange<uint8_t> offsetRange{0, 255};
        Sf16Signal clipSignal;
        f16 feather = f16(0);
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
        f16 feather,
        PipelineContext::PaletteClipPower clipPower
    ) {
        return MappedInputs{
            std::move(offset),
            MagnitudeRange<uint8_t>(0, 255),
            std::move(clipSignal),
            feather,
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
        f16 feather = f16(0);
        PipelineContext::PaletteClipPower clipPower = PipelineContext::PaletteClipPower::None;
        bool hasClip = false;

        explicit State(MappedInputs inputs)
            : offsetSignal(std::move(inputs.offsetSignal)),
              offsetRange(std::move(inputs.offsetRange)),
              clipSignal(std::move(inputs.clipSignal)),
              feather(inputs.feather),
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
        f16 feather,
        PipelineContext::PaletteClipPower clipPower
    ) {
        state = std::make_shared<State>(makeInputs(
            std::move(offset),
            std::move(clipSignal),
            feather,
            clipPower
        ));
    }

    void PaletteTransform::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        state->offsetValue = state->offsetSignal.sample(state->offsetRange, elapsedMs);
        if (context) {
            context->paletteOffset = state->offsetValue;
            if (state->hasClip) {
                f16 clipRaw = sampleClipMagnitude(state->clipSignal, elapsedMs);
                state->clipInvert = false;
                state->clipValue = PatternNormU16(raw(clipRaw));
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
