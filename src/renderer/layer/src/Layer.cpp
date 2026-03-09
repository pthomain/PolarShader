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

#include "renderer/layer/Layer.h"
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
        f16 alpha,
        BlendMode blendMode
    ) : pattern(std::move(pattern)),
        palette(palette),
        steps(std::move(steps)),
        name(name ? name : "unnamed"),
        context(std::move(context)),
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

    std::unique_ptr<ColourMap> Layer::blackLayer(const char *reason) {
        if (reason) Serial.println(reason);
        return std::make_unique<ColourMap>([](f16, f16) {
            return CRGB::Black;
        });
    }

    CRGB Layer::mapPalette(
        const CRGBPalette16 &palette,
        PatternNormU16 value,
        const std::shared_ptr<PipelineContext> &context
    ) {
        uint16_t hue_value = raw(value);
        uint16_t mask_value = F16_MAX;
        if (context && context->paletteClipEnabled) {
            uint16_t clip_input = hue_value;
            if (context->paletteClipInvert) {
                clip_input = static_cast<uint16_t>(F16_MAX - clip_input);
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
                mask_value = (clip_input < clip) ? 0 : F16_MAX;
            } else {
                uint32_t edge1 = static_cast<uint32_t>(clip) + feather;
                if (edge1 > F16_MAX) edge1 = F16_MAX;
                mask_value = raw(patternSmoothstepU16(
                    clip,
                    static_cast<uint16_t>(edge1),
                    clip_input
                ));
            }
        }

        uint8_t index = fl::map16_to_8(hue_value);
        if (context) {
            index = static_cast<uint8_t>(index + context->paletteOffset);
        }

        CRGB color = ColorFromPalette(palette, index, 255, LINEARBLEND);
        if (context && context->paletteClipEnabled && mask_value != F16_MAX) {
            color.nscale8_video(static_cast<uint8_t>(mask_value >> 8));
        }
        return color;
    }

    void Layer::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        if (!context) {
            Serial.println("Layer::advanceFrame context is null.");
        } else {
            context->timeMs = elapsedMs;
        }

        if (pattern) {
            pattern->advanceFrame(progress, elapsedMs);
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

    std::unique_ptr<ColourMap> Layer::compile() const {
        if (!pattern) return blackLayer("Layer::compile has no base pattern.");

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
        return std::make_unique<ColourMap>([palette = palette, layer = std::move(currentUV), context = context](
            f16 angle,
            f16 radius
        ) {
            // Display provides (Angle, Radius) in legacy f16/sf16.
            // Convert to UV (fl::u16x16/fl::s16x16 (Q16.16)).
            UV input = polarToCartesianUV(UV(
                fl::s16x16::from_raw(raw(angle)),
                fl::s16x16::from_raw(raw(radius))
            ));

            PatternNormU16 value = layer(input);
            return mapPalette(palette, value, context);
        });
    }
}
