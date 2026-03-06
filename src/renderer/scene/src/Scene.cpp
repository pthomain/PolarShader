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

#include "renderer/scene/Scene.h"
#include <utility>

namespace PolarShader {
    Scene::Scene(fl::vector<std::shared_ptr<Layer> > layers, TimeMillis durationMs)
        : layers(std::move(layers)),
          durationMs(durationMs) {
    }

    void Scene::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        for (auto &layer: layers) {
            layer->advanceFrame(progress, elapsedMs);
        }
    }

    bool Scene::isExpired(TimeMillis elapsedMs) const {
        if (durationMs == 0) return false;
        return elapsedMs >= durationMs;
    }

    namespace {
        CRGB blend(CRGB base, CRGB top, f16 alpha, BlendMode mode) {
            if (raw(alpha) == 0) return base;

            switch (mode) {
                case BlendMode::Normal: {
                    return blend(base, top, static_cast<uint8_t>(raw(alpha) >> 8));
                }
                case BlendMode::Add: {
                    if (raw(alpha) != 0xFFFFu) {
                        top.nscale8_video(static_cast<uint8_t>(raw(alpha) >> 8));
                    }
                    return base + top;
                }
                case BlendMode::Multiply: {
                    if (raw(alpha) != 0xFFFFu) {
                        top.nscale8_video(static_cast<uint8_t>(raw(alpha) >> 8));
                    }
                    CRGB result;
                    result.r = (static_cast<uint16_t>(base.r) * top.r) >> 8;
                    result.g = (static_cast<uint16_t>(base.g) * top.g) >> 8;
                    result.b = (static_cast<uint16_t>(base.b) * top.b) >> 8;
                    return result;
                }
                case BlendMode::Screen: {
                    if (raw(alpha) != 0xFFFFu) {
                        top.nscale8_video(static_cast<uint8_t>(raw(alpha) >> 8));
                    }
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

    void Scene::compile() {
        for (auto &coreLayers: compiledLayers) {
            coreLayers.clear();
            coreLayers.reserve(layers.size());
        }

        for (auto &coreLayers: compiledLayers) {
            for (const auto &layer: layers) {
                coreLayers.push_back(CompositedLayer{
                    layer->compile(),
                    layer->getAlpha(),
                    layer->getBlendMode()
                });
            }
        }
    }

    CRGB Scene::sample(uint8_t coreIndex, f16 angle, f16 radius) const {
        if (layers.empty()) {
            return CRGB::Black;
        }

        const auto &coreLayers = compiledLayers[coreIndex == 0 ? 0 : 1];
        CRGB result = CRGB::Black;
        for (const auto &entry: coreLayers) {
            if (!entry.map) continue;
            CRGB layerColor = (*entry.map)(angle, radius);
            result = blend(result, layerColor, entry.alpha, entry.blendMode);
        }
        return result;
    }
}
