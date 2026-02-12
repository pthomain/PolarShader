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

#ifndef POLAR_SHADER_PIPELINE_UNITS_UNITS_H
#define POLAR_SHADER_PIPELINE_UNITS_UNITS_H

#ifdef ARDUINO
#include <stdint.h>
#else
#include <cstdint>
#endif

/**
 * @brief Time alias for standard Arduino millis().
 * 
 * Analysis: Simple type alias to ensure consistent time units across the signal engine.
 * Defined in global namespace to match Arduino environment.
 */
using TimeMillis = unsigned long;
using Period = TimeMillis;

inline constexpr TimeMillis MAX_DELTA_TIME_MS = 200; // 0 disables delta-time clamping.

namespace PolarShader {
    // Pull standard types into our namespace so we can use them without prefixes
    // while avoiding global namespace pollution.
#ifdef ARDUINO
    using ::int8_t;
    using ::uint8_t;
    using ::int16_t;
    using ::uint16_t;
    using ::int32_t;
    using ::uint32_t;
    using ::int64_t;
    using ::uint64_t;
#else
    using std::int8_t;
    using std::uint8_t;
    using std::int16_t;
    using std::uint16_t;
    using std::int32_t;
    using std::uint32_t;
    using std::int64_t;
    using std::uint64_t;
#endif

    // --- Strong Type Wrapper ---

    template<typename Rep, typename Tag>
    class Typed {
    public:
        using rep_type = Rep;

        constexpr Typed() : v_(0) {
        }

        explicit constexpr Typed(Rep raw) : v_(raw) {
        }

        constexpr Rep raw() const { return v_; }

        friend constexpr bool operator==(Typed a, Typed b) { return a.v_ == b.v_; }
        friend constexpr bool operator!=(Typed a, Typed b) { return a.v_ != b.v_; }
        friend constexpr bool operator<(Typed a, Typed b) { return a.v_ < b.v_; }
        friend constexpr bool operator>(Typed a, Typed b) { return a.v_ > b.v_; }
        friend constexpr bool operator<=(Typed a, Typed b) { return a.v_ <= b.v_; }
        friend constexpr bool operator>=(Typed a, Typed b) { return a.v_ >= b.v_; }

    private:
        Rep v_;
    };

    constexpr int32_t raw(int32_t v) { return v; }
    constexpr uint32_t raw(uint32_t v) { return v; }
    constexpr uint16_t raw(uint16_t v) { return v; }
    constexpr uint8_t raw(uint8_t v) { return v; }

    // --- Constants ---

    // Represents the midpoint of a 16-bit unsigned integer range, often used for remapping.
    inline constexpr uint16_t U16_HALF = 0x8000;

    // The maximum positive value for an sf16 signed integer stored in a 32-bit raw format.
    inline constexpr int32_t SF16_MIN = -(1 << 16);
    inline constexpr int32_t SF16_MAX = (1 << 16) - 1;
    inline constexpr int32_t SF16_ONE = 1 << 16;

    // The maximum value for an f16 unsigned fraction, representing the value closest to 1.0.
    inline constexpr uint16_t F16_MAX = 0xFFFF;

    // Full turn in the 16-bit angle domain.
    inline constexpr uint32_t ANGLE_FULL_TURN_U32 = 1u << 16;

    // Cartesian coordinate fixed-point fractional bits (sr8/r8).
    // These coordinates represent f16 lattice units with extra fractional precision.
    inline constexpr uint8_t R8_FRAC_BITS = 8;

    // --- Angle domain constants (uint16_t) ---
    // The domain for 16-bit angle samples is [0..65535], where 65536 represents a full circle (the modulus).
    inline constexpr uint16_t QUARTER_TURN_U16 = 16384u;
    inline constexpr uint16_t HALF_TURN_U16 = 32768u;
    inline constexpr uint16_t ANGLE_U16_MAX = 65535u; // The maximum representable value for a uint16_t angle.

    // --- Scalar Units ---

    struct f16Tag {
    };

    struct sf16Tag {
    };

    struct r16Tag {
    };

    struct sr16Tag {
    };

    /**
     * @brief Unsigned fixed-point fraction in f16 (Q0.16) format.
     * 
     * Definition: 16-bit integer where 0 represents 0.0 and 65535 represents ~1.0.
     * Usage: Used for angles (mod 2^16), alpha blending, and unsigned scaling factors.
     * Analysis: Strictly required for circular math and operations where negative values are semantically invalid.
     */
    using f16 = Typed<uint16_t, f16Tag>;

