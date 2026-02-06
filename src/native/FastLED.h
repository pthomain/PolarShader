//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#ifndef POLAR_SHADER_NATIVE_FASTLED_H
#define POLAR_SHADER_NATIVE_FASTLED_H

#include <cstdint>
#include <cmath>
#include <functional>

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
    using u8 = uint8_t;
    
    template<typename T>
    using function = std::function<T>;
}

struct CRGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    CRGB() : r(0), g(0), b(0) {}
    CRGB(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
};

// Mock FastLED trig functions
static inline int16_t sin16(uint16_t theta) {
    double angle = (double)theta * 2.0 * M_PI / 65536.0;
    return (int16_t)(sin(angle) * 32767.0);
}

static inline int16_t cos16(uint16_t theta) {
    double angle = (double)theta * 2.0 * M_PI / 65536.0;
    return (int16_t)(cos(angle) * 32767.0);
}

// Simple linear congruent generator or just return 0 for now
static inline uint16_t inoise16(uint16_t x) {
    return (x * 2053) + 13849;
}

#endif // POLAR_SHADER_NATIVE_FASTLED_H
