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
#include "renderer/pipeline/maths/PatternMaths.h"
#include "renderer/pipeline/maths/PolarMaths.h"
#include "renderer/pipeline/units/UnitConstants.h"
#include <Arduino.h>
#include "FastLED.h"

namespace PolarShader {
    PolarPipeline::PolarPipeline(
        std::unique_ptr<BasePattern> pattern,
        const CRGBPalette16 &palette,
        fl::vector<PipelineStep> steps,
        const char *name,
        std::shared_ptr<PipelineContext> context,
        DepthSignal depthSignal
    ) : pattern(std::move(pattern)),
        palette(palette),
        steps(std::move(steps)),
        name(name ? name : "unnamed"),
        context(std::move(context)),
        depthSignal(std::move(depthSignal)) {
        Serial.print("Building pipeline: ");
        Serial.println(this->name);

        if (!this->context) this->context = std::make_shared<PipelineContext>();
        if (pattern) pattern->setContext(this->context);

        for (auto &step: this->steps) {
            if (step.cartesianTransform) step.cartesianTransform->setContext(this->context);
            if (step.polarTransform) step.polarTransform->setContext(this->context);
            if (step.paletteTransform) step.paletteTransform->setContext(this->context);
        }
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

    CRGB PolarPipeline::mapPalette(
        const CRGBPalette16 &palette,
        PatternNormU16 value,
        const std::shared_ptr<PipelineContext> &context
    ) {
        uint16_t hue_value = raw(value);
        uint16_t mask_value = FRACT_Q0_16_MAX;
        if (context && context->paletteClipEnabled) {
            uint16_t clip_input = hue_value;
            if (context->paletteClipInvert) {
                clip_input = static_cast<uint16_t>(FRACT_Q0_16_MAX - clip_input);
            }
            switch (context->paletteClipPower) {
                case PipelineContext::PaletteClipPower::Quartic: {
                    // Strong shaping: only the very top of the noise survives the threshold.
                    uint16_t n2 = scale16(clip_input, clip_input);
                    clip_input = scale16(n2, n2);
                    break;
                }
                case PipelineContext::PaletteClipPower::Square: {
                    // Moderate shaping: reduces mid values so fewer areas pass the threshold.
                    clip_input = scale16(clip_input, clip_input);
                    break;
                }
                case PipelineContext::PaletteClipPower::None:
                default:
                    // No shaping; clip compares directly against the raw pattern value.
                    break;
            }

            uint16_t clip = raw(context->paletteClip);
            uint16_t feather = raw(context->paletteClipFeather);
            if (feather == 0) {
                mask_value = (clip_input < clip) ? 0 : FRACT_Q0_16_MAX;
            } else {
                uint32_t edge1 = static_cast<uint32_t>(clip) + feather;
                if (edge1 > FRACT_Q0_16_MAX) edge1 = FRACT_Q0_16_MAX;
                mask_value = raw(patternSmoothstepU16(
                    clip,
                    static_cast<uint16_t>(edge1),
                    clip_input
                ));
            }
        }

        uint8_t index = map16_to_8(hue_value);
        if (context) {
            index = static_cast<uint8_t>(index + context->paletteOffset);
        }
        CRGB color = ColorFromPalette(palette, index, 255, LINEARBLEND);
        if (context && context->paletteClipEnabled && mask_value != FRACT_Q0_16_MAX) {
            color.nscale8_video(static_cast<uint8_t>(mask_value >> 8));
        }
        return color;
    }

    void PolarPipeline::advanceFrame(TimeMillis timeInMillis) {
        if (!context) {
            Serial.println("PolarPipeline::advanceFrame context is null.");
        } else {
            context->depth = depthSignal(timeInMillis);
        }

        for (const auto &step: steps) {
            if (step.cartesianTransform) {
                step.cartesianTransform->advanceFrame(timeInMillis);
            }
            if (step.polarTransform) {
                step.polarTransform->advanceFrame(timeInMillis);
            }
            if (step.paletteTransform) {
                step.paletteTransform->advanceFrame(timeInMillis);
            }
        }
    }

    ColourLayer PolarPipeline::build() const {
        if (steps.empty()) return blackLayer("PolarPipeline::build has no steps.");
        if (!pattern) return blackLayer("PolarPipeline::build has no base pattern.");

        CartesianLayer currentCartesian;
        PolarLayer currentPolar;
        bool hasCartesian = false;
        bool hasPolar = false;

        if (pattern->domain() == PatternDomain::Cartesian) {
            currentCartesian = static_cast<CartesianPattern *>(pattern.get())->layer(context);
            hasCartesian = true;
        } else {
            currentPolar = static_cast<PolarPattern *>(pattern.get())->layer(context);
            hasPolar = true;
        }

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
                case PipelineStepKind::Palette:
                    break;
            }
        }

        if (!hasPolar) return blackLayer("PolarPipeline::build ended without a Polar layer.");

        // Final stage: sample the pattern value and map it to a color from the palette.
        return [palette = palette, layer = std::move(currentPolar), context = context](
            FracQ0_16 angle,
            FracQ0_16 radius
        ) {
            PatternNormU16 value = layer(angle, radius);
            return mapPalette(palette, value, context);
        };
    }
}
