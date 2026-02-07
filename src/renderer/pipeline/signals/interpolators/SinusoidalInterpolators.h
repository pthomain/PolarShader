//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_INTERPOLATORS_SINUSOIDAL_INTERPOLATORS_H
#define POLAR_SHADER_PIPELINE_SIGNALS_INTERPOLATORS_SINUSOIDAL_INTERPOLATORS_H

#include "Interpolator.h"
#include "renderer/pipeline/maths/ScalarMaths.h"

namespace PolarShader {
    class SinusoidalInterpolatorIn : public Interpolator {
    public:
        FracQ0_16 calculate(FracQ0_16 progress) const override {
            // 1 - cos(p * PI / 2)
            // p maps 0..1 to 0..16384 (PI/2 turns)
            uint16_t theta = raw(progress) >> 2; 
            int16_t c = cos16(theta); // 32767..0
            uint32_t result = 0x7FFFf - c; // 0..32767
            return FracQ0_16(static_cast<uint16_t>(result << 1));
        }

        Interpolator* clone() const override { return new SinusoidalInterpolatorIn(*this); }
    };

    class SinusoidalInterpolatorOut : public Interpolator {
    public:
        FracQ0_16 calculate(FracQ0_16 progress) const override {
            // sin(p * PI / 2)
            uint16_t theta = raw(progress) >> 2;
            int16_t s = sin16(theta); // 0..32767
            return FracQ0_16(static_cast<uint16_t>(static_cast<uint32_t>(s) << 1));
        }

        Interpolator* clone() const override { return new SinusoidalInterpolatorOut(*this); }
    };

    class SinusoidalInterpolatorInOut : public Interpolator {
    public:
        FracQ0_16 calculate(FracQ0_16 progress) const override {
            // 0.5 * (1 - cos(p * PI))
            uint16_t theta = raw(progress) >> 1; // 0..32768 (PI turns)
            int16_t c = cos16(theta); // 32767..-32767
            int32_t result = 0x7FFF - c; // 0..65534
            return FracQ0_16(static_cast<uint16_t>(result));
        }

        Interpolator* clone() const override { return new SinusoidalInterpolatorInOut(*this); }
    };
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_INTERPOLATORS_SINUSOIDAL_INTERPOLATORS_H
