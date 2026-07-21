//  SPDX-License-Identifier: GPL-3.0-only

/*
 * PatternFlow adaptation NOTICE
 * -----------------------------
 * Field algorithms adapted from PatternFlow (engmung/Patternflow, author
 * Seunghun LEE, CC-BY-SA-4.0): effects 0510, 0515, 0515-3, 0531, 0614-2.
 * Algorithm only, re-derived into fixed point; no source lines copied.
 * Distributed under GPL-3.0-only via the CC-BY-SA-4.0 -> GPLv3 path. See
 * patternflow/ATTRIBUTION.md.
 */

#include "renderer/pipeline/patterns/patternflow/PfInterferenceField.h"
#include "renderer/pipeline/patterns/patternflow/PfFieldMaths.h"
#include "renderer/pipeline/maths/AngleMaths.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include <algorithm>

namespace PolarShader {
    struct PfInterferenceField::State {
        Variant variant;
        uint8_t levels;
        S0x16Signal phaseSpeedSignal;
        S0x16Signal warpSignal;
        S0x16Signal thicknessSignal;

        // Cached per frame.
        int32_t tTurns{0};       // Q16 turns internal time
        u0x16 warpU0x16{u0x16(0)};     // warp as [0,1] scale factor
        int32_t thicknessRaw{0}; // s0x16 [0,1]

        TimeMillis lastElapsedMs{0u};
        bool hasLastElapsed{false};

        State(Variant v, uint8_t lv, S0x16Signal ps, S0x16Signal w, S0x16Signal th)
            : variant(v),
              levels(lv < 2 ? uint8_t(2) : lv),
              phaseSpeedSignal(std::move(ps)),
              warpSignal(std::move(w)),
              thicknessSignal(std::move(th)) {}
    };

    struct PfInterferenceField::Functor {
        const State *state;

        // --- individual variant fields ---

        PaletteSample dualAxis(int32_t X, int32_t Y) const {
            const int32_t t = state->tTurns;
            // Domain-warp waves (scaled by warp).
            s0x16 dy = scaleS0x16(PfMath::pfSinTurns(2 * Y + PfMath::pfCoefT(t, 4, 5)), state->warpU0x16);
            s0x16 dx = scaleS0x16(PfMath::pfCosTurns(2 * X - PfMath::pfCoefT(t, 9, 10)), state->warpU0x16);
            int32_t s1 = raw(PfMath::pfSinTurns(3 * X + raw(dy) + t));
            int32_t s2 = raw(PfMath::pfCosTurns(3 * Y - PfMath::pfCoefT(t, 6, 5) + raw(dx)));
            PatternNormU0x16 value = PfMath::pfSignedToNorm(s1 + s2, 2 * S0X16_ONE);
            // Hue: the orthogonal wave difference gives an independent phase.
            PatternNormU0x16 hue = PfMath::pfSignedToNorm(s1 - s2, 2 * S0X16_ONE);
            return PaletteSample{hue, value};
        }

        PaletteSample counterRibbons(int32_t X, int32_t Y) const {
            const int32_t t = state->tTurns;
            // Two counter-travelling layers with cross-axis phase modulation.
            s0x16 inner1 = scaleS0x16(PfMath::pfSinTurns(1 * Y + t), perMil(500));
            s0x16 inner2 = scaleS0x16(PfMath::pfCosTurns(1 * Y - t), perMil(500));
            int32_t tex1 = raw(PfMath::pfSinTurns(6 * X + t + raw(inner1)));
            int32_t tex2 = raw(PfMath::pfCosTurns(5 * X - t + raw(inner2)));
            // v in [0,1] Q16.
            uint32_t v1 = static_cast<uint32_t>((tex1 + S0X16_ONE) >> 1);
            uint32_t v2 = static_cast<uint32_t>((tex2 + S0X16_ONE) >> 1);
            // Moving thresholds; thickness lowers them to widen the ribbons.
            int32_t thick = state->thicknessRaw; // [0,1]
            int32_t base = (S0X16_ONE >> 1) - ((thick * 3) >> 4); // 0.5 - 0.1875*thick
            int32_t th1 = base + (raw(PfMath::pfSinTurns(2 * t)) / 5);
            int32_t th2 = base + (raw(PfMath::pfCosTurns(PfMath::pfCoefT(t, 23, 10))) / 5);
            bool m1 = static_cast<int32_t>(v1) > th1;
            bool m2 = static_cast<int32_t>(v2) > th2;
            uint32_t intensity = 0;
            if (m1 && m2) intensity = U0X16_MAX;                 // overlap highlight
            else if (m1) intensity = static_cast<uint32_t>(U0X16_MAX * 6 / 10);
            else if (m2) intensity = static_cast<uint32_t>(U0X16_MAX * 4 / 10);
            // Hue: the mean of the two ribbon textures shifts colour along the weave.
            PatternNormU0x16 hue = PfMath::pfUnitToNorm(static_cast<int32_t>((v1 + v2) >> 1));
            return PaletteSample{hue, PatternNormU0x16(static_cast<uint16_t>(intensity))};
        }

