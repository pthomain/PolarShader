//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

/*
 * This file is part of PolarShader. Original PolarShader code (no PatternFlow
 * adaptation). Distributed under GPL-3.0-or-later. See
 * <https://www.gnu.org/licenses/>.
 */

#include "renderer/pipeline/patterns/patternflow/PfSourceField.h"
#include "renderer/pipeline/patterns/patternflow/PfFieldMaths.h"
#include "renderer/pipeline/maths/AngleMaths.h"
#include "renderer/pipeline/maths/ScalarMaths.h"

namespace PolarShader {
    namespace {
        constexpr uint8_t PF_SOURCE_MAX = 8;
    }

    struct PfSourceField::State {
        uint8_t sourceCount;
        Sf16Signal phaseSpeedSignal;
        Sf16Signal warpSignal;
        Sf16Signal thicknessSignal;

        int32_t tTurns{0};
        int32_t kCycles{3};      // spatial frequency (cycles per Q16 unit dist)
        int32_t halfBandRaw{0};  // crest band half-width in Q16 units

        // Emitter positions in centred Q16 cartesian (raw), refreshed per frame.
        int32_t srcX[PF_SOURCE_MAX]{};
        int32_t srcY[PF_SOURCE_MAX]{};

        TimeMillis lastElapsedMs{0u};
        bool hasLastElapsed{false};

        State(uint8_t sc, Sf16Signal ps, Sf16Signal wp, Sf16Signal th)
            : sourceCount(sc < 1 ? uint8_t(1) : (sc > PF_SOURCE_MAX ? PF_SOURCE_MAX : sc)),
              phaseSpeedSignal(std::move(ps)),
              warpSignal(std::move(wp)),
              thicknessSignal(std::move(th)) {}
    };

    struct PfSourceField::Functor {
        const State *state;

        PaletteSample sample(UV uv) const {
            const int32_t X = (raw(uv.u) << 1) - SF16_ONE;
            const int32_t Y = (raw(uv.v) << 1) - SF16_ONE;
            const int32_t t = state->tTurns;
            const int32_t k = state->kCycles;
            const uint8_t n = state->sourceCount;

            int32_t acc = 0;
            for (uint8_t i = 0; i < n; ++i) {
                int32_t dx = X - state->srcX[i];
                int32_t dy = Y - state->srcY[i];
                uint32_t dist = PfMath::pfDistQ16(dx, dy);
                int32_t phase = static_cast<int32_t>(k * dist) - t;
                acc += raw(PfMath::pfSinTurns(phase));
            }
            // Mean travelling-wave amplitude in sf16 raw units, [-1, 1].
            int32_t wv = acc / n;

            // Thin bright crest where the summed wave peaks (+1), plus a faint
            // base so troughs are not fully black.
            uint16_t crest = PfMath::pfBump(SF16_ONE - wv, state->halfBandRaw);
            uint32_t base = static_cast<uint32_t>(raw(PfMath::pfSignedToNorm(wv, SF16_ONE))) >> 2;
            uint32_t out = static_cast<uint32_t>(crest) + base;
            if (out > F16_MAX) out = F16_MAX;

            // Hue: the signed interference amplitude tints crest vs trough.
            PatternNormU16 hue = PfMath::pfSignedToNorm(wv, SF16_ONE);
            return PaletteSample{hue, PatternNormU16(static_cast<uint16_t>(out))};
        }

        PatternNormU16 operator()(UV uv) const {
            return sample(uv).value();
        }
    };

    PfSourceField::PfSourceField(
        uint8_t sourceCount,
        Sf16Signal phaseSpeed,
        Sf16Signal warp,
        Sf16Signal thickness
    ) : state(std::make_shared<State>(
        sourceCount,
        std::move(phaseSpeed),
        std::move(warp),
        std::move(thickness)
    )) {}

    void PfSourceField::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void)progress;
        State &s = *state;

        TimeMillis dtMs = 0u;
        if (!s.hasLastElapsed) {
            s.lastElapsedMs = elapsedMs;
            s.hasLastElapsed = true;
        } else {
            dtMs = elapsedMs >= s.lastElapsedMs ? (elapsedMs - s.lastElapsedMs) : 0u;
            if (dtMs > MAX_DELTA_TIME_MS) dtMs = MAX_DELTA_TIME_MS;
            s.lastElapsedMs = elapsedMs;
        }

        int32_t phaseRaw = raw(s.phaseSpeedSignal.sample(magnitudeRange(), elapsedMs));
        int32_t warpRaw = raw(s.warpSignal.sample(magnitudeRange(), elapsedMs));
        int32_t thickRaw = raw(s.thicknessSignal.sample(magnitudeRange(), elapsedMs));

        int32_t deltaTurns = static_cast<int32_t>(
            (static_cast<int64_t>(phaseRaw) * static_cast<int64_t>(dtMs) * 2) / 5000
        );
        s.tTurns += deltaTurns;

        // Spatial frequency: 3 .. 12 cycles across the field as warp goes 0 -> 1.
        s.kCycles = 3 + static_cast<int32_t>(
            (static_cast<int64_t>(warpRaw) * 9) >> 16
        );

        // Crest band half-width: 0.05 .. 0.25 of a unit as thickness goes 0 -> 1.
        s.halfBandRaw = (SF16_ONE * 5 / 100) + static_cast<int32_t>(
            (static_cast<int64_t>(thickRaw) * (SF16_ONE * 20 / 100)) >> 16
        );

        // Emitters drift on a rotating ring of radius 0.5. The ring rotation is
        // tied to the phase accumulator so drift speed follows phaseSpeed.
        int32_t ringSpin = PfMath::pfCoefT(s.tTurns, 1, 8);
        for (uint8_t i = 0; i < s.sourceCount; ++i) {
            int32_t baseTurns = static_cast<int32_t>(
                (static_cast<int64_t>(i) * F16_MAX) / s.sourceCount
            );
            f16 a = f16(static_cast<uint16_t>(baseTurns + ringSpin));
            s.srcX[i] = raw(scaleSf16(angleCosF16(a), perMil(500)));
            s.srcY[i] = raw(scaleSf16(angleSinF16(a), perMil(500)));
        }
    }

    UVMap PfSourceField::layer(const std::shared_ptr<PipelineContext> &context) const {
        (void)context;
        return Functor{state.get()};
    }

    bool PfSourceField::emitsColour() const {
        return true;
    }

    UVColourMap PfSourceField::colourLayer(const std::shared_ptr<PipelineContext> &context) const {
        (void)context;
        return [f = Functor{state.get()}](UV uv) { return f.sample(uv); };
    }
}
