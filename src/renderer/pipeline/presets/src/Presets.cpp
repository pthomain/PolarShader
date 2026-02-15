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
                        sine(constant(100), constant(200), constant(400)),
                        perMil(10)
                    )
                )
                .addTransform(TranslationTransform(
                    constant(500),
                    constant(100)
                ))
                .addTransform(
                    ZoomTransform(
                        sine(
                            constant(550),
                            constant(200),
                            constant(550)
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
                .addPaletteTransform(PaletteTransform(noise(constant(600))))
                .addTransform(TranslationTransform(
                    noise(constant(550)),
                    noise(constant(550), constant(300))
                ))
                .addTransform(ZoomTransform(
                    quadraticInOut(10000)
                ))
                .addTransform(VortexTransform(
                    noise(constant(505), constant(200))
                ))
                .addTransform(KaleidoscopeTransform(4, true))
                .addTransform(RotationTransform(
                    noise(constant(550))
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
                    noise(constant(510))
                )
                .addPaletteTransform(PaletteTransform(
                    noise(constant(550)),
                    sine(constant(550)),
                    perMil(10)
                ))
                .addTransform(TranslationTransform(
                    noise(constant(550)),
                    noise(constant(515), constant(200))
                ))
                .addTransform(ZoomTransform(
                    noise(constant(550))
                ))
                .addTransform(VortexTransform(
                    noise(constant(505), constant(500))
                ))
                .addTransform(KaleidoscopeTransform(4, true))
                .addTransform(RotationTransform(
                    noise(constant(505))
                ));
    }
}
