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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_ANGULARSIGNALS_H
#define POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_ANGULARSIGNALS_H

#include <utility>
#include "ScalarSignals.h"
#include "renderer/pipeline/maths/AngleMaths.h"

namespace PolarShader {
    /**
     * @brief Time-indexed angular signal bounded to AngleQ0_16.
     */
    using AngleQ0_16Signal = fl::function<AngleQ0_16(TimeMillis)>;

    /**
     * @brief Constant angular phase from AngleQ0_16.
     */
    inline AngleQ0_16Signal constant(AngleQ0_16 angle) {
        return [angle](TimeMillis) { return angle; };
    }

    inline AngleQ0_16Signal sine(
        FracQ0_16Signal phaseVelocity,
        FracQ0_16Signal amplitude,
        FracQ0_16Signal offset = constant(FracQ0_16(U16_HALF))
    ) {
        auto signal = createSignal( //TODO use existing mapper
            [phaseVelocity = std::move(phaseVelocity)](TimeMillis time) {
                return UnboundedScalar(raw(phaseVelocity(time)));
            },
            std::move(amplitude),
            std::move(offset),
            [](AngleQ0_16 phase) -> TrigQ0_16 {
                return angleSinQ0_16(phase);
            }
        );
        return [signal = std::move(signal)](TimeMillis time) {
            return AngleQ0_16(raw(signal(time)));
        };
    }

    inline AngleQ0_16Signal pulse(
        FracQ0_16Signal phaseVelocity,
        FracQ0_16Signal amplitude,
        FracQ0_16Signal offset = constant(FracQ0_16(U16_HALF))
    ) {
        auto signal = createSignal(//TODO use existing mapper
            [phaseVelocity = std::move(phaseVelocity)](TimeMillis time) {
                return UnboundedScalar(raw(phaseVelocity(time)));
            },
            std::move(amplitude),
            std::move(offset),
            [](AngleQ0_16 phase) -> TrigQ0_16 {
                uint16_t saw_raw = raw(phase);
                uint16_t pulse_raw = (saw_raw < HALF_TURN_U16)
                                         ? static_cast<uint16_t>(saw_raw << 1)
                                         : static_cast<uint16_t>((ANGLE_U16_MAX - saw_raw) << 1);
                int16_t signedPulse = static_cast<int16_t>(static_cast<int32_t>(pulse_raw) - U16_HALF);
                return TrigQ0_16(static_cast<int32_t>(signedPulse) << 1);
            }
        );
        return [signal = std::move(signal)](TimeMillis time) {
            return AngleQ0_16(raw(signal(time)));
        };
    }
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_ANGULARSIGNALS_H
