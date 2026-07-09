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
#if defined(ARDUINO) || defined(__EMSCRIPTEN__)
#include <Arduino.h>
#include "FastLED.h"
#else
#include "native/Arduino.h"
#include "native/FastLED.h"
#endif

namespace PolarShader {
    namespace {
        uint16_t scaleRgbChannel(uint16_t channel, uint16_t value, uint16_t mask) {
            uint16_t scaled = scale16(channel, value);
            if (mask != F16_MAX) scaled = scale16(scaled, mask);
            return scaled;
        }

        bool hasUvLayerMap(const UVLayer &layer) {
            switch (layer.kind) {
                case UVLayerKind::Palette:
                    return static_cast<bool>(layer.palette);
                case UVLayerKind::Rgb:
                    return static_cast<bool>(layer.rgb);
                case UVLayerKind::Scalar:
                default:
                    return static_cast<bool>(layer.scalar);
            }
        }

        PaletteSample rgbToPaletteSample(RgbSample sample) {
            const uint16_t r = raw(sample.red());
            const uint16_t g = raw(sample.green());
            const uint16_t b = raw(sample.blue());
            const uint16_t value = raw(sample.value());

            uint16_t maxc = r;
            if (g > maxc) maxc = g;
            if (b > maxc) maxc = b;

            uint16_t minc = r;
            if (g < minc) minc = g;
            if (b < minc) minc = b;

            const uint16_t effectiveValue = scale16(maxc, value);
            const uint16_t delta = static_cast<uint16_t>(maxc - minc);
            if (value <= 0x0100u || maxc <= 0x0100u) {
                return PaletteSample(PatternNormU16(0), PatternNormU16(0));
            }
            if (delta <= 0x0100u) {
                return PaletteSample(PatternNormU16(0), PatternNormU16(effectiveValue));
            }

            constexpr int32_t ONE_SIXTH_TURN = 10923;
            int32_t hue = 0;
            if (maxc == r) {
                hue = static_cast<int32_t>(
                    (static_cast<int64_t>(static_cast<int32_t>(g) - static_cast<int32_t>(b)) *
                     ONE_SIXTH_TURN) / delta
                );
            } else if (maxc == g) {
                hue = 2 * ONE_SIXTH_TURN + static_cast<int32_t>(
                    (static_cast<int64_t>(static_cast<int32_t>(b) - static_cast<int32_t>(r)) *
                     ONE_SIXTH_TURN) / delta
                );
            } else {
                hue = 4 * ONE_SIXTH_TURN + static_cast<int32_t>(
                    (static_cast<int64_t>(static_cast<int32_t>(r) - static_cast<int32_t>(g)) *
                     ONE_SIXTH_TURN) / delta
                );
            }

            hue %= static_cast<int32_t>(ANGLE_FULL_TURN_U32);
            if (hue < 0) hue += static_cast<int32_t>(ANGLE_FULL_TURN_U32);
            return PaletteSample(PatternNormU16(static_cast<uint16_t>(hue)), PatternNormU16(effectiveValue));
        }
    }

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
        if (this->pattern) this->pattern->setContext(this->context);

