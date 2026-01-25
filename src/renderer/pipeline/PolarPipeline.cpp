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

#include "PolarPipeline.h"
#include "renderer/pipeline/maths/NoiseMaths.h"
#include "renderer/pipeline/maths/PolarMaths.h"
#include <Arduino.h>
#include "FastLED.h"

namespace PolarShader {
    PolarPipeline::PolarPipeline(
        NoiseLayer sourceLayer,
        const CRGBPalette16 &palette,
        fl::vector<PipelineStep> steps,
        const char *name
    ) : sourceLayer(std::move(sourceLayer)),
        palette(palette),
        steps(std::move(steps)),
        name(name ? name : "unnamed") {
        Serial.print("Building pipeline: ");
        Serial.println(this->name);
    }

    ColourLayer PolarPipeline::blackLayer(const char *reason) {
        if (reason) Serial.println(reason);
        return [](FracQ0_16, FracQ0_16) {
            return CRGB::Black;
        };
    }

    PolarLayer PolarPipeline::toPolarLayer(const CartesianLayer &layer) {
        return [layer](FracQ0_16 angle, FracQ0_16 radius) {
            auto [x, y] = polarToCartesian(angle, radius);
            // Convert Q0.16 cartesian into Q24.8 with extra fractional precision.
            return layer(
                CartQ24_8(x << CARTESIAN_FRAC_BITS),
                CartQ24_8(y << CARTESIAN_FRAC_BITS)
            );
        };
    }

    CartesianLayer PolarPipeline::toCartesianLayer(const PolarLayer &layer) {
        return [layer](CartQ24_8 x, CartQ24_8 y) {
            // Drop fractional bits to recover Q0.16 cartesian coords for polar conversion.
            int32_t x_q0_16 = static_cast<int32_t>(static_cast<int64_t>(raw(x)) >> CARTESIAN_FRAC_BITS);
            int32_t y_q0_16 = static_cast<int32_t>(static_cast<int64_t>(raw(y)) >> CARTESIAN_FRAC_BITS);
            auto [angle, radius] = cartesianToPolar(x_q0_16, y_q0_16);
            return layer(angle, radius);
        };
    }

    void PolarPipeline::advanceFrame(TimeMillis timeInMillis) {
        for (const auto &step: steps) {
            if (step.cartesianTransform) {
                step.cartesianTransform->advanceFrame(timeInMillis);
            }
            if (step.polarTransform) {
                step.polarTransform->advanceFrame(timeInMillis);
            }
        }
    }

    ColourLayer PolarPipeline::build() const {
        if (steps.empty()) return blackLayer("PolarPipeline::build has no steps.");

        CartesianLayer adaptedSource = [sourceLayer = sourceLayer](CartQ24_8 x, CartQ24_8 y) {
            // Perform addition in signed space to correctly offset negative coordinates,
            // then cast to unsigned for the noise function with explicit wrap.
            int64_t offset = static_cast<int64_t>(NOISE_DOMAIN_OFFSET) << CARTESIAN_FRAC_BITS;
            int64_t sx = static_cast<int64_t>(raw(x)) + offset;
            int64_t sy = static_cast<int64_t>(raw(y)) + offset;
            uint32_t ux = static_cast<uint32_t>(sx);
            uint32_t uy = static_cast<uint32_t>(sy);
            return sourceLayer(CartesianUQ24_8(ux), CartesianUQ24_8(uy));
        };

        CartesianLayer currentCartesian = adaptedSource;
        PolarLayer currentPolar;
        bool hasCartesian = true;
        bool hasPolar = false;

        // Apply transforms in the order they were added by the builder.
        for (const auto &step: steps) {
            switch (step.kind) {
                case PipelineStepKind::Cartesian: {
                    if (!hasCartesian)
                        return blackLayer("PolarPipeline::build Cartesian step without Cartesian layer.");
                    if (!step.cartesianTransform) return blackLayer("Cartesian step missing transform.");

                    currentCartesian = (*step.cartesianTransform)(currentCartesian);
                    break;
                }
                case PipelineStepKind::Polar: {
                    if (!hasPolar) return blackLayer("PolarPipeline::build Polar step without Polar layer.");
                    if (!step.polarTransform) return blackLayer("Polar step missing transform.");

                    currentPolar = (*step.polarTransform)(currentPolar);
                    break;
                }
                case PipelineStepKind::ToCartesian: {
                    if (!hasPolar) return blackLayer("PolarPipeline::build ToCartesian without Polar layer.");

                    currentCartesian = toCartesianLayer(currentPolar);
                    hasCartesian = true;
                    hasPolar = false;
                    break;
                }
                case PipelineStepKind::ToPolar: {
                    if (!hasCartesian) return blackLayer("PolarPipeline::build ToPolar without Cartesian layer.");

                    currentPolar = toPolarLayer(currentCartesian);
                    hasPolar = true;
                    hasCartesian = false;
                    break;
                }
            }
        }

        if (!hasPolar) return blackLayer("PolarPipeline::build ended without a Polar layer.");

        // Final stage: sample the noise value and map it to a color from the palette.
        return [palette = palette, layer = currentPolar](
            FracQ0_16 angle,
            FracQ0_16 radius
        ) {
            NoiseNormU16 value = layer(angle, radius);
            uint8_t index = map16_to_8(raw(value));
            return ColorFromPalette(palette, index, 255, LINEARBLEND);
        };
    }
}
