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

#include "Layer.h"
#include "renderer/pipeline/maths/PatternMaths.h"
#include "renderer/pipeline/maths/PolarMaths.h"
#include "renderer/pipeline/maths/units/Units.h"
#ifdef ARDUINO
#include <Arduino.h>
#include "FastLED.h"
#else
#include "native/Arduino.h"
#include "native/FastLED.h"
#endif

namespace PolarShader {
    Layer::Layer(
        std::unique_ptr<UVPattern> pattern,
        const CRGBPalette16 &palette,
        fl::vector<PipelineStep> steps,
        const char *name,
        std::shared_ptr<PipelineContext> context,
        DepthSignal depthSignal,
        UQ0_16 alpha,
        BlendMode blendMode
    ) : pattern(std::move(pattern)),
        palette(palette),
        steps(std::move(steps)),
        name(name ? name : "unnamed"),
        context(std::move(context)),
        depthSignal(std::move(depthSignal)),
        alpha(alpha),
        blendMode(blendMode) {
        Serial.print("Building layer: ");
        Serial.println(this->name);

        if (!this->context) this->context = std::make_shared<PipelineContext>();
        if (pattern) pattern->setContext(this->context);

        for (auto &step: this->steps) {
            if (step.uvTransform) step.uvTransform->setContext(this->context);
            if (step.paletteTransform) step.paletteTransform->setContext(this->context);
        }
    }

    ColourMap Layer::blackLayer(const char *reason) {
        if (reason) Serial.println(reason);
        return [](UQ0_16, UQ0_16) {
            return CRGB::Black;
        };
    }

    CRGB Layer::mapPalette(
        const CRGBPalette16 &palette,
        PatternNormU16 value,
        const std::shared_ptr<PipelineContext> &context
    ) {
        uint16_t hue_value = raw(value);
        uint16_t mask_value = USQ0_16_MAX;
        if (context && context->paletteClipEnabled) {
            uint16_t clip_input = hue_value;
            if (context->paletteClipInvert) {
                clip_input = static_cast<uint16_t>(USQ0_16_MAX - clip_input);
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
                mask_value = (clip_input < clip) ? 0 : USQ0_16_MAX;
            } else {
                uint32_t edge1 = static_cast<uint32_t>(clip) + feather;
                if (edge1 > USQ0_16_MAX) edge1 = USQ0_16_MAX;
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
        uint8_t brightness = map16_to_8(hue_value);
        if (context && context->paletteClipEnabled) {
            brightness = 255;
        }
        CRGB color = ColorFromPalette(palette, index, brightness, LINEARBLEND);
        if (context && context->paletteClipEnabled && mask_value != USQ0_16_MAX) {
            color.nscale8_video(static_cast<uint8_t>(mask_value >> 8));
        }
        return color;
    }

    void Layer::advanceFrame(UQ0_16 progress, TimeMillis elapsedMs) {
        if (!context) {
            Serial.println("Layer::advanceFrame context is null.");
        } else {
            context->depth = depthSignal(progress, elapsedMs);
        }

        for (const auto &step: steps) {
            if (step.uvTransform) {
                step.uvTransform->advanceFrame(progress, elapsedMs);
            }
            if (step.paletteTransform) {
                step.paletteTransform->advanceFrame(progress, elapsedMs);
            }
        }
    }

    ColourMap Layer::build() const {
        if (!pattern) return blackLayer("Layer::build has no base pattern.");

        UVMap currentUV = pattern->layer(context);

        // Apply transforms in order
        for (const auto &step: steps) {
            if (step.kind == PipelineStepKind::UV) {
                if (!step.uvTransform) return blackLayer("UV step missing transform.");
                currentUV = (*step.uvTransform)(currentUV);
            }
        }

        // Final stage: map UV back to Polar domain for the display, 
        // then map pattern value to a color from the palette.
        return [palette = palette, layer = std::move(currentUV), context = context](
            UQ0_16 angle,
            UQ0_16 radius
        ) {
            // Display provides (Angle, Radius) in legacy Q0.16.
            // Convert to UV (Q16.16).
            UV input = polarToCartesianUV(UV(
                SQ16_16(raw(angle)),
                SQ16_16(raw(radius))
            ));

            PatternNormU16 value = layer(input);
            return mapPalette(palette, value, context);
        };
    }
}
