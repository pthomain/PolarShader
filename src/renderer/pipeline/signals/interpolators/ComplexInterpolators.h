//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_INTERPOLATORS_COMPLEX_INTERPOLATORS_H
#define POLAR_SHADER_PIPELINE_SIGNALS_INTERPOLATORS_COMPLEX_INTERPOLATORS_H

#include "Interpolator.h"
#include "renderer/pipeline/maths/ScalarMaths.h"

namespace PolarShader {
    /**
     * @brief Simple elastic out implementation.
     */
    class ElasticInterpolatorOut : public Interpolator {
    public:
        FracQ0_16 calculate(FracQ0_16 progress) const override {
            uint32_t p = raw(progress);
            if (p == 0) return FracQ0_16(0);
            if (p == 0xFFFFu) return FracQ0_16(0xFFFFu);

            // Rough fixed-point approximation of elastic out
            // sin(13 * PI/2 * p) * 2^(10 * (p-1)) + 1
            // For now, let's use a simpler spring-like behavior or just a dummy until we have better transcendental mocks
            // Using a damped sine wave approximation
            uint16_t theta = (p * 5) & 0xFFFFu; // ~5 cycles
            int16_t s = sin16(theta);
            
            uint32_t decay = 0xFFFFu - p;
            decay = (decay * decay) >> 16;
            decay = (decay * decay) >> 16; // Strong decay
            
            int32_t offset = (static_cast<int32_t>(s) * static_cast<int32_t>(decay)) >> 15;
            int32_t result = static_cast<int32_t>(p) + offset;
            
            if (result < 0) return FracQ0_16(0);
            if (result > 0xFFFF) return FracQ0_16(0xFFFFu);
            return FracQ0_16(static_cast<uint16_t>(result));
        }

        Interpolator* clone() const override { return new ElasticInterpolatorOut(*this); }
    };

    /**
     * @brief Simple bounce out implementation.
     */
    class BounceInterpolatorOut : public Interpolator {
    public:
        FracQ0_16 calculate(FracQ0_16 progress) const override {
            uint32_t p = raw(progress);
            if (p < 0x366Bu) { // 1/2.75
                return FracQ0_16(static_cast<uint16_t>((75625LL * p * p) >> 32));
            } else if (p < 0x6CD9u) { // 2/2.75
                p -= 0x51EBu; // 1.5/2.75
                return FracQ0_16(static_cast<uint16_t>(((75625LL * p * p) >> 32) + 0xC000u));
            } else if (p < 0x9249u) { // 2.5/2.75
                p -= 0x82EEu; // 2.25/2.75
                return FracQ0_16(static_cast<uint16_t>(((75625LL * p * p) >> 32) + 0xF000u));
            } else {
                p -= 0xFA41u; // 2.625/2.75
                return FracQ0_16(static_cast<uint16_t>(((75625LL * p * p) >> 32) + 0xFAAAu));
            }
        }

        Interpolator* clone() const override { return new BounceInterpolatorOut(*this); }
    };
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_INTERPOLATORS_COMPLEX_INTERPOLATORS_H
