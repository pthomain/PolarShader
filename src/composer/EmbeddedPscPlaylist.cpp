//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#include "composer/EmbeddedPscPlaylist.h"

#include <utility>

#if __has_include("PscPlaylistAssets.h")
#include "PscPlaylistAssets.h"
#define POLAR_SHADER_HAVE_PSC_PLAYLIST_ASSETS_HEADER 1
#else
#define POLAR_SHADER_HAVE_PSC_PLAYLIST_ASSETS_HEADER 0
namespace PolarShader::composer {
    inline constexpr const EmbeddedPscScene *kEmbeddedPscScenes = nullptr;
    inline constexpr std::size_t kEmbeddedPscSceneCount = 0;
}
#endif

#ifndef POLAR_SHADER_HAS_EMBEDDED_PSC_PLAYLIST
#define POLAR_SHADER_HAS_EMBEDDED_PSC_PLAYLIST 1
#endif

#if defined(ARDUINO) && POLAR_SHADER_HAVE_PSC_PLAYLIST_ASSETS_HEADER && !POLAR_SHADER_HAS_EMBEDDED_PSC_PLAYLIST
#define POLAR_SHADER_COMPILE_EMBEDDED_PSC_DECODER 0
#else
#define POLAR_SHADER_COMPILE_EMBEDDED_PSC_DECODER 1
#endif

#if POLAR_SHADER_COMPILE_EMBEDDED_PSC_DECODER
#include "FastLED.h"
#include "composer/SceneCodec.h"
#endif

namespace PolarShader::composer {
    EmbeddedPscPlaylistProvider::EmbeddedPscPlaylistProvider(
        const EmbeddedPscScene *scenes,
        std::size_t sceneCount,
        TimeMillis durationMs,
        std::unique_ptr<SceneProvider> fallbackProvider)
        : scenes(scenes),
          sceneCount(sceneCount),
          durationMs(durationMs),
          fallbackProvider(std::move(fallbackProvider)) {
    }

    std::unique_ptr<Scene> EmbeddedPscPlaylistProvider::nextScene() {
#if POLAR_SHADER_COMPILE_EMBEDDED_PSC_DECODER
        if (!scenes || sceneCount == 0) {
            return fallbackProvider ? fallbackProvider->nextScene() : nullptr;
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

        return fallbackProvider ? fallbackProvider->nextScene() : nullptr;
#else
        return fallbackProvider ? fallbackProvider->nextScene() : nullptr;
#endif
    }

    bool hasEmbeddedPscPlaylist() {
        return kEmbeddedPscSceneCount > 0;
    }

    std::unique_ptr<SceneProvider> makeEmbeddedPscPlaylistProvider(
        TimeMillis durationMs,
        std::unique_ptr<SceneProvider> fallbackProvider) {
        if (!hasEmbeddedPscPlaylist()) {
            return nullptr;
        }
        return std::make_unique<EmbeddedPscPlaylistProvider>(
            kEmbeddedPscScenes,
            kEmbeddedPscSceneCount,
            durationMs,
            std::move(fallbackProvider)
        );
    }
}
