//  SPDX-License-Identifier: GPL-3.0-only

/*
 * PatternFlow adaptation NOTICE
 * -----------------------------
 * Field algorithms adapted from PatternFlow (engmung/Patternflow, author
 * Seunghun LEE, CC-BY-SA-4.0): "A Big Hit" (Plasma), 0520 (Tendrils),
 * 0530 (LiquidGate).
 * Algorithm only, re-derived into fixed point; no source lines copied.
 * GPL-3.0-only via the CC-BY-SA-4.0 -> GPLv3 path. See patternflow/ATTRIBUTION.md.
 */

#include "renderer/pipeline/patterns/patternflow/PfPlasmaWarp.h"
#include "renderer/pipeline/patterns/patternflow/PfFieldMaths.h"
#include "renderer/pipeline/maths/AngleMaths.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include "renderer/pipeline/maths/PatternMaths.h"

namespace PolarShader {
    struct PfPlasmaWarp::State {
        Variant variant;
        Sf16Signal phaseSpeedSignal;
        Sf16Signal warpSignal;
        Sf16Signal thicknessSignal;

        int32_t tTurns{0};
        f16 warpF16{f16(0)};
        int32_t halfWidthRaw{0}; // Tendrils filament half-width (sf16 units)
        int32_t gateHalfRaw{0};  // LiquidGate contrast-window half-width (Q16)

        TimeMillis lastElapsedMs{0u};
        bool hasLastElapsed{false};

        State(Variant v, Sf16Signal ps, Sf16Signal w, Sf16Signal th)
            : variant(v),
              phaseSpeedSignal(std::move(ps)),
              warpSignal(std::move(w)),
              thicknessSignal(std::move(th)) {}
    };

    struct PfPlasmaWarp::Functor {
        const State *state;

        PaletteSample plasma(int32_t X, int32_t Y) const {
            const int32_t t = state->tTurns;
            int32_t v1 = raw(PfMath::pfSinTurns(3 * X + t));
            int32_t v2 = raw(PfMath::pfCosTurns(3 * Y - PfMath::pfCoefT(t, 4, 5)));
            // Chaos-scaled warp offsets fed back into higher-frequency waves.
            int32_t wx = raw(scaleSf16(PfMath::pfSinTurns(6 * Y + t), state->warpF16));
            int32_t wy = raw(scaleSf16(PfMath::pfCosTurns(6 * X - PfMath::pfCoefT(t, 6, 5)), state->warpF16));
            int32_t v3 = raw(PfMath::pfSinTurns(4 * X + wx + PfMath::pfCoefT(t, 3, 2)));
            int32_t v4 = raw(PfMath::pfCosTurns(4 * Y + wy - t));
            int32_t sum = v1 + v2 + v3 + v4;
            int32_t a = sum < 0 ? -sum : sum;      // [0, 4*SF16_ONE]
            int32_t inner = SF16_ONE - (a >> 1);        // 1 - |sum|/2
            if (inner < 0) inner = 0;
            int64_t c2 = (static_cast<int64_t>(inner) * inner) >> 16;
            int64_t c3 = (c2 * inner) >> 16;
            c3 = (c3 * 5) / 2;                       // *2.5 gain
            if (c3 > F16_MAX) c3 = F16_MAX;
            // Hue: the two base waves give a cheap phase proxy in [-2, 2].
            PatternNormU16 hue = PfMath::pfSignedToNorm(v1 + v2, 2 * SF16_ONE);
            return PaletteSample{hue, PatternNormU16(static_cast<uint16_t>(c3))};
        }

        PaletteSample tendrils(int32_t X, int32_t Y) const {
            const int32_t t = state->tTurns;
            sf16 f1 = mulSf16Sat(PfMath::pfSinTurns(3 * X + t),
                                 PfMath::pfCosTurns(3 * Y + PfMath::pfCoefT(t, 1, 2)));
            sf16 f2 = mulSf16Sat(PfMath::pfSinTurns(3 * Y - PfMath::pfCoefT(t, 7, 10)),
                                 PfMath::pfCosTurns(3 * X - PfMath::pfCoefT(t, 3, 10)));
            int32_t wx = raw(scaleSf16(f1, state->warpF16));
            int32_t wy = raw(scaleSf16(f2, state->warpF16));
            int32_t cf = raw(PfMath::pfSinTurns(4 * X + wx - PfMath::pfCoefT(t, 4, 5)))
                         + raw(PfMath::pfCosTurns(4 * Y + wy + PfMath::pfCoefT(t, 11, 10)));
            int32_t a = cf < 0 ? -cf : cf;           // [0, 2*SF16_ONE]
            // Filament ridge where the centre-field crosses ~0.4.
            int32_t d = a - (SF16_ONE * 2 / 5);
            uint16_t val = PfMath::pfBump(d, state->halfWidthRaw);
            // Hue: the signed centre-field in [-2, 2] varies along filaments.
            PatternNormU16 hue = PfMath::pfSignedToNorm(cf, 2 * SF16_ONE);
            return PaletteSample{hue, PatternNormU16(val)};
        }

