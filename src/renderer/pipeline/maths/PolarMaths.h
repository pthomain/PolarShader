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

#ifndef POLAR_SHADER_PIPELINE_MATHS_POLARMATHS_H
#define POLAR_SHADER_PIPELINE_MATHS_POLARMATHS_H

#ifdef ARDUINO
#include "FastLED.h"
#else
#include "native/FastLED.h"
#endif
#include "renderer/pipeline/units/ScalarUnits.h"
#include "renderer/pipeline/units/UnitConstants.h"
#include "renderer/pipeline/units/UVUnits.h"

namespace PolarShader {
    /** @brief Converts normalized Polar UV (Angle=U, Radius=V) to Cartesian UV. */
    UV polarToCartesianUV(UV polar_uv);

    /** @brief Converts Cartesian UV to normalized Polar UV (Angle=U, Radius=V). */
    UV cartesianToPolarUV(UV cart_uv);

    /**
     * @brief Provides explicit, safe helpers for polar coordinate arithmetic.
     *
     * Using these helpers is crucial for avoiding seam artifacts when working with
     * angles that wrap around.
     */
    namespace PolarMaths {

        /**
         * @brief Calculates the shortest distance between two angles in a wraparound domain.
         *
         * For example, the shortest distance between 350 degrees and 10 degrees is 20 degrees,
         * not 340. This is essential for creating seamless polar patterns.
         *
         * @param a The first angle (FracQ0_16, 0-65535 representing 0-360 deg).
         * @param b The second angle (FracQ0_16).
         * @return The shortest distance between the angles (FracQ0_16, 0-32767).
         */
        inline FracQ0_16 shortest_angle_dist(FracQ0_16 a, FracQ0_16 b) {
            uint16_t dist = raw(a) > raw(b) ? raw(a) - raw(b) : raw(b) - raw(a);
            if (dist > U16_HALF) {
                uint32_t wrap = ANGLE_FULL_TURN_U32 - dist;
                dist = static_cast<uint16_t>(wrap);
            }
            return FracQ0_16(dist);
        }

    } // namespace PolarMaths
}

#endif // POLAR_SHADER_PIPELINE_MATHS_POLARMATHS_H