    /**
     * @brief Signed fixed-point scalar in sf16 (Q0.16) format stored in a 32-bit container.
     * 
     * Definition: Signed integer where 65536 represents 1.0 and -65536 represents -1.0.
     * Usage: The primary currency for signals, oscillators, and trigonometric outputs (sine/cosine).
     * Analysis: Strictly required for the signal engine to support bidirectional modulation (e.g. oscillating around zero).
     */
    using sf16 = Typed<int32_t, sf16Tag>;

    /**
     * @brief Unsigned fixed-point scalar in r16 (Q16.16) format.
     *
     * Definition: 16 integer bits and 16 fractional bits. 65536 represents 1.0.
     * Usage: Higher-precision ratio/range values where transform composition needs sub-pixel stability.
     */
    using r16 = Typed<uint32_t, r16Tag>;

    /**
     * @brief Signed fixed-point scalar in sr16 (Q16.16) format.
     * 
     * Definition: 16 integer bits and 16 fractional bits. 65536 represents 1.0.
     * Usage: Used exclusively for UV spatial coordinates.
     * Analysis: Strictly required to provide enough dynamic range for tiling/zoom (values > 1.0) while maintaining sub-pixel precision.
     */
    using sr16 = Typed<int32_t, sr16Tag>;

    // --- Raw extractors ---
    constexpr uint16_t raw(f16 f) { return f.raw(); }
    constexpr int32_t raw(sf16 v) { return v.raw(); }
    constexpr uint32_t raw(r16 v) { return v.raw(); }
    constexpr int32_t raw(sr16 v) { return v.raw(); }

    // --- Cartesian Units ---

    /**
     * @brief A simple 2D signed integer vector.
     * 
     * Usage: Lightweight tuple for raw coordinate pairs or intermediate displacements.
     * Analysis: Useful internal utility, distinct from the unified UV coordinate system.
     */
    struct v32 {
        int32_t x;
        int32_t y;
    };

    struct sr8Tag {
    };

    struct r8Tag {
    };

    /**
     * @brief Signed fixed-point coordinate in sr8/r8 format.
     * 
     * Definition: 24 integer bits, 8 fractional bits. 256 represents 1.0.
     * Usage: Implementation detail for lattice-aligned patterns (Worley, HexTiling).
     * Analysis: Chosen over sr16 when grid/index logic dominates and 8 fractional bits are sufficient.
     */
    using sr8 = Typed<int32_t, sr8Tag>;

    /**
     * @brief Unsigned fixed-point coordinate in sr8/r8 format.
     * 
     * Usage: Exclusively for noise domain sampling (inoise16).
     * Analysis: Required to match the unsigned interface of hardware-optimized noise generators and keep noise-domain math lightweight.
     */
    using r8 = Typed<uint32_t, r8Tag>;

    constexpr int32_t raw(sr8 v) { return v.raw(); }
    constexpr uint32_t raw(r8 v) { return v.raw(); }

    // --- Pattern Units ---

    struct NoiseRawU16_Tag {
    };

    struct PatternNormU16_Tag {
    };

    /**
     * @brief Raw 16-bit output from a noise generator.
     * 
     * Usage: Transient type in NoiseMaths before normalization.
     * Analysis: Required to distinguish "raw" unmapped data from "clean" pipeline data.
     */
    using NoiseRawU16 = Typed<uint16_t, NoiseRawU16_Tag>;

    /**
     * @brief Strictly normalized 16-bit pattern intensity.
     * 
     * Definition: Unsigned 16-bit integer spanning the full 0..65535 range.
     * Usage: The standard output of every UVPattern and the standard input for palette mapping.
     * Analysis: Strictly required as the "universal currency" of the visual pipeline.
     */
    using PatternNormU16 = Typed<uint16_t, PatternNormU16_Tag>;

    // --- Raw extractors ---
    constexpr uint16_t raw(NoiseRawU16 n) { return n.raw(); }
    constexpr uint16_t raw(PatternNormU16 n) { return n.raw(); }

    // --- UV Units ---

    /**
     * @brief Represents a normalized spatial coordinate in UV space.
     * 
     * Definition: A 2D vector where U and V are normalized values in the range [0.0, 1.0], 
     * mapped to the signed sr16 (Q16.16) fixed-point type.
     * 
     * Usage: The unified standard for all spatial transformations and pattern sampling.
     * 
     * Analysis: Strictly required to enable seamless composability between Cartesian 
     * and Polar transforms without explicit domain switching logic.
     */
    struct UV {
        sr16 u;
        sr16 v;

        constexpr UV() : u(0), v(0) {
        }

        constexpr UV(sr16 u, sr16 v) : u(u), v(v) {
        }
    };
}

#endif // POLAR_SHADER_PIPELINE_UNITS_UNITS_H
