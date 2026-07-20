//  SPDX-License-Identifier: GPL-3.0-only

/*
 * PatternFlow adaptation NOTICE
 * -----------------------------
 * Field algorithm adapted from PatternFlow (engmung/Patternflow, author
 * Seunghun LEE, CC-BY-SA-4.0): effects 0519-1 (Organic), 0529 (Topographic).
 * Algorithm only, re-derived into fixed point; no source lines copied.
 * GPL-3.0-only via the CC-BY-SA-4.0 -> GPLv3 path. See patternflow/ATTRIBUTION.md.
 */

#include "renderer/pipeline/patterns/patternflow/PfContourField.h"
#include "renderer/pipeline/patterns/patternflow/PfFieldMaths.h"
#include "renderer/pipeline/maths/AngleMaths.h"
#include "renderer/pipeline/maths/ScalarMaths.h"

namespace PolarShader {
    struct PfContourField::State {
        Variant variant;
        uint8_t contourLevels;
        bool hardEdges;
        S0x16Signal phaseSpeedSignal;
        S0x16Signal tensionSignal;

        int32_t tTurns{0};
        int32_t tensionBias{0}; // Q16 bias added to the normalised height (Organic)
        int32_t softHalfRaw{0}; // iso-line half-width in Q16 (Topographic)

        TimeMillis lastElapsedMs{0u};
        bool hasLastElapsed{false};

        State(Variant v, uint8_t levels, bool hard, S0x16Signal ps, S0x16Signal ten)
            : variant(v),
              contourLevels(levels < 1 ? uint8_t(1) : levels),
              hardEdges(hard),
              phaseSpeedSignal(std::move(ps)),
              tensionSignal(std::move(ten)) {}
    };

    struct PfContourField::Functor {
        const State *state;

        // Smooth drifting height field shared by both variants. [0, 65535].
        int32_t heightField(int32_t X, int32_t Y) const {
            const int32_t t = state->tTurns;
            int32_t h = raw(PfMath::pfSinTurns(3 * X + t))
                        + raw(PfMath::pfCosTurns(3 * Y - t))
                        + raw(PfMath::pfSinTurns(2 * (X + Y) + PfMath::pfCoefT(t, 13, 10)))
                        + raw(PfMath::pfCosTurns(3 * (X - Y) - PfMath::pfCoefT(t, 7, 10)));
            return raw(PfMath::pfSignedToNorm(h, 4 * S0X16_ONE));
        }

        PatternNormU16 organic(int32_t X, int32_t Y) const {
            int32_t height = heightField(X, Y);
            height += state->tensionBias;                              // DC shift
            if (height < 0) height += U0X16_MAX;
            height &= U0X16_MAX;

            // Slice into contour bands and read a triangle ridge per band.
            uint32_t prod = static_cast<uint32_t>(height) * state->contourLevels;
            uint32_t frac = prod & U0X16_MAX;
            uint32_t ridge = (frac < (U0X16_MAX >> 1)) ? (frac << 1) : ((U0X16_MAX - frac) << 1);
            if (ridge > U0X16_MAX) ridge = U0X16_MAX;

            if (state->hardEdges) {
                ridge = (ridge > (U0X16_MAX >> 1)) ? U0X16_MAX : 0u;
            }
            return PatternNormU16(static_cast<uint16_t>(ridge));
        }

        PatternNormU16 topographic(int32_t X, int32_t Y) const {
            int32_t height = heightField(X, Y);
            // Position within the current contour band, in Q16 [0, 65535].
            uint32_t prod = static_cast<uint32_t>(height) * state->contourLevels;
            int32_t frac = static_cast<int32_t>(prod & U0X16_MAX);
            // Distance to the nearest band edge (a level-set line).
            int32_t edgeDist = frac < (U0X16_MAX >> 1) ? frac : (U0X16_MAX - frac);
            // Thin bright iso-line; tension widens/softens it.
            uint16_t line = PfMath::pfBump(edgeDist, state->softHalfRaw);
            if (state->hardEdges) {
                line = (line > (U0X16_MAX >> 1)) ? U0X16_MAX : 0u;
            }
            return PatternNormU16(line);
        }

        PatternNormU16 operator()(UV uv) const {
            int32_t X = raw(uv.u);
            int32_t Y = raw(uv.v);
            switch (state->variant) {
                case Variant::Organic:     return organic(X, Y);
                case Variant::Topographic: return topographic(X, Y);
            }
            return PatternNormU16(0);
        }
    };

    PfContourField::PfContourField(
        Variant variant,
        uint8_t contourLevels,
        bool hardEdges,
        S0x16Signal phaseSpeed,
        S0x16Signal tension
    ) : state(std::make_shared<State>(
        variant,
        contourLevels,
        hardEdges,
        std::move(phaseSpeed),
        std::move(tension)
    )) {}

    void PfContourField::advanceFrame(u0x16 progress, TimeMillis elapsedMs) {
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
        int32_t tensionRaw = raw(s.tensionSignal.sample(magnitudeRange(), elapsedMs));

        int32_t deltaTurns = static_cast<int32_t>(
            (static_cast<int64_t>(phaseRaw) * static_cast<int64_t>(dtMs) * 2) / 5000
        );
        s.tTurns += deltaTurns;

        // tension in [0,1] -> [-0.5, +0.5] turn/height bias (Organic).
        s.tensionBias = (tensionRaw >> 1) - (S0X16_ONE >> 2);

        // tension in [0,1] -> iso-line half-width 0.03..0.20 of a band (Topographic).
        int32_t tenClamped = tensionRaw < 0 ? 0 : (tensionRaw > static_cast<int32_t>(U0X16_MAX) ? U0X16_MAX : tensionRaw);
        s.softHalfRaw = (U0X16_MAX * 3 / 100) + static_cast<int32_t>(
            (static_cast<int64_t>(tenClamped) * (U0X16_MAX * 17 / 100)) >> 16
        );
    }

    UVMap PfContourField::layer(const std::shared_ptr<PipelineContext> &context) const {
        (void)context;
        return Functor{state.get()};
    }
}
