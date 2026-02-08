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

#include "Scene.h"
#include <utility>

namespace PolarShader {
    Scene::Scene(fl::vector<std::shared_ptr<Layer> > layers, TimeMillis durationMs)
        : layers(std::move(layers)),
          durationMs(durationMs) {
    }

    void Scene::advanceFrame(FracQ0_16 progress, TimeMillis elapsedMs) {
        for (auto &layer: layers) {
            layer->advanceFrame(progress, elapsedMs);
        }
    }

    bool Scene::isExpired(TimeMillis elapsedMs) const {
        if (durationMs == 0) return false;
        return elapsedMs >= durationMs;
    }

    namespace {
        CRGB blend(CRGB base, CRGB top, FracQ0_16 alpha, BlendMode mode) {
            if (raw(alpha) == 0) return base;

            // Apply layer alpha to top colour
            if (raw(alpha) != 0xFFFFu) {
                top.nscale8_video(static_cast<uint8_t>(raw(alpha) >> 8));
            }

            switch (mode) {
                case BlendMode::Normal: {
                    // Simple alpha compositing (assuming top is already scaled by alpha)
                    // For Normal blend mode with explicit alpha, we usually do lerp.
                    // But here top is already premultiplied by alpha.
                    uint8_t a = static_cast<uint8_t>(raw(alpha) >> 8);
                    return blend(base, top, a);
                }
                case BlendMode::Add: {
                    return base + top;
                }
                case BlendMode::Multiply: {
                    CRGB result;
                    result.r = (static_cast<uint16_t>(base.r) * top.r) >> 8;
                    result.g = (static_cast<uint16_t>(base.g) * top.g) >> 8;
                    result.b = (static_cast<uint16_t>(base.b) * top.b) >> 8;
                    return result;
                }
                case BlendMode::Screen: {
                    CRGB result;
                    result.r = 255 - ((static_cast<uint16_t>(255 - base.r) * (255 - top.r)) >> 8);
                    result.g = 255 - ((static_cast<uint16_t>(255 - base.g) * (255 - top.g)) >> 8);
                    result.b = 255 - ((static_cast<uint16_t>(255 - base.b) * (255 - top.b)) >> 8);
                    return result;
                }
                default:
                    return top;
            }
        }
    }

    ColourMap Scene::build() const {
        if (layers.empty()) {
            return [](FracQ0_16, FracQ0_16) { return CRGB::Black; };
        }

        fl::vector<ColourMap> maps;
        for (const auto &layer: layers) {
            maps.push_back(layer->build());
        }

        return [layers = this->layers, maps = std::move(maps)](FracQ0_16 angle, FracQ0_16 radius) {
            CRGB result = CRGB::Black;
            for (size_t i = 0; i < layers.size(); ++i) {
                CRGB layerColor = maps[i](angle, radius);
                result = blend(result, layerColor, layers[i]->getAlpha(), layers[i]->getBlendMode());
            }
            return result;
        };
    }
}