        PaletteSample quadDirectional(int32_t X, int32_t Y) const {
            const int32_t t = state->tTurns;
            int32_t s1 = raw(PfMath::pfSinTurns(4 * X + t));
            int32_t s2 = raw(PfMath::pfSinTurns(5 * Y - PfMath::pfCoefT(t, 11, 10)));
            int32_t s3 = raw(PfMath::pfSinTurns(3 * (X + Y) + PfMath::pfCoefT(t, 4, 5)));
            int32_t s4 = raw(PfMath::pfSinTurns(4 * (X - Y) - PfMath::pfCoefT(t, 9, 10)));
            int32_t sum = s1 + s2 + s3 + s4; // [-4, 4] * S0X16_ONE
            int32_t a = sum < 0 ? -sum : sum;
            // field = |sum|/4 scaled by (0.4 + 0.1*warp); veins where field peaks.
            int32_t fieldQ16 = a >> 2; // [0,1] Q16
            int32_t gain = (S0X16_ONE * 4 / 10) + ((raw(state->warpU0x16)) / 10);
            fieldQ16 = static_cast<int32_t>((static_cast<int64_t>(fieldQ16) * gain) >> 16);
            int32_t inner = (S0X16_ONE * 12 / 10) - fieldQ16; // 1.2 - field
            if (inner < 0) inner = 0;
            // cube (Q16) then clamp to [0,1].
            int64_t c2 = (static_cast<int64_t>(inner) * inner) >> 16;
            int64_t c3 = (c2 * inner) >> 16;
            if (c3 > U0X16_MAX) c3 = U0X16_MAX;
            // Hue: the signed directional sum in [-4, 4] tracks the interference phase.
            PatternNormU0x16 hue = PfMath::pfSignedToNorm(sum, 4 * S0X16_ONE);
            return PaletteSample{hue, PatternNormU0x16(static_cast<uint16_t>(c3))};
        }

        PaletteSample posterized(int32_t X, int32_t Y) const {
            const int32_t t = state->tTurns;
            int32_t s1 = raw(PfMath::pfSinTurns(4 * X + PfMath::pfCoefT(t, 7, 10)));
            int32_t s2 = raw(PfMath::pfCosTurns(4 * Y - PfMath::pfCoefT(t, 1, 2)));
            int32_t s3 = raw(PfMath::pfSinTurns(2 * (X + Y) + t));
            PatternNormU0x16 composite = PfMath::pfSignedToNorm(s1 + s2 + s3, 3 * S0X16_ONE);
            // Hue: the smooth pre-posterized composite drives continuous colour.
            return PaletteSample{composite,
                                 PatternNormU0x16(PfMath::pfPosterize(raw(composite), state->levels))};
        }

