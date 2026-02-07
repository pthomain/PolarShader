//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#ifndef POLAR_SHADER_NATIVE_FASTLED_H
#define POLAR_SHADER_NATIVE_FASTLED_H

#include <cstdint>
#include <cmath>
#include <functional>
#include <vector>

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

    template<typename T>
    using vector = std::vector<T>;
}

struct CRGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    CRGB() : r(0), g(0), b(0) {}
    CRGB(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
    
    static const CRGB Black;

    void nscale8_video(uint8_t scale) {
        r = (static_cast<uint16_t>(r) * scale) >> 8;
        g = (static_cast<uint16_t>(g) * scale) >> 8;
        b = (static_cast<uint16_t>(b) * scale) >> 8;
    }

    CRGB operator+(const CRGB& other) const {
        return CRGB(
            (static_cast<uint16_t>(r) + other.r > 255) ? 255 : r + other.r,
            (static_cast<uint16_t>(g) + other.g > 255) ? 255 : g + other.g,
            (static_cast<uint16_t>(b) + other.b > 255) ? 255 : b + other.b
        );
    }
};

inline const CRGB CRGB::Black = CRGB(0, 0, 0);

enum TBlendType { NOBLEND, LINEARBLEND };

struct CRGBPalette16 {
    CRGB entries[16];
    CRGBPalette16() {
        for (int i = 0; i < 16; ++i) entries[i] = CRGB::Black;
    }
};

static inline CRGB ColorFromPalette(const CRGBPalette16& pal, uint8_t index, uint8_t brightness, TBlendType blend) {
    return pal.entries[index % 16];
}

static inline uint8_t map16_to_8(uint16_t v) {
    return static_cast<uint8_t>(v >> 8);
}

static inline uint16_t scale16(uint16_t a, uint16_t b) {
    return static_cast<uint16_t>((static_cast<uint32_t>(a) * b) >> 16);
}

static inline CRGB blend(const CRGB& a, const CRGB& b, uint8_t amount) {
    uint8_t inv = 255 - amount;
    return CRGB(
        (static_cast<uint16_t>(a.r) * inv + static_cast<uint16_t>(b.r) * amount) >> 8,
        (static_cast<uint16_t>(a.g) * inv + static_cast<uint16_t>(b.g) * amount) >> 8,
        (static_cast<uint16_t>(a.b) * inv + static_cast<uint16_t>(b.b) * amount) >> 8
    );
}

extern const CRGBPalette16 Rainbow_gp;

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
static inline uint16_t inoise16(uint32_t x) {
    return (uint16_t)((x * 2053u) + 13849u);
}

static inline uint16_t inoise16(uint32_t x, uint32_t y) {
    return (uint16_t)((x * 2053u) ^ (y * 3037u) + 13849u);
}

static inline uint16_t inoise16(uint32_t x, uint32_t y, uint32_t z) {
    return (uint16_t)((x * 2053u) ^ (y * 3037u) ^ (z * 4093u) + 13849u);
}

static inline uint16_t random16() {
    static uint32_t seed = 42;
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    return (uint16_t)(seed >> 16);
}

#endif // POLAR_SHADER_NATIVE_FASTLED_H
