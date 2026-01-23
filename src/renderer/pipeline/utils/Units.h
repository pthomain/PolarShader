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

#ifndef POLAR_SHADER_PIPELINE_UTILS_UNITS_H
#define POLAR_SHADER_PIPELINE_UTILS_UNITS_H

#include <FixMath.h>
#include "FastLED.h"
#include "StrongTypes.h"

namespace PolarShader {
    // --- Constants ---

    // Represents the midpoint of a 16-bit unsigned integer range, often used for remapping.
    inline constexpr uint16_t U16_HALF = 0x8000;

    // The maximum positive value for a Q1.15 signed integer, used in FastLED's trig functions.
    inline constexpr int16_t TRIG_Q1_15_MAX = 32767;

    // The maximum value for a Q0.16 unsigned fraction, representing the value closest to 1.0.
    inline constexpr uint16_t FRACT_Q0_16_MAX = 0xFFFF;

    // Scalar 1.0 in Q16.16 fixed-point format.
    inline constexpr uint32_t Q16_16_ONE = 1u << 16;
    // A full revolution in the phase accumulator: AngleUnitsQ0_16 stored in the high 16 bits, wraps at 2^32.
    inline constexpr uint64_t PHASE_FULL_TURN = 1ULL << 32;

    // --- Angle Domain Constants (uint16_t) ---
    // The domain for 16-bit angle samples is [0..65535], where 65536 represents a full circle (the modulus).
    inline constexpr uint16_t QUARTER_TURN_U16 = 16384u;
    inline constexpr uint16_t HALF_TURN_U16 = 32768u;
    inline constexpr uint16_t ANGLE_U16_MAX = 65535u; // The maximum representable value for a uint16_t angle.

    // --- Strong Type Tags ---
    struct AngleUnitsQ0_16_Tag {
    };

    struct AngleTurnsUQ16_16_Tag {
    };

    struct FracQ0_16_Tag {
    };

    struct NoiseRawU16_Tag {
    };

    struct NoiseNormU16_Tag {
    };

    struct TrigQ1_15_Tag {
    };

    struct RawQ16_16_Tag {
    };

    // --- Type Aliases ---

    /**
    *   A phase angle expressed as a fraction of a full turn, quantized to 16 bits.
    *
    *   • Interpretation:
    *
    *       0 = 0 turn
    *       65535 ≈ 1 full turn (just below wrap)
    *       One unit = 1 / 65536 of a turn.
    *
    *   • Where it is typically used:
    *
    *       - As a sample angle when calling FastLED trig:
    *       sin16(angle) / cos16(angle) expect a uint16_t phase where 65536 corresponds to 2π.
    *
    *       - As a compact representation of angle for:
    *       indexing symmetry sectors (kaleidoscope/mandala),
    *
    *       - LUT indexing:
    *       “angle-only” operations that do not need sub-angle accumulation over time.
    *
    *   • When it should NOT be used:
    *
    *       As a continuously-integrated time accumulator (i.e., phase evolving with fractional dt).
    *       You lose sub-step precision and may quantize motion if you step by small increments.
    *
    *   • Key property:
    *
    *       It is modular: arithmetic wraps naturally at 16 bits.
    */
    using BoundedAngle = Strong<uint16_t, AngleUnitsQ0_16_Tag>;

    /**
    *   A higher-resolution phase accumulator in AngleUnitsQ0_16 using unsigned Q16.16 semantics:
    *
    *       Upper 16 bits: the coarse turn fraction (compatible with AngleTurns16)
    *       Lower 16 bits: fractional sub-steps (1 / 2^16 of an AngleTurns16 unit)
    *
    *   • Concretely:
    *
    *       AngleUnitsQ0_16 angle = phase >> 16;
    *       phase += (velocity * dt) (with velocity and dt scaled appropriately)
    *
    *   • Where it is typically used:
    *
    *       As the canonical phase type for:
    *
    *       - integrators / accumulators (phase evolving with dt),
    *       - anything needing smooth motion when dt is fractional or velocities are small,
    *       - transforms that add/compose rotations at sub-angle precision,
    *       - maintaining continuity across frames when frame timing jitters.
    *
    *   • When it should NOT be used:
    *
    *       - As input to FastLED trig directly without shifting:
    *         You must pass phase >> 16 (or extract upper 16 bits) to sin16/cos16.
    *
    *       - As a general signed scalar (it is unsigned, wrap semantics).
    *
    *   • Key property
    *
    *       It supports smooth accumulation and deterministic wrapping modulo 2^32
    *       (full wrap after 2^32 units, which corresponds to 65536 turns in Q16.16-turn space).
    *
    *   • Relationship to AngleUnitsQ0_16
    *
    *       - AngleUnitsQ0_16 is essentially the “integer part” of AngleTurnsUQ16_16: angle = phase >> 16
    *       - “Promoting” an angle into phase space is: phase = (uint32_t(angle) << 16)
    *
    *   This is precisely the choke-point that must be correct in the pipeline.
    */
    using UnboundedAngle = Strong<uint32_t, AngleTurnsUQ16_16_Tag>;