        PaletteSample cross(int32_t X, int32_t Y) const {
            const int32_t t = state->tTurns;
            // Warp drifts the bar centres; breathing modulates their width.
            int32_t driftX = raw(scaleS0x16(PfMath::pfSinTurns(PfMath::pfCoefT(t, 1, 1)), state->warpU0x16)) / 4;
            int32_t driftY = raw(scaleS0x16(PfMath::pfCosTurns(PfMath::pfCoefT(t, 3, 2)), state->warpU0x16)) / 4;
            int32_t breathe = raw(PfMath::pfSinTurns(PfMath::pfCoefT(t, 1, 2))); // [-1, 1]
            // Bar half-width 0.10 .. 0.45 of a unit; +/- a third with breathing.
            int32_t base = (S0X16_ONE * 10 / 100)
                           + static_cast<int32_t>((static_cast<int64_t>(state->thicknessRaw) * (S0X16_ONE * 35 / 100)) >> 16);
            int32_t hw = base + static_cast<int32_t>((static_cast<int64_t>(breathe) * (base / 3)) >> 16);
            if (hw < (S0X16_ONE / 100)) hw = S0X16_ONE / 100;
            // Cross is centred in the [0,1] cell, so the bars sit at X,Y == 0.5.
            constexpr int32_t kCentre = S0X16_ONE >> 1;
            uint16_t barV = PfMath::pfBump(X - kCentre + driftX, hw); // vertical bar
            uint16_t barH = PfMath::pfBump(Y - kCentre + driftY, hw); // horizontal bar
            // Hue: a diagonal position gradient tints the two arms differently.
            PatternNormU0x16 hue = PfMath::pfSignedToNorm(X + Y - S0X16_ONE, S0X16_ONE);
            return PaletteSample{hue, PatternNormU0x16(barV > barH ? barV : barH)};
        }

        // Separable standing wave: sin(kx X)*sin(ky Y). Antinodes (|product|~1)
        // glow as a grid of dots; warp detunes the Y axis, thickness sizes dots.
        PaletteSample lattice(int32_t X, int32_t Y) const {
            const int32_t t = state->tTurns;
            int32_t detune = static_cast<int32_t>(
                (static_cast<int64_t>(raw(state->warpU0x16)) * 2 * Y) >> 16);
            int32_t sx = raw(PfMath::pfSinTurns(4 * X + t));
            int32_t sy = raw(PfMath::pfSinTurns(4 * Y + detune - t));
            int32_t prod = static_cast<int32_t>((static_cast<int64_t>(sx) * sy) >> 16); // s0x16
            int32_t aprod = prod < 0 ? -prod : prod;                                    // |prod|
            // Dot half-width 0.125 .. 0.625 of the antinode neighbourhood.
            int32_t half = (S0X16_ONE / 8) + (state->thicknessRaw / 2);
            uint16_t glow = PfMath::pfBump(S0X16_ONE - aprod, half);
            PatternNormU0x16 hue = PfMath::pfSignedToNorm(sx - sy, 2 * S0X16_ONE);
            return PaletteSample{hue, PatternNormU0x16(glow)};
        }

        // Two near-equal gratings per axis beat into slow moire fringes. The
        // product of the pair isolates the beat envelope; thickness bands it.
        PaletteSample moire(int32_t X, int32_t Y) const {
            const int32_t t = state->tTurns;
            const int32_t k = 6;
            int32_t dk = 1 + static_cast<int32_t>(
                (static_cast<int64_t>(raw(state->warpU0x16)) * 6) >> 16); // 1..7
            int32_t g1 = raw(PfMath::pfSinTurns(k * X + t));
            int32_t g2 = raw(PfMath::pfSinTurns((k + dk) * X - t));
            int32_t g3 = raw(PfMath::pfSinTurns(k * Y + PfMath::pfCoefT(t, 3, 4)));
            int32_t g4 = raw(PfMath::pfSinTurns((k + dk) * Y - PfMath::pfCoefT(t, 3, 4)));
            int32_t beatX = static_cast<int32_t>((static_cast<int64_t>(g1) * g2) >> 16);
            int32_t beatY = static_cast<int32_t>((static_cast<int64_t>(g3) * g4) >> 16);
            PatternNormU0x16 composite = PfMath::pfSignedToNorm(beatX + beatY, 2 * S0X16_ONE);
            // thickness widens the fringes -> fewer, chunkier bands (8 down to 2).
            uint8_t levels = static_cast<uint8_t>(
                2 + (((S0X16_ONE - state->thicknessRaw) * 6) >> 16));
            PatternNormU0x16 hue = PfMath::pfSignedToNorm(beatX - beatY, 2 * S0X16_ONE);
            return PaletteSample{hue, PatternNormU0x16(PfMath::pfPosterize(raw(composite), levels))};
        }