        for (auto &step: this->steps) {
            if (step.uvTransform) step.uvTransform->setContext(this->context);
            if (step.paletteTransform) step.paletteTransform->setContext(this->context);
        }
    }

    std::unique_ptr<ColourMap> Layer::blackLayer(const char *reason) {
        if (reason) Serial.println(reason);
        return std::make_unique<ColourMap>([](const RenderPoint&) {
            return CRGB::Black;
        });
    }

    uint16_t Layer::computeClipMask(
        const std::shared_ptr<PipelineContext> &context,
        uint16_t clip_input
    ) {
        if (!(context && context->paletteClipEnabled)) {
            return F16_MAX;
        }
        if (context->paletteClipInvert) {
            clip_input = static_cast<uint16_t>(F16_MAX - clip_input);
        }

        uint16_t clip = raw(context->paletteClip);
        uint16_t feather = raw(context->paletteClipFeather);
        if (feather == 0) {
            return (clip_input < clip) ? 0 : F16_MAX;
        }
        uint32_t edge1 = static_cast<uint32_t>(clip) + feather;
        if (edge1 > F16_MAX) edge1 = F16_MAX;
        return raw(patternSmoothstepU16(
            clip,
            static_cast<uint16_t>(edge1),
            clip_input
        ));
    }

    CRGB Layer::mapPalette(
        const CRGBPalette16 &palette,
        PatternNormU16 value,
        const std::shared_ptr<PipelineContext> &context
    ) {
        uint16_t hue_value = raw(value);
        uint16_t mask_value = computeClipMask(context, hue_value);
        const PipelineContext::PaletteTintMode mode =
            context ? context->paletteTintMode : PipelineContext::PaletteTintMode::HueRemap;
        const uint8_t offset = context ? context->paletteOffset : 0;

        if (mode == PipelineContext::PaletteTintMode::ColourMask) {
            // Colour-mask mode: paletteOffset selects a single tint colour for
            // the whole scene; the pattern value drives alpha (brightness),
            // further shaped by the clip mask. When the clip signal is 0 the
            // mask is fully open (F16_MAX), so alpha reduces to the raw value.
            CRGB color = ColorFromPalette(palette, offset, 255, LINEARBLEND);
            uint16_t alpha = scale16(hue_value, mask_value);
            color.nscale8_video(static_cast<uint8_t>(alpha >> 8));
            return color;
        }

        if (mode == PipelineContext::PaletteTintMode::Native) {
            // Scalar patterns emit no hue; native mode bypasses the palette and
            // renders the intensity as greyscale. Rainbow-skip does not apply to
            // scalar patterns — for them the palette IS the colour source.
            uint8_t v = fl::map16_to_8(hue_value);
            CRGB color = CRGB(v, v, v);
            if (context && context->paletteClipEnabled && mask_value != F16_MAX) {
                color.nscale8_video(static_cast<uint8_t>(mask_value >> 8));
            }
            return color;
        }

        // HueRemap: intensity indexes the palette (offset = phase) and also
        // drives brightness, so a zero-value ("black") pixel stays black after
        // being mapped through the palette instead of lighting up at full
        // brightness.
        uint8_t bright = fl::map16_to_8(hue_value);
        uint8_t index = static_cast<uint8_t>(bright + offset);

        CRGB color = ColorFromPalette(palette, index, bright, LINEARBLEND);
        if (context && context->paletteClipEnabled && mask_value != F16_MAX) {
            color.nscale8_video(static_cast<uint8_t>(mask_value >> 8));
        }
        return color;
    }

    CRGB Layer::tintPalette(
        const CRGBPalette16 &palette,
        PaletteSample sample,
        const std::shared_ptr<PipelineContext> &context
    ) {
        uint16_t value_raw = raw(sample.value());
        // Intensity channel gates clip/colour-mask, the same role the scalar
        // plays in mapPalette.
        uint16_t mask_value = computeClipMask(context, value_raw);
        const PipelineContext::PaletteTintMode mode =
            context ? context->paletteTintMode : PipelineContext::PaletteTintMode::HueRemap;
        const uint8_t offset = context ? context->paletteOffset : 0;
        const bool isRainbow = context && context->paletteIsRainbow;

        if (mode == PipelineContext::PaletteTintMode::ColourMask) {
            // Colour-mask mode deliberately overrides the emitted hue: a single
            // paletteOffset tint for the whole scene, with the value channel
            // (shaped by the clip mask) driving alpha. Matches mapPalette.
            CRGB color = ColorFromPalette(palette, offset, 255, LINEARBLEND);
            uint16_t alpha = scale16(value_raw, mask_value);
            color.nscale8_video(static_cast<uint8_t>(alpha >> 8));
            return color;
        }

        uint8_t bright = fl::map16_to_8(value_raw);
        uint8_t hue8 = static_cast<uint8_t>(fl::map16_to_8(raw(sample.hue())) + offset);

        // Native mode renders the emitted hue directly (offset = hue phase),
        // bypassing the palette. HueRemap onto the Rainbow palette is redundant
        // (the palette already is the hue wheel), so it renders natively too.
        if (mode == PipelineContext::PaletteTintMode::Native || isRainbow) {
            CRGB color = CHSV(hue8, 255, bright);
            if (context && context->paletteClipEnabled && mask_value != F16_MAX) {
                color.nscale8_video(static_cast<uint8_t>(mask_value >> 8));
            }
            return color;
        }

        // HueRemap: the emitted hue selects the palette entry (offset = phase)
        // and the emitted value drives brightness.
        CRGB color = ColorFromPalette(palette, hue8, bright, LINEARBLEND);
        if (context && context->paletteClipEnabled && mask_value != F16_MAX) {
            color.nscale8_video(static_cast<uint8_t>(mask_value >> 8));
        }
        return color;
    }

    CRGB Layer::mapRgb(
        const CRGBPalette16 &palette,
        RgbSample sample,
        const std::shared_ptr<PipelineContext> &context
    ) {
        const uint16_t value_raw = raw(sample.value());
        const uint16_t mask_value = computeClipMask(context, value_raw);
        const PipelineContext::PaletteTintMode mode =
            context ? context->paletteTintMode : PipelineContext::PaletteTintMode::HueRemap;
        const uint8_t offset = context ? context->paletteOffset : 0;

        if (mode == PipelineContext::PaletteTintMode::ColourMask) {
            CRGB color = ColorFromPalette(palette, offset, 255, LINEARBLEND);
            uint16_t alpha = scale16(value_raw, mask_value);
            color.nscale8_video(static_cast<uint8_t>(alpha >> 8));
            return color;
        }

        if (mode == PipelineContext::PaletteTintMode::HueRemap) {
            PaletteSample converted = rgbToPaletteSample(sample);
            if (raw(converted.value()) <= 0x0100u) return CRGB::Black;
            return tintPalette(palette, converted, context);
        }

        CRGB color(
            fl::map16_to_8(scaleRgbChannel(raw(sample.red()), value_raw, mask_value)),
            fl::map16_to_8(scaleRgbChannel(raw(sample.green()), value_raw, mask_value)),
            fl::map16_to_8(scaleRgbChannel(raw(sample.blue()), value_raw, mask_value))
        );
        return color;
    }

    void Layer::setRasterDisplayInfo(const RasterDisplayInfo &rasterDisplay) {
        if (!context) {
            context = std::make_shared<PipelineContext>();
        }
        context->rasterDisplay = rasterDisplay;
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

        if (pattern->domain() == PatternDomain::RasterGrid) {
            for (const auto &step: steps) {
                if (step.kind == PipelineStepKind::UV) {
                    return blackLayer("Raster pattern cannot be compiled with UV transforms.");
                }
            }

            if (pattern->emitsColour()) {
                RasterColourMap currentRasterColour = pattern->rasterColourLayer(context);
                if (currentRasterColour) {
                    return std::make_unique<ColourMap>([
                        palette = palette,
                        layer = std::move(currentRasterColour),
                        context = context
                    ](const RenderPoint &point) {
                        PaletteSample sample = layer(point.raster);
                        return tintPalette(palette, sample, context);
                    });
                }
            }

            RasterMap currentRaster = pattern->rasterLayer(context);
            if (!currentRaster) return blackLayer("Raster pattern returned no raster layer.");

            return std::make_unique<ColourMap>([palette = palette, layer = std::move(currentRaster), context = context](
                const RenderPoint &point
            ) {
                PatternNormU16 value = layer(point.raster);
                return mapPalette(palette, value, context);
            });
        }

        UVLayer currentUV = pattern->uvLayer(context);
        if (!hasUvLayerMap(currentUV)) return blackLayer("Continuous pattern returned no UV layer.");

        // Apply transforms in order
        for (const auto &step: steps) {
            if (step.kind == PipelineStepKind::UV) {
                if (!step.uvTransform) return blackLayer("UV step missing transform.");
                currentUV = step.uvTransform->apply(currentUV);
                if (!hasUvLayerMap(currentUV)) return blackLayer("UV transform returned no UV layer.");
            }
        }

        // Final stage: map UV back to Polar domain for the display,
        // then map pattern value to a color from the palette.
        return std::make_unique<ColourMap>([palette = palette, layer = std::move(currentUV), context = context](
            const RenderPoint &point
        ) {
            // Display provides (Angle, Radius) in legacy f16/sf16.
            // Convert to UV (fl::u16x16/fl::s16x16 (Q16.16)).
            UV input = polarToCartesianUV(UV(
                fl::s16x16::from_raw(raw(point.angle)),
                fl::s16x16::from_raw(raw(point.radius))
            ));

            switch (layer.kind) {
                case UVLayerKind::Palette:
                    return tintPalette(palette, layer.palette(input), context);
                case UVLayerKind::Rgb:
                    return mapRgb(palette, layer.rgb(input), context);
                case UVLayerKind::Scalar:
                default:
                    return mapPalette(palette, layer.scalar(input), context);
            }
        });
    }
}
