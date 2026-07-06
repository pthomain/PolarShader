//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#include "composer/EmbeddedPscPlaylist.h"

#include "FastLED.h"
#include "composer/SceneCodec.h"

#if __has_include("PscPlaylistAssets.h")
#include "PscPlaylistAssets.h"
#else
namespace PolarShader::composer {
    inline constexpr const EmbeddedPscScene *kEmbeddedPscScenes = nullptr;
    inline constexpr std::size_t kEmbeddedPscSceneCount = 0;
}
#endif

namespace PolarShader::composer {
    EmbeddedPscPlaylistProvider::EmbeddedPscPlaylistProvider(
        const EmbeddedPscScene *scenes,
        std::size_t sceneCount,
        TimeMillis durationMs)
        : scenes(scenes),
          sceneCount(sceneCount),
          durationMs(durationMs) {
    }

    std::unique_ptr<Scene> EmbeddedPscPlaylistProvider::nextScene() {
        if (!scenes || sceneCount == 0) {
            return nullptr;
        }

        std::size_t start = random16(static_cast<uint16_t>(sceneCount));
        if (sceneCount > 1 && hasLastIndex && start == lastIndex) {
            start = (start + 1 + random16(static_cast<uint16_t>(sceneCount - 1))) % sceneCount;
        }

        for (std::size_t attempt = 0; attempt < sceneCount; ++attempt) {
            const std::size_t index = (start + attempt) % sceneCount;
            DecodeStatus status = DecodeStatus::OK;
            auto scene = decodeSceneWithDuration(
                scenes[index].bytes,
                scenes[index].size,
                durationMs,
                &status
            );

            if (scene && status == DecodeStatus::OK) {
                lastIndex = index;
                hasLastIndex = true;
                return scene;
            }
        }

        return nullptr;
    }

    bool hasEmbeddedPscPlaylist() {
        return kEmbeddedPscScenes != nullptr && kEmbeddedPscSceneCount > 0;
    }

    std::unique_ptr<SceneProvider> makeEmbeddedPscPlaylistProvider(TimeMillis durationMs) {
        if (!hasEmbeddedPscPlaylist()) {
            return nullptr;
        }
        return std::make_unique<EmbeddedPscPlaylistProvider>(
            kEmbeddedPscScenes,
            kEmbeddedPscSceneCount,
            durationMs
        );
    }
}