        // Chladni figure: field = cos(m X)cos(n Y) - cos(n X)cos(m Y). Sand
        // collects on the nodal curves (field ~ 0), drawn as bright thin lines.
        PaletteSample chladni(int32_t X, int32_t Y) const {
            const int32_t t = state->tTurns;
            const int32_t m = state->levels;   // mode m (>= 2 by construction)
            const int32_t n = m + 1;           // paired mode
            int32_t p = PfMath::pfCoefT(t, 1, 4); // slow morph phase
            int32_t q = p + raw(scaleS0x16(
                PfMath::pfSinTurns(PfMath::pfCoefT(t, 1, 3)), state->warpU0x16)); // axis skew
            int32_t a = raw(PfMath::pfCosTurns(m * X + p));
            int32_t b = raw(PfMath::pfCosTurns(n * Y + q));
            int32_t c = raw(PfMath::pfCosTurns(n * X + q));
            int32_t d = raw(PfMath::pfCosTurns(m * Y + p));
            int32_t ab = static_cast<int32_t>((static_cast<int64_t>(a) * b) >> 16);
            int32_t cd = static_cast<int32_t>((static_cast<int64_t>(c) * d) >> 16);
            int32_t field = ab - cd; // [-2, 2] in s0x16 units
            int32_t half = (S0X16_ONE * 4 / 100) + (state->thicknessRaw / 4); // 0.04 .. 0.29
            uint16_t node = PfMath::pfBump(field, half);
            PatternNormU0x16 hue = PfMath::pfSignedToNorm(ab + cd, 2 * S0X16_ONE);
            return PaletteSample{hue, PatternNormU0x16(node)};
        }

        PaletteSample sample(UV uv) const {
            int32_t X = raw(uv.u);
            int32_t Y = raw(uv.v);
            switch (state->variant) {
                case Variant::DualAxis:        return dualAxis(X, Y);
                case Variant::CounterRibbons:  return counterRibbons(X, Y);
                case Variant::QuadDirectional: return quadDirectional(X, Y);
                case Variant::Posterized:      return posterized(X, Y);
                case Variant::Cross:           return cross(X, Y);
                case Variant::Lattice:         return lattice(X, Y);
                case Variant::Moire:           return moire(X, Y);
                case Variant::Chladni:         return chladni(X, Y);
            }
            return PaletteSample{PatternNormU0x16(0), PatternNormU0x16(0)};
        }

        PatternNormU0x16 operator()(UV uv) const {
            return sample(uv).value();
        }
    };

    PfInterferenceField::PfInterferenceField(
        Variant variant,
        uint8_t posterizeLevels,
        S0x16Signal phaseSpeed,
        S0x16Signal warp,
        S0x16Signal thickness
    ) : state(std::make_shared<State>(
        variant,
        posterizeLevels,
        std::move(phaseSpeed),
        std::move(warp),
        std::move(thickness)
    )) {}

    void PfInterferenceField::advanceFrame(u0x16 progress, TimeMillis elapsedMs) {
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
        s.thicknessRaw = raw(s.thicknessSignal.sample(magnitudeRange(), elapsedMs));

        // Advance internal time: at phaseSpeed=1, ~0.4 turns/sec.
        int32_t deltaTurns = static_cast<int32_t>(
            (static_cast<int64_t>(phaseRaw) * static_cast<int64_t>(dtMs) * 2) / 5000
        );
        s.tTurns += deltaTurns;

        int32_t warpClamped = warpRaw < 0 ? 0 : (warpRaw > static_cast<int32_t>(U0X16_MAX) ? U0X16_MAX : warpRaw);
        s.warpU0x16 = u0x16(static_cast<uint16_t>(warpClamped));
    }

    UVMap PfInterferenceField::layer(const std::shared_ptr<PipelineContext> &context) const {
        (void)context;
        return Functor{state.get()};
    }

    bool PfInterferenceField::emitsColour() const {
        return true;
    }

    UVColourMap PfInterferenceField::colourLayer(const std::shared_ptr<PipelineContext> &context) const {
        (void)context;
        return [f = Functor{state.get()}](UV uv) { return f.sample(uv); };
    }
}
