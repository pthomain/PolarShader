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
#include "renderer/pipeline/transforms/polar/RotationTransform.h"
#include "renderer/pipeline/transforms/cartesian/TranslationTransform.h"
#include "renderer/pipeline/transforms/cartesian/ZoomTransform.h"
#include "renderer/pipeline/transforms/polar/KaleidoscopeTransform.h"
#include "renderer/pipeline/transforms/polar/RadialKaleidoscopeTransform.h"
#include "renderer/pipeline/transforms/polar/VortexTransform.h"
#include "renderer/pipeline/patterns/BasePattern.h"
#include <utility>

#include "renderer/pipeline/patterns/Patterns.h"
#include "renderer/pipeline/transforms/cartesian/CartesianTilingTransform.h"
#include "renderer/pipeline/transforms/cartesian/DomainWarpPresets.h"

namespace PolarShader {
    namespace {
        PolarPipelineBuilder makeBuilder(
            std::unique_ptr<BasePattern> pattern,
            const CRGBPalette16 &palette,
            const char *name
        ) {
            return {std::move(pattern), palette, name};
        }
    }

    PolarPipelineBuilder defaultPreset(
        const CRGBPalette16 &palette
    ) {
        return makeBuilder(
                    noisePattern(),
                    palette,
                    "kaleidoscope"
                )
                .setDepthSignal(
                    noise(cPerMil(100))
                )
                .addPolarTransform(KaleidoscopeTransform(3, true))
                .addCartesianTransform(ZoomTransform(noise(cPerMil(100))))
                .addCartesianTransform(CartesianTilingTransform(
                    noise(cPerMil(200)),
                    200,
                    2000,
                    false
                ))
                .addPolarTransform(RotationTransform(
                    noise(cPerMil(100))
                ));
    }

    PolarPipelineBuilder hexKaleidoscopePreset(
        const CRGBPalette16 &palette
    ) {
        return makeBuilder(
                    hexTilingPattern(
                        10000,
                        32,
                        1000
                    ), palette,
                    "kaleidoscope"
                )
                .addPaletteTransform(PaletteTransform(noise(cPerMil(200))))
                .addCartesianTransform(TranslationTransform(
                    noise(),
                    noise(cPerMil(100), cPerMil(300))
                ))
                .addCartesianTransform(ZoomTransform(
                    pulse(cPerMil(30))
                ))
                .addPolarTransform(VortexTransform(
                    noise(cPerMil(10), cPerMil(200))
                ))
                .addPolarTransform(KaleidoscopeTransform(4, true))
                .addPolarTransform(RotationTransform(
                    noise(cPerMil(100))
                ));
    }

    PolarPipelineBuilder noiseKaleidoscopePattern(
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
                .addCartesianTransform(TranslationTransform(
                    noise(),
                    noise(cPerMil(30), cPerMil(200))
                ))
                .addCartesianTransform(ZoomTransform(
                    // noise(cPerMil(100))
                    noise(cPerMil(100), cPerMil(100), cPerMil(100))
                ))
                .addPolarTransform(VortexTransform(
                    noise(cPerMil(10), cPerMil(200))
                ))
                .addPolarTransform(RadialKaleidoscopeTransform(4, true))
                .addPolarTransform(RotationTransform(
                    noise(cPerMil(100))
                ));
    }
}