    inline constexpr UnboundedAngle ANGLE_TURNS_ONE_TURN = UnboundedAngle(Q16_16_ONE);

    /**
    *   The output of sin16/cos16 in signed fixed-point Q1.15:
    *
    *       range approximately [-32768, +32767]
    *       interpretation: value ≈ raw / 32768
    *
    *   • Examples:
    *
    *       sin16(0) ≈ 0
    *       sin16(16384) ≈ +32767 (1/4 turn, cartesian: {0, 1})
    *       sin16(49152) ≈ -32767 (3/4 turn, cartesian: {0, -1})
    *
    *   • Where it is used
    *
    *       Multiplying a trig result by an amplitude/scale in Q16.16 or Q0.16:
    *       Typical pattern: Q16.16 * Q1.15 -> Q17.31, then shift right by 15 to return to Q16.16-ish.
    *
    *   • When it should NOT be used
    *
    *       As a general “signal value” without documenting scaling.
    *       It is not Q16.16 and not Q0.16.
    *
    *   • Key differences
    *
    *       Signed bipolar waveform centered at 0.
    *       Different scaling from BoundedScalar and from Q16.16 signals.
    */
    using TrigQ1_15 = Strong<int16_t, TrigQ1_15_Tag>;

    /**
    *   An unsigned fraction in Q0.16, typically interpreted as [0, 1):
    *
    *       0x0000 = 0.0
    *       0xFFFF ≈ 0.9999847
    *
    *   • Where it is used:
    *
    *       Parameters that are conceptually “percentages” / “mix factors” / “attenuations”:
    *
    *       - retention/damping factors,
    *       - interpolation weights,
    *       - scaling factors that should never go negative.
    *
    *   • When it should NOT be used:
    *
    *       For bipolar signals (needs signed).
    *       For values that can exceed 1.0 (it saturates / wraps depending on code).
    *
    *   • Key differences:
    *
    *       Unsigned, non-negative, normalized.
    *       Often used with FastLED scale helpers and blends.
    */
    using BoundedScalar = Strong<uint16_t, FracQ0_16_Tag>;

    /**
    *   A signed fixed-point scalar with:
    *
    *       16 integer bits
    *       16 fractional bits
    *       Can represent roughly [-32768, +32767.999...].
    *
    *   • Where it is used:
    *
    *       - Any “control-rate” signal that must support:
    *       - negative values,
    *       - values larger than 1.0,
    *       - accumulation/integration (position, velocity, offsets),
    *       - parameter modulation where type clarity reduces mistakes.
    *
    *       This is the “typed” alternative to carrying raw int32_t Q16.16 around.
    *
    *   • Important: constructor vs raw:
    *
    *       SFix<16,16>(n) usually means “n as a numeric value”, i.e. n.0
    *
    *       SFix<16,16>::fromRaw(raw) means “raw bits are already Q16.16”
    *       This distinction matters a lot (scale factors, constants).
    *
    *   • When it should NOT be used:
    *
    *       In per-LED hot loops where you have intentionally hand-tuned integer math
    *       and want to avoid abstraction overhead.
    *       
    *       When you require modulo wrap semantics (use explicit wrap operations on raw, or unsigned types).
    */
    using UnboundedScalar = SFix<16, 16>;

    /**
    *   The underlying “raw bits” representation of a signed Q16.16 number.
    *
    *       Typical interpretation: numeric value = raw / 65536.
    *
    *   • Where it is used
    *
    *       Crossing boundaries where performance or interoperability matters:
    *
    *       - calling low-level math utilities (mul_q16_16_*, add_*, pow_q16),
    *       - saturating or wrapping operations that are defined on raw ints,
    *       - serialization or storage in tight memory.
    *
    *   • When it should NOT be used:
    *
    *       - As a public API type where unit ambiguity can creep in.
    *       - When you want the compiler to help you avoid mixing incompatible quantities.
    *
    *   • Key differences:
    *
    *       Same numeric meaning as UnboundedScalar, but without type safety.
    *       Wrap/overflow behavior depends entirely on the operations you choose.
    */
    using RawQ16_16 = Strong<int32_t, RawQ16_16_Tag>;

