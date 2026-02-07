//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#include "SceneManager.h"

namespace PolarShader {
    SceneManager::SceneManager(std::unique_ptr<SceneProvider> provider)
        : provider(std::move(provider)) {
    }

    void SceneManager::advanceFrame(TimeMillis currentTimeMs) {
        if (!currentScene || currentScene->isExpired(currentTimeMs)) {
            currentScene = provider->nextScene();
            if (currentScene) {
                currentScene->start(currentTimeMs);
                currentMap = currentScene->build();
            } else {
                currentMap = [](FracQ0_16, FracQ0_16) { return CRGB::Black; };
            }
        }

        if (currentScene) {
            currentScene->advanceFrame(currentTimeMs);
        }
    }

    ColourMap SceneManager::build() const {
        return currentMap;
    }
}