        PaletteSample liquidGate(int32_t X, int32_t Y) const {
            const int32_t t = state->tTurns;
            // Self-warp offsets fed from lower-frequency waves.
            int32_t wx = raw(scaleSf16(PfMath::pfSinTurns(2 * Y + PfMath::pfCoefT(t, 3, 5)), state->warpF16));
            int32_t wy = raw(scaleSf16(PfMath::pfCosTurns(2 * X - PfMath::pfCoefT(t, 4, 5)), state->warpF16));
            // Warped sine*cos product -> the liquid metaball field.
            sf16 a = PfMath::pfSinTurns(3 * X + wx + t);
            sf16 b = PfMath::pfCosTurns(3 * Y + wy - PfMath::pfCoefT(t, 7, 10));
            int32_t prod = raw(mulSf16Sat(a, b)); // [-1, 1]
            sf16 c = PfMath::pfSinTurns(2 * (X + Y) + wx - t);
            int32_t field = prod + raw(scaleSf16(c, perMil(500))); // ~[-1.5, 1.5]
            int32_t norm = raw(PfMath::pfSignedToNorm(field, 3 * SF16_ONE / 2)); // [0, 65535]
            // Drifting contrast gate; thickness widens the transition band.
            int32_t mid = (F16_MAX >> 1) + (raw(PfMath::pfSinTurns(PfMath::pfCoefT(t, 1, 2))) / 6);
            int32_t hw = state->gateHalfRaw;
            int32_t e0 = mid - hw; if (e0 < 0) e0 = 0;
            int32_t e1 = mid + hw; if (e1 > static_cast<int32_t>(F16_MAX)) e1 = F16_MAX;
            PatternNormU16 value = patternSmoothstepU16(static_cast<uint16_t>(e0),
                                                        static_cast<uint16_t>(e1),
                                                        static_cast<uint16_t>(norm));
            // Hue: the pre-gate metaball field is already a [0, 65535] proxy.
            return PaletteSample{PatternNormU16(static_cast<uint16_t>(norm)), value};
        }

        PaletteSample sample(UV uv) const {
            int32_t X = raw(uv.u);
            int32_t Y = raw(uv.v);
            switch (state->variant) {
                case Variant::Plasma:     return plasma(X, Y);
                case Variant::Tendrils:   return tendrils(X, Y);
                case Variant::LiquidGate: return liquidGate(X, Y);
            }
            return PaletteSample{PatternNormU16(0), PatternNormU16(0)};
        }

        PatternNormU16 operator()(UV uv) const {
            return sample(uv).value();
        }
    };

    PfPlasmaWarp::PfPlasmaWarp(
        Variant variant,
        Sf16Signal phaseSpeed,
        Sf16Signal warp,
        Sf16Signal thickness
    ) : state(std::make_shared<State>(
        variant,
        std::move(phaseSpeed),
        std::move(warp),
        std::move(thickness)
    )) {}

    void PfPlasmaWarp::advanceFrame(f16 progress, TimeMillis elapsedMs) {
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

        int32_t warpClamped = warpRaw < 0 ? 0 : (warpRaw > static_cast<int32_t>(F16_MAX) ? F16_MAX : warpRaw);
        s.warpF16 = f16(static_cast<uint16_t>(warpClamped));

        // Filament half-width: 0.08 .. 0.40 of a unit as thickness goes 0 -> 1.
        s.halfWidthRaw = (SF16_ONE * 8 / 100) + static_cast<int32_t>(
            (static_cast<int64_t>(thickRaw) * (SF16_ONE * 32 / 100)) >> 16
        );

        // LiquidGate contrast window: 0.05 .. 0.45 of full range (thin gate ->
        // hard blobs; wide gate -> soft) as thickness goes 0 -> 1.
        int32_t thickClamped = thickRaw < 0 ? 0 : (thickRaw > static_cast<int32_t>(F16_MAX) ? F16_MAX : thickRaw);
        s.gateHalfRaw = (F16_MAX * 5 / 100) + static_cast<int32_t>(
            (static_cast<int64_t>(thickClamped) * (F16_MAX * 40 / 100)) >> 16
        );
    }

    UVMap PfPlasmaWarp::layer(const std::shared_ptr<PipelineContext> &context) const {
        (void)context;
        return Functor{state.get()};
    }

    bool PfPlasmaWarp::emitsColour() const {
        return true;
    }

    UVColourMap PfPlasmaWarp::colourLayer(const std::shared_ptr<PipelineContext> &context) const {
        (void)context;
        return [f = Functor{state.get()}](UV uv) { return f.sample(uv); };
    }
}
