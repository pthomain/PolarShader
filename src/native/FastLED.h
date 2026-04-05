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

    // Fixed-point stub types for native (non-Arduino) builds
    // These mirror FastLED's fl/stl/fixed_point.h types
    template<int FracBits, typename Rep>
    class fixed_base {
        Rep v_;
        constexpr explicit fixed_base(Rep r, int) : v_(r) {}  // raw-value constructor
    public:
        using rep_type = Rep;
        constexpr fixed_base() : v_(0) {}
        explicit constexpr fixed_base(float f) : v_(static_cast<Rep>(f * (Rep(1) << FracBits))) {}
        static constexpr fixed_base from_raw(Rep r) { return fixed_base(r, 0); }
        constexpr Rep raw() const { return v_; }
        constexpr int32_t to_int() const { return static_cast<int32_t>(v_ >> FracBits); }
        float to_float() const { return static_cast<float>(v_) / static_cast<float>(Rep(1) << FracBits); }
        fixed_base operator+(fixed_base o) const { return from_raw(v_ + o.v_); }
        fixed_base operator-(fixed_base o) const { return from_raw(v_ - o.v_); }
        fixed_base operator-() const { return from_raw(-v_); }
        fixed_base operator*(fixed_base o) const {
            int64_t t = static_cast<int64_t>(v_) * static_cast<int64_t>(o.v_);
            return from_raw(static_cast<Rep>(t >> FracBits));
        }
        fixed_base operator/(fixed_base o) const {
            if (o.v_ == 0) return from_raw(0);
            int64_t t = static_cast<int64_t>(v_) << FracBits;
            return from_raw(static_cast<Rep>(t / static_cast<int64_t>(o.v_)));
        }
        fixed_base operator>>(int n) const { return from_raw(v_ >> n); }
        fixed_base operator<<(int n) const { return from_raw(v_ << n); }
        fixed_base& operator+=(fixed_base o) { v_ += o.v_; return *this; }
        fixed_base& operator-=(fixed_base o) { v_ -= o.v_; return *this; }
        bool operator==(fixed_base o) const { return v_ == o.v_; }
        bool operator!=(fixed_base o) const { return v_ != o.v_; }
        bool operator<(fixed_base o) const { return v_ < o.v_; }
        bool operator>(fixed_base o) const { return v_ > o.v_; }
        bool operator<=(fixed_base o) const { return v_ <= o.v_; }
        bool operator>=(fixed_base o) const { return v_ >= o.v_; }
        static fixed_base sqrt(fixed_base x) {
            if (x.v_ <= 0) return from_raw(0);
            // Newton's method in float for the native stub.
            double val = static_cast<double>(x.v_) / static_cast<double>(Rep(1) << FracBits);
            double root = std::sqrt(val);
            return from_raw(static_cast<Rep>(root * static_cast<double>(Rep(1) << FracBits)));
        }
    };
    using s16x16 = fixed_base<16, int32_t>;
    using u16x16 = fixed_base<16, uint32_t>;
    using s24x8  = fixed_base<8,  int32_t>;
    using u24x8  = fixed_base<8,  uint32_t>;

    static inline uint8_t map16_to_8(uint16_t v) {
        return static_cast<uint8_t>(v >> 8);
    }
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

    CRGB& operator+=(const CRGB& other) {
        r = (static_cast<uint16_t>(r) + other.r > 255) ? 255 : r + other.r;
        g = (static_cast<uint16_t>(g) + other.g > 255) ? 255 : g + other.g;
        b = (static_cast<uint16_t>(b) + other.b > 255) ? 255 : b + other.b;
        return *this;
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

extern const CRGBPalette16 RainbowColors_p;
extern const CRGBPalette16 CloudColors_p;
extern const CRGBPalette16 PartyColors_p;
extern const CRGBPalette16 ForestColors_p;
extern const CRGBPalette16 Rainbow_gp;

// Define them so we don't get linker errors in native tests
#ifndef POLAR_SHADER_UNIT_TEST_PALETTES
#define POLAR_SHADER_UNIT_TEST_PALETTES
inline const CRGBPalette16 RainbowColors_p = CRGBPalette16();
inline const CRGBPalette16 CloudColors_p = CRGBPalette16();
inline const CRGBPalette16 PartyColors_p = CRGBPalette16();
inline const CRGBPalette16 ForestColors_p = CRGBPalette16();
inline const CRGBPalette16 Rainbow_gp = CRGBPalette16();
#endif

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

static inline uint16_t random16(uint16_t lim) {
    if (lim == 0) return 0;
    return static_cast<uint16_t>(random16() % lim);
}

static inline uint8_t random8() {
    return static_cast<uint8_t>(random16() >> 8);
}

static inline uint8_t random8(uint8_t lim) {
    if (lim == 0) return 0;
    return static_cast<uint8_t>(random8() % lim);
}

#endif // POLAR_SHADER_NATIVE_FASTLED_H
