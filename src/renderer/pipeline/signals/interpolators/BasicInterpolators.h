//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_INTERPOLATORS_BASIC_INTERPOLATORS_H
#define POLAR_SHADER_PIPELINE_SIGNALS_INTERPOLATORS_BASIC_INTERPOLATORS_H

#include "Interpolator.h"

namespace PolarShader {
    class LinearInterpolator : public Interpolator {
    public:
        FracQ0_16 calculate(FracQ0_16 progress) const override {
            return progress;
        }

        Interpolator* clone() const override { return new LinearInterpolator(*this); }
    };

    class QuadraticInterpolatorIn : public Interpolator {
    public:
        FracQ0_16 calculate(FracQ0_16 progress) const override {
            uint32_t p = raw(progress);
            uint32_t result = (p * p) >> 16;
            return FracQ0_16(static_cast<uint16_t>(result));
        }

        Interpolator* clone() const override { return new QuadraticInterpolatorIn(*this); }
    };

    class QuadraticInterpolatorOut : public Interpolator {
    public:
        FracQ0_16 calculate(FracQ0_16 progress) const override {
            uint32_t p = raw(progress);
            uint32_t inv = 0xFFFFu - p;
            uint32_t result = 0xFFFFu - ((inv * inv) >> 16);
            return FracQ0_16(static_cast<uint16_t>(result));
        }

        Interpolator* clone() const override { return new QuadraticInterpolatorOut(*this); }
    };

    class QuadraticInterpolatorInOut : public Interpolator {
    public:
        FracQ0_16 calculate(FracQ0_16 progress) const override {
            uint32_t p = raw(progress);
            if (p < 0x8000u) {
                uint32_t result = (p * p) >> 15;
                return FracQ0_16(static_cast<uint16_t>(result));
            } else {
                uint32_t inv = 0xFFFFu - p;
                uint32_t tail = (inv * inv) >> 15;
                return FracQ0_16(static_cast<uint16_t>(0xFFFFu - tail));
            }
        }

        Interpolator* clone() const override { return new QuadraticInterpolatorInOut(*this); }
    };
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_INTERPOLATORS_BASIC_INTERPOLATORS_H
