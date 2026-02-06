//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#ifndef POLAR_SHADER_NATIVE_FASTLED_H
#define POLAR_SHADER_NATIVE_FASTLED_H

#include <cstdint>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace fl {
    template<typename T1, typename T2>
    struct pair {
        T1 first;
        T2 second;
    };
    using i32 = int32_t;
}

// Mock FastLED trig functions
static inline int16_t sin16(uint16_t theta) {
    double angle = (double)theta * 2.0 * M_PI / 65536.0;
    return (int16_t)(sin(angle) * 32767.0);
}

static inline int16_t cos16(uint16_t theta) {
    double angle = (double)theta * 2.0 * M_PI / 65536.0;
    return (int16_t)(cos(angle) * 32767.0);
}

#endif // POLAR_SHADER_NATIVE_FASTLED_H
