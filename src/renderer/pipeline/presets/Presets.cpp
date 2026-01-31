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
#include "renderer/pipeline/transforms/polar/VortexTransform.h"
#include "renderer/pipeline/patterns/BasePattern.h"
#include <utility>

namespace PolarShader {
    namespace {
        PolarPipelineBuilder makeBuilder(
            std::unique_ptr<BasePattern> pattern,
            const CRGBPalette16 &palette,
            const char *name
        ) {
            return {std::move(pattern), palette, name};
        }

        PolarPipelineBuilder buildKaleidoscope(
            std::unique_ptr<BasePattern> pattern,
            const CRGBPalette16 &palette
        ) {
            return makeBuilder(std::move(pattern), palette, "kaleidoscope")
                    .setDepthSignal(
                        noise(cPerMil(100))
                    )
                    .addPaletteTransform(PaletteTransform(
                        noise(cPerMil(200)),
                        cPerMil(0),
                        perMil(200)
                    ))
                    .addCartesianTransform(TranslationTransform(
                        noise(),
                        noise(cPerMil(100), cPerMil(30))
                    ))
                    .addCartesianTransform(ZoomTransform(
                        noise()
                    ))
                    .addPolarTransform(VortexTransform(
                        noise(cPerMil(100), cPerMil(200))
                    ))
                    // .addPolarTransform(KaleidoscopeTransform(6, true))
                    .addPolarTransform(RotationTransform(
                        noise(cPerMil(100), cPerMil(500))
                    ));
        }
    }

    PolarPipelineBuilder defaultPreset(
        std::unique_ptr<BasePattern> pattern,
        const CRGBPalette16 &palette
    ) {
        return buildKaleidoscope(std::move(pattern), palette);
    }

    PolarPipelineBuilder kaleidoscopePattern(
        std::unique_ptr<BasePattern> pattern,
        const CRGBPalette16 &palette
    ) {
        return buildKaleidoscope(std::move(pattern), palette);
    }
}
