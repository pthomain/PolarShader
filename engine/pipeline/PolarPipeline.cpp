//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2023 Pierre Thomain

/*
 * This file is part of LED Segments.
 *
 * LED Segments is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LED Segments is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LED Segments. If not, see <https://www.gnu.org/licenses/>.
 */

#include "PolarPipeline.h"
#include "utils/NoiseUtils.h"
#include "PolarUtils.h"
#include "FastLED.h"

namespace LEDSegments {

    PolarPipeline::PolarPipeline(
        NoiseLayer sourceLayer,
        const CRGBPalette16 &palette,
        fl::vector<PipelineStep> steps
    ) : sourceLayer(std::move(sourceLayer)),
        palette(palette),
        steps(std::move(steps)) {
    }

    ColourLayer PolarPipeline::blackLayer(const char *reason) {
#ifdef ARDUINO
        if (reason) Serial.println(reason);
#endif
        return [](Units::PhaseTurnsUQ16_16, Units::FractQ0_16, Units::TimeMillis) {
            return CRGB::Black;
        };
    }

    PolarLayer PolarPipeline::toPolarLayer(const CartesianLayer &layer) {
        return [layer](Units::PhaseTurnsUQ16_16 angle_q16, Units::FractQ0_16 radius, Units::TimeMillis t) {
            auto [x, y] = cartesianCoords(angle_q16, radius);
            return layer(x, y, t);
        };
    }

    CartesianLayer PolarPipeline::toCartesianLayer(const PolarLayer &layer) {
        return [layer](int32_t x, int32_t y, Units::TimeMillis t) {
            auto [angle_q16, radius] = polarCoords(x, y);
            return layer(angle_q16, radius, t);
        };
    }

    void PolarPipeline::advanceFrame(Units::TimeMillis timeInMillis) {
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

        CartesianLayer adaptedSource = [sourceLayer = sourceLayer](int32_t x, int32_t y, Units::TimeMillis t) {
            // Perform addition in signed space to correctly offset negative coordinates,
            // then cast to unsigned for the noise function with explicit wrap.
            int64_t sx = static_cast<int64_t>(x) + static_cast<int64_t>(NOISE_DOMAIN_OFFSET);
            int64_t sy = static_cast<int64_t>(y) + static_cast<int64_t>(NOISE_DOMAIN_OFFSET);
            uint32_t ux = static_cast<uint32_t>(sx);
            uint32_t uy = static_cast<uint32_t>(sy);
            return sourceLayer(ux, uy, t);
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
        return [palette = palette, layer = currentPolar](Units::PhaseTurnsUQ16_16 angle_q16, Units::FractQ0_16 radius, Units::TimeMillis t) {
            Units::NoiseNormU16 value = layer(angle_q16, radius, t);
            uint8_t phase = 0; //t / 8;
            uint8_t index = map16_to_8(value) + phase;
            return ColorFromPalette(palette, index, 255, LINEARBLEND);
        };
    }
}
