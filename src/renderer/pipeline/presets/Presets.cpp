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

#include "Presets.h"
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
#include "renderer/pipeline/transforms/CartesianTilingTransform.h"

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
                .addTransform(
                    TranslationTransform(
                        cPerMil(500),
                        cPerMil(200)
                    )
                );
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
                .addPaletteTransform(PaletteTransform(noise(cPerMil(200))))
                .addTransform(TranslationTransform(
                    noise(cPerMil(100)),
                    noise(cPerMil(100), cPerMil(300))
                ))
                .addTransform(ZoomTransform(
                    quadraticInOut(10000)
                ))
                .addTransform(VortexTransform(
                    noise(cPerMil(10), cPerMil(200))
                ))
                .addTransform(KaleidoscopeTransform(4, true))
                .addTransform(RotationTransform(
                    noise(cPerMil(100))
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
                    noise(cPerMil(20))
                )
                .addPaletteTransform(PaletteTransform(
                    noise(cPerMil(100)),
                    noise(cPerMil(100), cPerMil(300), cPerMil(50)),
                    perMil(50)
                ))
                .addTransform(TranslationTransform(
                    noise(cPerMil(100)),
                    noise(cPerMil(30), cPerMil(200))
                ))
                .addTransform(ZoomTransform(
                    // noise(cPerMil(100))
                    noise(cPerMil(100), cPerMil(100), cPerMil(100))
                ))
                .addTransform(VortexTransform(
                    noise(cPerMil(10), cPerMil(200))
                ))
                .addTransform(RadialKaleidoscopeTransform(4, true))
                .addTransform(RotationTransform(
                    noise(cPerMil(100))
                ));
    }
}
