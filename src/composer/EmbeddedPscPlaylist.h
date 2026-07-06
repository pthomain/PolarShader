//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#ifndef POLAR_SHADER_COMPOSER_EMBEDDED_PSC_PLAYLIST_H
#define POLAR_SHADER_COMPOSER_EMBEDDED_PSC_PLAYLIST_H

#include <cstddef>
#include <cstdint>
#include <memory>

#include "renderer/scene/SceneProvider.h"

namespace PolarShader::composer {
    inline constexpr TimeMillis kDefaultEmbeddedPscDurationMs = 30000;

    struct EmbeddedPscScene {
        const char *name;
        const uint8_t *bytes;
        std::size_t size;
    };

    class EmbeddedPscPlaylistProvider : public SceneProvider {
        const EmbeddedPscScene *scenes;
        std::size_t sceneCount;
        TimeMillis durationMs;
        std::size_t lastIndex{0};
        bool hasLastIndex{false};

    public:
        EmbeddedPscPlaylistProvider(const EmbeddedPscScene *scenes,
                                    std::size_t sceneCount,
                                    TimeMillis durationMs = kDefaultEmbeddedPscDurationMs);

        std::unique_ptr<Scene> nextScene() override;

        std::size_t size() const { return sceneCount; }
    };

    bool hasEmbeddedPscPlaylist();

    std::unique_ptr<SceneProvider> makeEmbeddedPscPlaylistProvider(
        TimeMillis durationMs = kDefaultEmbeddedPscDurationMs);
}

#endif // POLAR_SHADER_COMPOSER_EMBEDDED_PSC_PLAYLIST_H
