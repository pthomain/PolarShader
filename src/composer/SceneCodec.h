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

#ifndef POLAR_SHADER_COMPOSER_SCENE_CODEC_H
#define POLAR_SHADER_COMPOSER_SCENE_CODEC_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include "renderer/scene/Scene.h"

namespace PolarShader::composer {
    // Wire format:
    //
    //   offset  size  field
    //   ─────── ────  ──────────────────────────────────────────────
    //   0       4     magic        "PSC\0"  (0x50 0x53 0x43 0x00)
    //   4       1     version      0x01 (built-in palette), 0x02 (raw embedded),
    //                              or 0x03 (cosine embedded)
    //   5       1     palette_id   composer palette ID (PaletteTable), or
    //                              0xFF under v2/v3 = embedded-palette sentinel
    //   [6      48]   palette      v2 + 0xFF only: 16 x (r,g,b) bytes
    //   [6      24]   palette      v3 + 0xFF only: 12 u16 cosine coefficients
    //   …       1     pattern_tag  see PATTERN_TAGS in SceneCodec.cpp
    //   …       2     pattern_len  little-endian body length
    //   …       …     pattern_body static-config bytes + signal slots
    //   …       1     transform_n  transform count
    //   …       …     transform[i] tag(1) + length(2) + static-config + signal slots
    //
    // Version 2 is emitted when a scene carries a raw embedded custom palette
    // (palette_id 0xFF followed by 48 raw bytes). Version 3 is emitted when a
    // scene carries cosine palette coefficients (a,b,c,d RGB triples as u16;
    // c is scaled over 0..4, all other coefficients over 0..1). Built-in
    // palettes stay v1 and are byte-identical to before. Under v1, palette_id
    // 0xFF is an unknown id (BAD_ENUM); the sentinel meaning applies only under
    // v2/v3.
    //
    // A Signal blob is recursive: signal_tag(1) + body_len(2) + body.
    //
    // Little-endian throughout. The format is display-agnostic — nothing
    // in the file ties a scene to fabric vs round.
    //
    // Tag tables MUST stay in lockstep with web/sketches/composer/schema.js.
    // Drift is caught by the cross-implementation golden fixture in
    // test/test_composer.

    enum class DecodeStatus : uint8_t {
        OK = 0,
        BAD_MAGIC,
        BAD_VERSION,
        TRUNCATED,
        UNKNOWN_TAG,
        BAD_ENUM,
        BAD_VALUE,
    };

    // Decode the bytes into a fresh Scene. Returns nullptr on failure;
    // *statusOut (if non-null) is set to a diagnostic code.
    //
    // The returned Scene has duration UINT32_MAX (never expires) so the
    // caller's SceneManager won't fall back to its provider.
    //
    // No allocations happen beyond what LayerBuilder/Scene already do.
    std::unique_ptr<Scene> decodeScene(const uint8_t *bytes,
                                       std::size_t len,
                                       DecodeStatus *statusOut = nullptr);

    // Decode the same .psc wire format but override the Scene duration.
    // Intended for embedded playlists where a decoded scene must expire so
    // SceneManager asks the provider for another entry.
    std::unique_ptr<Scene> decodeSceneWithDuration(const uint8_t *bytes,
                                                   std::size_t len,
                                                   TimeMillis durationMs,
                                                   DecodeStatus *statusOut = nullptr);
}

#endif // POLAR_SHADER_COMPOSER_SCENE_CODEC_H
