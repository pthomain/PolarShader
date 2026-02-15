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

#include "renderer/pipeline/presets/Presets.h"
#include <renderer/pipeline/signals/Signals.h>
#include "renderer/pipeline/transforms/RotationTransform.h"
#include "renderer/pipeline/transforms/TranslationTransform.h"
#include "renderer/pipeline/transforms/ZoomTransform.h"
#include "renderer/pipeline/transforms/KaleidoscopeTransform.h"
#include "renderer/pipeline/transforms/RadialKaleidoscopeTransform.h"
#include "renderer/pipeline/transforms/VortexTransform.h"
#include "renderer/pipeline/patterns/base/UVPattern.h"
#include <utility>
#include "renderer/pipeline/patterns/Patterns.h"

namespace PolarShader {
    namespace {
        LayerBuilder makeBuilder(
            std::unique_ptr<UVPattern> pattern,
            const CRGBPalette16 &palette,
            const char *name
        ) {
            return {std::move(pattern), palette, name};
        }
    }

    LayerBuilder defaultPreset(
        const CRGBPalette16 &palette
    ) {
        return makeBuilder(
                    noisePattern(),
                    palette,
                    "kaleidoscope"
                )
                .addPaletteTransform(
                    PaletteTransform(
                        sine(),
                        sine(biMid(100), biMid(), biFloor()),
                        perMil(10)
                    )
                )
                .addTransform(TranslationTransform(
                    biMid(0),
                    biFloor(200)
                ))
                .addTransform(
                    ZoomTransform(
                        sine(
                            biMid(100),
                            biFloor(400),
                            biMid(100)
                        )
                    ));
                // .addTransform(KaleidoscopeTransform(
                //     4,
                //     true
                // ));
    }

    LayerBuilder hexKaleidoscopePreset(
        const CRGBPalette16 &palette
    ) {
        return makeBuilder(
                    hexTilingPattern(
                        10000,
                        32,
                        50
                    ), palette,
                    "kaleidoscope"
                )
                .addPaletteTransform(PaletteTransform(noise(biMid(200))))
                .addTransform(TranslationTransform(
                    noise(biMid(100)),
                    noise(biMid(100), biFloor(600))
                ))
                .addTransform(ZoomTransform(
                    quadraticInOut(10000)
                ))
                .addTransform(VortexTransform(
                    noise(biMid(10), biFloor(400))
                ))
                .addTransform(KaleidoscopeTransform(4, true))
                .addTransform(RotationTransform(
                    noise(biMid(100))
                ));
    }

    LayerBuilder noiseKaleidoscopePattern(
        const CRGBPalette16 &palette
    ) {
        return makeBuilder(
                    noisePattern(),
                    palette,
                    "kaleidoscope"
                )
                .setDepthSignal(
                    noise(biMid(20))
                )
                .addPaletteTransform(PaletteTransform(
                    noise(biMid(100)),
                    sine(biMid(100)),
                    perMil(10)
                ))
                .addTransform(TranslationTransform(
                    noise(biMid(100)),
                    noise(biMid(30), biFloor(400))
                ))
                .addTransform(ZoomTransform(
                    noise(biMid(100))
                ))
                .addTransform(VortexTransform(
                    noise(biMid(10), biFloor(1000))
                ))
                .addTransform(KaleidoscopeTransform(4, true))
                .addTransform(RotationTransform(
                    noise(biMid(10))
                ));
    }
}
