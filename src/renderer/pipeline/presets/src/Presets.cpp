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
#include "renderer/pipeline/transforms/TilingTransform.h"
#include "renderer/pipeline/transforms/KaleidoscopeTransform.h"
#include "renderer/pipeline/transforms/VortexTransform.h"
#include "renderer/pipeline/transforms/FlowFieldTransform.h"
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
                    // flurryPattern(
                    //     32,
                    //     3,
                    //     FlurryPattern::Shape::Line,
                    //     noise(),
                    //     noise(),
                    //     constant(500),
                    //     constant(500),
                    //     constant(600),
                    //     constant(800)
                    // ),
                    noisePattern(),
                    palette,
                    "kaleidoscope"
                )
                .addPaletteTransform(
                    PaletteTransform(
                        noise(),
                        constant(0)
                    )
                )
                .addTransform(
                    ZoomTransform(
                        // noise(constant(300), constant(100))
                        // noise(constant(10), constant(500), constant(500))
                    )
                )
                // .addTransform(
                //     TranslationTransform(
                //         noise(constant(30)),
                //         noise(constant(100), constant(50), constant(50))
                //     ))

                // .addTransform(KaleidoscopeTransform(
                //     2,
                //     true
                // ))
                //
                // .addTransform(VortexTransform(
                //     noise(
                //         constant(100),
                //         constant(250),
                //         constant(750)
                //     )
                // ))
                .addTransform(
                    RotationTransform(
                        noise(constant(100)),
                        true
                    )
                )
        ;
    }

    LayerBuilder fabricPreset(
        const CRGBPalette16 &palette
    ) {
        return makeBuilder(
                    noisePattern(constant(100)),
                    palette,
                    "kaleidoscope"
                )
                .addPaletteTransform(
                    PaletteTransform(
                        sine(),
                        noise(constant(300), constant(250), constant(750)),
                        perMil(500)
                    )
                )
                .addTransform(
                    ZoomTransform(
                        constant(500)
                    )
                )
                .addTransform(
                    TranslationTransform(
                        noise(constant(30)),
                        noise(constant(100), constant(25), constant(75))
                    ))
                .addTransform(VortexTransform(
                    noise(constant(100), constant(250), constant(750))
                ))
                .addTransform(KaleidoscopeTransform(
                    2,
                    true
                ))
                .addTransform(
                    RotationTransform(
                        noise(constant(50)),
                        false
                    )
                );
    }

    LayerBuilder hexKaleidoscopePreset(
        const CRGBPalette16 &palette
    ) {
        return makeBuilder(
                    // tilingPattern(
                    // 10000,
                    // 32,
                    // 50
                    // ),
                    noisePattern(),
                    palette,
                    "kaleidoscope"
                )
                .addPaletteTransform(PaletteTransform(noise(constant(600))))
                .addTransform(TranslationTransform(
                    noise(constant(550)),
                    noise(constant(550), constant(350), constant(650))
                ))
                .addTransform(ZoomTransform(
                    quadraticInOut(10000)
                ))
                .addTransform(VortexTransform(
                    noise(constant(505), constant(400), constant(600))
                ))
                .addTransform(KaleidoscopeTransform(6, true))
                .addTransform(RotationTransform(
                    noise(constant(550))
                ));
    }

    LayerBuilder noiseKaleidoscopePattern(
        const CRGBPalette16 &palette
    ) {
        return makeBuilder(
                    // tilingPattern(
                    // 100,
                    // 16
                    // ),
                    noisePattern(noise()),
                    palette,
                    "kaleidoscope"
                )
                .addPaletteTransform(
                    PaletteTransform(
                        sine(),
                        noise(constant(100), constant(50), constant(150)),
                        perMil(200)
                    ))
                .addTransform(
                    ZoomTransform(
                        noise(constant(10), constant(250), constant(750))
                    ))

                //Translation drifts into solid colour when placed after kaleidoscope
                .addTransform(TranslationTransform(
                    noise(constant(30)),
                    noise(constant(100), constant(25), constant(75))
                ))
                .addTransform(KaleidoscopeTransform(
                    4,
                    false
                ))

                .addTransform(VortexTransform(
                    noise(constant(100), constant(250), constant(750))
                ))
                .addTransform(
                    RotationTransform(
                        noise(
                            constant(200)
                        ),
                        true
                    )
                );
    }

    LayerBuilder flowFieldPreset(
        const CRGBPalette16 &palette
    ) {
        return makeBuilder(
                    flowFieldPattern(
                        32,
                        3,
                        FlowFieldPattern::EmitterMode::Both,
                        noise(),
                        noise(),
                        constant(500),
                        constant(500),
                        constant(600),
                        constant(600),
                        constant(300),
                        constant(500)
                    ),
                    palette,
                    "flowField"
                )
                .addPaletteTransform(
                    PaletteTransform(
                        noise(),
                        constant(0)
                    )
                )
                .addTransform(
                    ZoomTransform(
                        sine(constant(550))
                    )
                )
                .addTransform(
                    RotationTransform(
                        noise(constant(100)),
                        true
                    )
                );
    }

    LayerBuilder flowFieldDotsPreset(
        const CRGBPalette16 &palette
    ) {
        return makeBuilder(
                    flowFieldPattern(
                        32,
                        5,
                        FlowFieldPattern::EmitterMode::Dots,
                        constant(75),
                        constant(75),
                        constant(600),
                        constant(400),
                        constant(0),
                        constant(700),
                        constant(400),
                        constant(600)
                    ),
                    palette,
                    "flowFieldDots"
                )
                .addPaletteTransform(
                    PaletteTransform(
                        sine(constant(300)),
                        noise(constant(200), constant(300), constant(700)),
                        perMil(400)
                    )
                )
                .addTransform(
                    VortexTransform(
                        noise(constant(100), constant(250), constant(750))
                    )
                )
                .addTransform(KaleidoscopeTransform(
                    3,
                    true
                ))
                .addTransform(
                    RotationTransform(
                        noise(constant(50)),
                        false
                    )
                );
    }

    LayerBuilder flowFieldKaleidoscopePreset(
        const CRGBPalette16 &palette
    ) {
        return makeBuilder(
                    flowFieldPattern(
                        32,
                        2,
                        FlowFieldPattern::EmitterMode::Lissajous,
                        noise(constant(200)),
                        noise(constant(200)),
                        constant(700),
                        constant(600),
                        constant(500),
                        constant(500),
                        constant(0),
                        constant(0)
                    ),
                    palette,
                    "flowFieldKaleidoscope"
                )
                .addPaletteTransform(
                    PaletteTransform(
                        noise(constant(600))
                    )
                )
                .addTransform(TranslationTransform(
                    noise(constant(550)),
                    noise(constant(550), constant(350), constant(650))
                ))
                .addTransform(
                    ZoomTransform(
                        quadraticInOut(10000)
                    )
                )
                .addTransform(KaleidoscopeTransform(6, true))
                .addTransform(
                    RotationTransform(
                        noise(constant(550))
                    )
                );
    }
}
