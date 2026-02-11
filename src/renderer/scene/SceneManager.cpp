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

#include "SceneManager.h"

namespace PolarShader {
    SceneManager::SceneManager(std::unique_ptr<SceneProvider> provider)
        : provider(std::move(provider)) {
    }

    void SceneManager::advanceFrame(TimeMillis currentTimeMs) {
        Serial.println("SceneManager");
        if (!currentScene || currentScene->isExpired(currentTimeMs - currentSceneStartTimeMs)) {
            currentScene = provider->nextScene();
            if (currentScene) {
                currentSceneStartTimeMs = currentTimeMs;
                currentMap = currentScene->build();
            } else {
                currentMap = [](FracQ0_16, FracQ0_16) { return CRGB::Black; };
            }
        }
        Serial.println("currentScene before");

        if (currentScene) {
            Serial.println("currentScene after");
            TimeMillis elapsed = currentTimeMs - currentSceneStartTimeMs;
            TimeMillis duration = currentScene->getDuration();
            FracQ0_16 progress;
            Serial.print("duration:");
            Serial.println(duration);

            if (duration == 0) {
                progress = FracQ0_16(0xFFFFu);
            } else {
                uint64_t p = (static_cast<uint64_t>(elapsed) * 0xFFFFu) / duration;
                if (p > 0xFFFFu) p = 0xFFFFu;
                progress = FracQ0_16(static_cast<uint16_t>(p));
            }
            currentScene->advanceFrame(progress, elapsed);
        }
    }

    ColourMap SceneManager::build() const {
        return currentMap;
    }
}
