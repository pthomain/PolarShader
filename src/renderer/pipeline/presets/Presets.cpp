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
#include <renderer/pipeline/patterns/cartesian/NoisePattern.h>
#include <renderer/pipeline/signals/Signals.h>
#include "renderer/pipeline/PolarPipelineBuilder.h"
#include "renderer/pipeline/transforms/polar/RotationTransform.h"
#include <memory>
#include "renderer/pipeline/patterns/Patterns.h"
#include "renderer/pipeline/transforms/cartesian/DomainWarpPresets.h"
#include "renderer/pipeline/transforms/cartesian/DomainWarpTransform.h"
#include "renderer/pipeline/transforms/cartesian/TranslationTransform.h"
#include "renderer/pipeline/transforms/cartesian/ZoomTransform.h"
#include "renderer/pipeline/transforms/polar/KaleidoscopeTransform.h"
#include "renderer/pipeline/transforms/polar/VortexTransform.h"

namespace PolarShader {
    PolarPipeline defaultPreset(const CRGBPalette16 &palette) {
        return PolarPipelineBuilder(
                    // voronoiPattern(CartQ24_8(4 * WorleyCellUnit), WorleyAliasing::Fast),
                    noisePattern(),
                    palette,
                    "default"
                )
                .setDepthSignal(
                    noise(cPerMil(100))
                )
                // .addPaletteTransform(PaletteTransform(
                //     noise(cPerMil(50))
                // ))
                // .addCartesianTransform(domainWarpNested())
                // .setDepthSignal(sine(
                //     cPerMil(100),
                //     cPerMil(200)
                // ))
                .addCartesianTransform(TranslationTransform(
                    noise(),
                    cPerMil(100)
                ))
                .addCartesianTransform(ZoomTransform(
                    noise(
                        cPerMil(100),
                        cPerMil(250),
                        cPerMil(0)
                    )
                ))
                .addPolarTransform(VortexTransform(
                    sine(cPerMil(200))
                ))
                .addPolarTransform(KaleidoscopeTransform(6, false))
                .addPolarTransform(RotationTransform(
                    noise(cPerMil(100))
                ))
                .build();
    }
}
