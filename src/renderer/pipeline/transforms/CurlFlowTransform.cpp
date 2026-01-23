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

#include "CurlFlowTransform.h"
#include <cstring>
#include "FastLED.h"
#include "../modulators/BoundUtils.h"
#include "renderer/pipeline/utils/MathUtils.h"

namespace PolarShader {
    namespace {
        constexpr UnboundedScalar kAmpMin = UnboundedScalar::fromRaw(-static_cast<int32_t>(Q16_16_ONE / 4));
        constexpr UnboundedScalar kAmpMax = UnboundedScalar::fromRaw(Q16_16_ONE / 4);
    }

    struct CurlFlowTransform::State {
        BoundedScalarSignal amplitudeSignal;
        uint8_t sampleShift;
        UnboundedScalar amplitudeValue = kAmpMin;

        State(BoundedScalarSignal amp, uint8_t shift)
            : amplitudeSignal(std::move(amp)), sampleShift(shift) {
        }
    };

    CurlFlowTransform::CurlFlowTransform(BoundedScalarSignal amplitude, uint8_t sampleShift)
        : state(std::make_shared<State>(std::move(amplitude), sampleShift)) {
    }

    void CurlFlowTransform::advanceFrame(TimeMillis timeInMillis) {
        state->amplitudeValue = unbound(state->amplitudeSignal(timeInMillis), kAmpMin, kAmpMax);
    }

    CartesianLayer CurlFlowTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y) {
            const uint8_t shift = state->sampleShift;
            const int32_t dx = 256;
            auto sample_noise = [](int32_t sx, int32_t sy) -> int32_t {
                NoiseRawU16 noise_raw = NoiseRawU16(inoise16(sx, sy));
                return static_cast<int32_t>(raw(noise_raw)) - U16_HALF;
            };
            // Sample low-frequency noise for gradient
            int32_t n_x1 = sample_noise((x + dx) >> shift, y >> shift);
            int32_t n_x0 = sample_noise((x - dx) >> shift, y >> shift);
            int32_t n_y1 = sample_noise(x >> shift, (y + dx) >> shift);
            int32_t n_y0 = sample_noise(x >> shift, (y - dx) >> shift);

            int32_t grad_x = (n_x1 - n_x0); // approximate d/dx
            int32_t grad_y = (n_y1 - n_y0); // approximate d/dy

            // curl = (grad_y, -grad_x)
            RawQ16_16 amp = RawQ16_16(state->amplitudeValue.asRaw());
            // Gradients are large (~±65535); scale them down before applying amp to avoid extreme warps.
            constexpr uint8_t GRAD_SCALE_SHIFT = 8; // damp gradient magnitude by 256
            int64_t offset_x = (static_cast<int64_t>(grad_y) * raw(amp)) >> (16 + GRAD_SCALE_SHIFT);
            int64_t offset_y = (static_cast<int64_t>(-grad_x) * raw(amp)) >> (16 + GRAD_SCALE_SHIFT);

            uint32_t wrappedX = static_cast<uint32_t>(static_cast<int64_t>(x) + offset_x);
            uint32_t wrappedY = static_cast<uint32_t>(static_cast<int64_t>(y) + offset_y);

            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));

            return layer(finalX, finalY);
        };
    }
}
