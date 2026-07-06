//  SPDX-License-Identifier: GPL-3.0-only

/*
 * PatternFlow adaptation NOTICE
 * -----------------------------
 * Field algorithm adapted from PatternFlow (engmung/Patternflow, author
 * Seunghun LEE, CC-BY-SA-4.0): effects 0512 (Petals), 0524-2 (Ripple).
 * Algorithm only, re-derived into fixed point; no source lines copied.
 * GPL-3.0-only via the CC-BY-SA-4.0 -> GPLv3 path. See patternflow/ATTRIBUTION.md.
 */

#include "renderer/pipeline/patterns/patternflow/PfRadialField.h"
#include "renderer/pipeline/patterns/patternflow/PfFieldMaths.h"
#include "renderer/pipeline/maths/AngleMaths.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include "renderer/pipeline/maths/PolarMaths.h"

namespace PolarShader {
    struct PfRadialField::State {
        Variant variant;
        uint8_t petalCount;
        Sf16Signal phaseSpeedSignal;
        Sf16Signal foldSignal;
        Sf16Signal thicknessSignal;

        int32_t tTurns{0};
        f16 foldF16{f16(0)};
        int32_t halfBandRaw{0}; // petal band half-width in Q16 radius units

        TimeMillis lastElapsedMs{0u};
        bool hasLastElapsed{false};

        State(Variant v, uint8_t pc, Sf16Signal ps, Sf16Signal fold, Sf16Signal th)
            : variant(v),
              petalCount(pc < 1 ? uint8_t(1) : pc),
              phaseSpeedSignal(std::move(ps)),
              foldSignal(std::move(fold)),
              thicknessSignal(std::move(th)) {}
    };

    struct PfRadialField::Functor {
        const State *state;

        PaletteSample petals(UV uv) const {
            UV polar = cartesianToPolarUV(uv);
            int32_t angleTurns = raw(polar.u);   // f16 turns (0..65535)
            int32_t radiusQ16 = raw(polar.v);    // [0, 65536]
            const int32_t t = state->tTurns;

            // Angular petal wave and a radial ripple layered on top.
            sf16 petalWave = PfMath::pfSinTurns(state->petalCount * angleTurns + PfMath::pfCoefT(t, 2, 1));
            sf16 ripple = scaleSf16(
                PfMath::pfSinTurns(2 * radiusQ16 - PfMath::pfCoefT(t, 3, 1)),
                state->foldF16
            );

            // targetRadius = 0.45 + 0.30*petalWave + 0.15*ripple  (Q16 radius)
            int32_t target = (SF16_ONE * 45 / 100)
                             + raw(scaleSf16(petalWave, perMil(300)))
                             + raw(scaleSf16(ripple, perMil(150)));

            int32_t diff = radiusQ16 - target;
            uint16_t band = PfMath::pfBump(diff, state->halfBandRaw);
            // Hue: the polar angle sweeps colour around the flower.
            PatternNormU16 hue = PatternNormU16(static_cast<uint16_t>(angleTurns));
            return PaletteSample{hue, PatternNormU16(band)};
        }

        PaletteSample rippleField(UV uv) const {
            UV polar = cartesianToPolarUV(uv);
            int32_t angleTurns = raw(polar.u);
            int32_t radiusQ16 = raw(polar.v);
            const int32_t t = state->tTurns;

            // Angular warp bends the radial wavefronts (refraction stand-in),
            // scaled by fold. No true refraction sampling.
            int32_t warpPhase = raw(scaleSf16(
                PfMath::pfSinTurns(2 * angleTurns + PfMath::pfCoefT(t, 3, 2)),
                state->foldF16
            ));
            // petalCount travelling concentric ripples pushed inward over time.
            int32_t wave = raw(PfMath::pfSinTurns(
                state->petalCount * 2 * radiusQ16 + warpPhase - t
            ));
            // Bright thin crest where the wave peaks (+1), plus a faint base so
            // troughs are not fully black.
            uint16_t crest = PfMath::pfBump(SF16_ONE - wave, state->halfBandRaw);
            uint32_t base = static_cast<uint32_t>(raw(PfMath::pfSignedToNorm(wave, SF16_ONE))) >> 2;
            uint32_t out = static_cast<uint32_t>(crest) + base;
            if (out > F16_MAX) out = F16_MAX;
            // Hue: the signed ripple wave in [-1, 1] tints crest vs trough.
            PatternNormU16 hue = PfMath::pfSignedToNorm(wave, SF16_ONE);
            return PaletteSample{hue, PatternNormU16(static_cast<uint16_t>(out))};
        }

        PaletteSample sample(UV uv) const {
            switch (state->variant) {
                case Variant::Petals: return petals(uv);
                case Variant::Ripple: return rippleField(uv);
            }
            return PaletteSample{PatternNormU16(0), PatternNormU16(0)};
        }

        PatternNormU16 operator()(UV uv) const {
            return sample(uv).value();
        }
    };

    PfRadialField::PfRadialField(
        Variant variant,
        uint8_t petalCount,
        Sf16Signal phaseSpeed,
        Sf16Signal fold,
        Sf16Signal thickness
    ) : state(std::make_shared<State>(
        variant,
        petalCount,
        std::move(phaseSpeed),
        std::move(fold),
        std::move(thickness)
    )) {}

    void PfRadialField::advanceFrame(f16 progress, TimeMillis elapsedMs) {
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
        int32_t foldRaw = raw(s.foldSignal.sample(magnitudeRange(), elapsedMs));
        int32_t thickRaw = raw(s.thicknessSignal.sample(magnitudeRange(), elapsedMs));

        int32_t deltaTurns = static_cast<int32_t>(
            (static_cast<int64_t>(phaseRaw) * static_cast<int64_t>(dtMs) * 2) / 5000
        );
        s.tTurns += deltaTurns;

        int32_t foldClamped = foldRaw < 0 ? 0 : (foldRaw > static_cast<int32_t>(F16_MAX) ? F16_MAX : foldRaw);
        s.foldF16 = f16(static_cast<uint16_t>(foldClamped));

        // Band half-width: 0.06 .. 0.26 of radius as thickness goes 0 -> 1.
        s.halfBandRaw = (SF16_ONE * 6 / 100) + static_cast<int32_t>(
            (static_cast<int64_t>(thickRaw) * (SF16_ONE * 20 / 100)) >> 16
        );
    }

    UVMap PfRadialField::layer(const std::shared_ptr<PipelineContext> &context) const {
        (void)context;
        return Functor{state.get()};
    }

    bool PfRadialField::emitsColour() const {
        return true;
    }

    UVColourMap PfRadialField::colourLayer(const std::shared_ptr<PipelineContext> &context) const {
        (void)context;
        return [f = Functor{state.get()}](UV uv) { return f.sample(uv); };
    }
}