    /**
    *   The raw 16-bit output of FastLED’s inoise16.
    *
    *       - It is not guaranteed to span the full 0..65535 range for all inputs.
    *       - It’s best treated as “noise-domain units” rather than normalized units.
    *
    *   • Where it is used:
    *
    *       - Immediately after calling inoise16.
    *       - As an intermediate value that you either normalize,
    *         or remap/center around a midpoint to make it signed-ish.
    *
    *   • When it should NOT be used:
    *
    *       As if it were uniformly distributed or fully normalized.
    *       As an input to code that expects NoiseNormU16 semantics.
    */
    using NoiseRawU16 = Strong<uint16_t, NoiseRawU16_Tag>;

    /**
    *   A 16-bit value intended to represent noise mapped to the full 0..65535 domain (or at least a consistent normalized span).
    *
    *   • This makes it safe to use as:
    *
    *       - a “normalized noise” signal,
    *       - a consistent input to subsequent layers/mixers.
    *
    *   • Where it is used:
    *
    *       - As the standard output type of “noise layers” if your pipeline contract is “all layers return normalized noise.”
    *       - When converting to signed centered noise: signedNoise = int32_t(normNoise) - 32768
    *
    *   • When it should NOT be used:
    *
    *       - If your layer contract is “raw noise-domain value” and you already normalize later.
    *       - In code paths where normalizing twice can compress contrast.
    *
    *   • Key differences:
    *
    *       NoiseRawU16 is “whatever inoise16 returns.”
    *       NoiseNormU16 is “pipeline-normalized, consistent 0..65535-ish.”
     */
    using NoiseNormU16 = Strong<uint16_t, NoiseNormU16_Tag>;

    // Time
    using TimeMillis = unsigned long; // Arduino millis()

    // --- Raw extractors (avoid generic templates). ---
    constexpr uint16_t raw(BoundedAngle a) { return a.raw(); }
    constexpr uint32_t raw(UnboundedAngle p) { return p.raw(); }
    constexpr uint16_t raw(BoundedScalar f) { return f.raw(); }
    constexpr uint16_t raw(NoiseRawU16 n) { return n.raw(); }
    constexpr uint16_t raw(NoiseNormU16 n) { return n.raw(); }
    constexpr int16_t raw(TrigQ1_15 t) { return t.raw(); }
    constexpr int32_t raw(RawQ16_16 v) { return v.raw(); }

    // --- Angle/phase promotion and sampling ---
    constexpr UnboundedAngle unbindAngle(BoundedAngle units) {
        return UnboundedAngle(static_cast<uint32_t>(raw(units)) << 16);
    }

    constexpr BoundedAngle bindAngle(UnboundedAngle turns) {
        return BoundedAngle(static_cast<uint16_t>(raw(turns) >> 16));
    }

    // FastLED trig sampling helpers (explicit angle/phase conversions).
    constexpr uint16_t toFastLedPhase(BoundedAngle a) { return raw(a); }

    constexpr uint16_t toFastLedPhase(UnboundedAngle p) {
        return static_cast<uint16_t>(raw(p) >> 16);
    }

    inline TrigQ1_15 sinQ1_15(BoundedAngle a) {
        return TrigQ1_15(sin16(toFastLedPhase(a)));
    }

    inline TrigQ1_15 sinQ1_15(UnboundedAngle p) {
        return TrigQ1_15(sin16(toFastLedPhase(p)));
    }

    inline TrigQ1_15 cosQ1_15(BoundedAngle a) {
        return TrigQ1_15(cos16(toFastLedPhase(a)));
    }

    inline TrigQ1_15 cosQ1_15(UnboundedAngle p) {
        return TrigQ1_15(cos16(toFastLedPhase(p)));
    }

    // --- Phase wrap arithmetic (mod 2^32) ---
    constexpr UnboundedAngle wrapAdd(UnboundedAngle a, uint32_t delta) {
        return UnboundedAngle(raw(a) + delta);
    }

    // Wrap-add using signed raw delta (Q16.16), interpreted via two's-complement.
    constexpr UnboundedAngle wrapAddSigned(UnboundedAngle a, int32_t delta_raw_q16_16) {
        return UnboundedAngle(raw(a) + static_cast<uint32_t>(delta_raw_q16_16));
    }

    // --- Phase quantize/fold helpers ---
    inline UnboundedAngle mulPhaseWrap(UnboundedAngle p, uint8_t k) {
        return UnboundedAngle(raw(p) * static_cast<uint32_t>(k));
    }

    inline UnboundedAngle quantizePhase(UnboundedAngle p, uint32_t binWidth) {
        if (binWidth == 0) return p;
        uint32_t v = raw(p);
        uint32_t q = static_cast<uint32_t>((static_cast<uint64_t>(v) / binWidth) * binWidth);
        return UnboundedAngle(q);
    }
}

#endif //POLAR_SHADER_PIPELINE_UTILS_UNITS_H
