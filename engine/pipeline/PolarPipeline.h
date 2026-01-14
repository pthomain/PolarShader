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

#ifndef LED_SEGMENTS_EFFECTS_DECORATORS_POLARPIPELINE_H
#define LED_SEGMENTS_EFFECTS_DECORATORS_POLARPIPELINE_H

#include "transforms/base/Transforms.h"
#ifdef ARDUINO
#include "Arduino.h"
#endif
#include "PolarUtils.h"
#include "utils/NoiseUtils.h"
#include <memory>

#include "PipelineStep.h"

struct PipelineStep;

namespace LEDSegments {
    /**
     * @brief Manages the rendering pipeline for polar effects.
     *
     * The PolarPipeline is responsible for chaining together a series of transforms
     * to transform coordinates and generate the final colour for each pixel. It handles
     * both Cartesian and Polar coordinate transforms.
     */
    class PolarPipeline {
        NoiseLayer sourceLayer;
        CRGBPalette16 palette;
        fl::vector<PipelineStep> steps;

        static ColourLayer blackLayer(const char *reason) {
#ifdef ARDUINO
            if (reason) Serial.println(reason);
#endif
            return [](uint16_t, fract16, unsigned long) {
                return CRGB::Black;
            };
        }

        static PolarLayer toPolarLayer(const CartesianLayer &layer) {
            return [layer](uint16_t angle_turns, fract16 radius, unsigned long t) {
                auto [x, y] = cartesianCoords(angle_turns, radius);
                return layer(x, y, t);
            };
        }

        static CartesianLayer toCartesianLayer(const PolarLayer &layer) {
            return [layer](int32_t x, int32_t y, unsigned long t) {
                auto [angle_turns, radius] = polarCoords(x, y);
                return layer(angle_turns, radius, t);
            };
        }

        PolarPipeline(
            NoiseLayer sourceLayer,
            const CRGBPalette16 &palette,
            fl::vector<PipelineStep> steps
        ) : sourceLayer(std::move(sourceLayer)),
            palette(palette),
            steps(std::move(steps)) {
        }

        friend class PolarPipelineBuilder;

    public:
        void advanceFrame(unsigned long timeInMillis) {
            for (const auto &step: steps) {
                if (step.cartesianTransform) {
                    step.cartesianTransform->advanceFrame(timeInMillis);
                }
                if (step.polarTransform) {
                    step.polarTransform->advanceFrame(timeInMillis);
                }
            }
        }

        ColourLayer build() const {
            if (steps.empty()) return blackLayer("PolarPipeline::build has no steps.");

            CartesianLayer adaptedSource = [sourceLayer = sourceLayer](int32_t x, int32_t y, unsigned long t) {
                return sourceLayer(
                    x + NOISE_DOMAIN_OFFSET,
                    y + NOISE_DOMAIN_OFFSET,
                    t
                );
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
            return [palette = palette, layer = currentPolar](uint16_t angle_turns, fract16 radius, unsigned long t) {
                uint16_t value = normaliseNoise16(layer(angle_turns, radius, t));
                uint8_t phase = 0; //t / 8;
                uint8_t index = map16_to_8(value) + phase;
                return ColorFromPalette(palette, index, 255, LINEARBLEND);
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_POLARPIPELINE_H
