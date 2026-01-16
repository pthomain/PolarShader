//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#ifndef LED_SEGMENTS_PIPELINE_PRESETPICKER_H
#define LED_SEGMENTS_PIPELINE_PRESETPICKER_H

#include "Presets.h"

namespace LEDSegments {
    /**
     * Utility to pick preset pipelines by name/index.
     */
    class PresetPicker {
    public:
        using PresetBuilder = PolarPipeline(*)(const CRGBPalette16 &);

        static PolarPipeline pickRandom(const CRGBPalette16 &palette);
    };
}

#endif //LED_SEGMENTS_PIPELINE_PRESETPICKER_H
