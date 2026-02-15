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
                        sine(midPoint(), floor(), floor()),
                        midPoint(10)
                    )
                )
                .addTransform(TranslationTransform(
                    midPoint(0),
                    floor(200)
                ))
                .addTransform(
                    ZoomTransform(
                        sine(
                            midPoint(100),
                            floor(400),
                            midPoint(100)
                        )
                    ))
                .addTransform(KaleidoscopeTransform(
                    4,
                    true
                ));
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
                .addPaletteTransform(PaletteTransform(noise(midPoint(200))))
                .addTransform(TranslationTransform(
                    noise(midPoint(100)),
                    noise(midPoint(100), floor(600))
                ))
                .addTransform(ZoomTransform(
                    quadraticInOut(10000)
                ))
                .addTransform(VortexTransform(
                    noise(midPoint(10), floor(400))
                ))
                .addTransform(KaleidoscopeTransform(4, true))
                .addTransform(RotationTransform(
                    noise(midPoint(100))
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
                    noise(midPoint(20))
                )
                .addPaletteTransform(PaletteTransform(
                    noise(midPoint(100)),
                    sine(midPoint(100), floor()),
                    midPoint(10)
                ))
                .addTransform(TranslationTransform(
                    noise(midPoint(100)),
                    noise(midPoint(30), floor(400))
                ))
                .addTransform(ZoomTransform(
                    noise(midPoint(100))
                ))
                .addTransform(VortexTransform(
                    noise(midPoint(10), floor(1000))
                ))
                .addTransform(KaleidoscopeTransform(4, true))
                .addTransform(RotationTransform(
                    noise(midPoint(10))
                ));
    }
}
