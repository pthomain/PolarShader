//  SPDX-License-Identifier: GPL-3.0-only

/*
 * PatternFlow adaptation NOTICE
 * -----------------------------
 * Field algorithms adapted from PatternFlow (engmung/Patternflow, author
 * Seunghun LEE, CC-BY-SA-4.0): Origin (ConcentricGrid), 0511 (RowSegments,
 * approx), 0514 (Shapes), 0528 (Dots), 0601 (WaveMatrix), 0624 (RadialPulse,
 * approx). Algorithm only, re-derived into fixed point; no source lines copied.
 * GPL-3.0-only via the CC-BY-SA-4.0 -> GPLv3 path. See patternflow/ATTRIBUTION.md.
 */

#include "renderer/pipeline/patterns/patternflow/PfCellularField.h"
#include "renderer/pipeline/patterns/patternflow/PfFieldMaths.h"
#include "renderer/pipeline/maths/AngleMaths.h"
#include "renderer/pipeline/maths/ScalarMaths.h"

namespace PolarShader {
    namespace {
        constexpr int32_t kHalf = U0X16_MAX >> 1; // 0.5 in Q16 local-cell units
    }

    struct PfCellularField::State {
        Variant variant;
        uint16_t cellCount;
        S0x16Signal phaseSpeedSignal;
        S0x16Signal warpSignal;
        S0x16Signal thicknessSignal;

        int32_t tTurns{0};
        int32_t warpRaw{0};   // [0, 65535] warp/chaos depth
        int32_t sizeRaw{0};   // feature half-width / radius in local-cell Q16
        int32_t threshRaw{0}; // activation threshold in [0, 65535]

        TimeMillis lastElapsedMs{0u};
        bool hasLastElapsed{false};

        State(Variant v, uint8_t count, S0x16Signal ps, S0x16Signal w, S0x16Signal th)
            : variant(v),
              cellCount(count < 1 ? uint16_t(1) : uint16_t(count)),
              phaseSpeedSignal(std::move(ps)),
              warpSignal(std::move(w)),
              thicknessSignal(std::move(th)) {}
    };

    struct PfCellularField::Functor {
        const State *state;

        PatternNormU16 concentricGrid(uint16_t localX, uint16_t localY) const {
            const int32_t t = state->tTurns;
            int32_t dx = static_cast<int32_t>(localX) - kHalf;
            int32_t dy = static_cast<int32_t>(localY) - kHalf;
            uint32_t dist = PfMath::pfDistQ16(dx, dy);
            int32_t ring = raw(PfMath::pfSinTurns(static_cast<int32_t>(4 * dist) - t));
            return PfMath::pfSignedToNorm(ring, S0X16_ONE);
        }

        PatternNormU16 rowSegments(int32_t u01, uint16_t cellY) const {
            const int32_t t = state->tTurns;
            int32_t dir = (cellY & 1u) ? 1 : -1;
            int32_t shifted = u01 + PfMath::pfCoefT(t, dir, 1);
            uint16_t segIdx, segLocal;
            PfMath::pfCell(static_cast<int32_t>(static_cast<uint32_t>(shifted) & U0X16_MAX),
                           state->cellCount, segIdx, segLocal);
            uint16_t h = PfMath::pfHash2(segIdx, static_cast<uint32_t>(cellY));
            if (h < 30000u) return PatternNormU16(0);
            uint16_t bar = PfMath::pfBump(static_cast<int32_t>(segLocal) - kHalf, state->sizeRaw);
            return PatternNormU16(bar);
        }

        PatternNormU16 shapes(uint16_t cellX, uint16_t cellY,
                              uint16_t localX, uint16_t localY) const {
            const int32_t t = state->tTurns;
            uint16_t present = PfMath::pfHash2(cellX * 3u + 1u, cellY * 5u + 2u);
            if (present < 18000u) return PatternNormU16(0);
            uint16_t hx = PfMath::pfHash2(cellX, cellY);
            uint16_t hy = PfMath::pfHash2(cellY + 101u, cellX + 7u);
            int32_t wobX = raw(PfMath::pfSinTurns(t + static_cast<int32_t>(hx)));
            int32_t wobY = raw(PfMath::pfCosTurns(t + static_cast<int32_t>(hy)));
            int32_t jitX = (static_cast<int32_t>(hx) - kHalf) + (wobX >> 4);
            int32_t jitY = (static_cast<int32_t>(hy) - kHalf) + (wobY >> 4);
            int32_t cx = kHalf + static_cast<int32_t>((static_cast<int64_t>(jitX) * state->warpRaw) >> 16);
            int32_t cy = kHalf + static_cast<int32_t>((static_cast<int64_t>(jitY) * state->warpRaw) >> 16);
            uint32_t dist = PfMath::pfDistQ16(static_cast<int32_t>(localX) - cx,
                                              static_cast<int32_t>(localY) - cy);
            uint16_t blob = PfMath::pfBump(static_cast<int32_t>(dist), state->sizeRaw);
            return PatternNormU16(blob);
        }

        PatternNormU16 dots(uint16_t cellX, uint16_t cellY,
                            uint16_t localX, uint16_t localY) const {
            const int32_t t = state->tTurns;
            int32_t energyS = raw(PfMath::pfSinTurns(
                static_cast<int32_t>(cellX) * 9000 + static_cast<int32_t>(cellY) * 13000 + t));
            int32_t energy01 = (energyS + S0X16_ONE) >> 1; // [0, 65536]
            if (energy01 <= state->threshRaw) return PatternNormU16(0);
            int32_t dx = static_cast<int32_t>(localX) - kHalf;
            int32_t dy = static_cast<int32_t>(localY) - kHalf;
            uint32_t dist = PfMath::pfDistQ16(dx, dy);
            uint16_t dot = PfMath::pfBump(static_cast<int32_t>(dist), state->sizeRaw);
            uint32_t scaled = (static_cast<uint32_t>(dot) * static_cast<uint32_t>(energy01)) >> 16;
            if (scaled > U0X16_MAX) scaled = U0X16_MAX;
            return PatternNormU16(static_cast<uint16_t>(scaled));
        }

        PatternNormU16 waveMatrix(uint16_t cellX, uint16_t cellY) const {
            const int32_t t = state->tTurns;
            int32_t split = (cellX & 1u) ? state->warpRaw : 0;
            int32_t phase = t + static_cast<int32_t>(cellX) * 7000
                            + static_cast<int32_t>(cellY) * 11000 + split;
            int32_t s = raw(PfMath::pfSinTurns(phase));
            int32_t sig01 = (s + S0X16_ONE) >> 1; // [0, 65536]
            if (sig01 <= state->threshRaw) return PatternNormU16(0);
            return PfMath::pfUnitToNorm(sig01);
        }

        PatternNormU16 radialPulse(uint16_t cellX, uint16_t cellY,
                                   uint16_t localX, uint16_t localY) const {
            const int32_t t = state->tTurns;
            // Cell centre in [0,1] Q16, measured from the grid centre (0.5).
            int32_t span = static_cast<int32_t>(state->cellCount);
            int32_t centreX = ((2 * static_cast<int32_t>(cellX) + 1) * U0X16_MAX) / (2 * span);
            int32_t centreY = ((2 * static_cast<int32_t>(cellY) + 1) * U0X16_MAX) / (2 * span);
            uint32_t cellDist = PfMath::pfDistQ16(centreX - kHalf, centreY - kHalf);
            // Outward-travelling pulse; warp scatters the ring phase per cell.
            int32_t jitter = (static_cast<int32_t>(PfMath::pfHash2(cellX, cellY)) - kHalf);
            int32_t phase = static_cast<int32_t>(3 * cellDist) - t
                            + static_cast<int32_t>((static_cast<int64_t>(jitter) * state->warpRaw) >> 18);
            int32_t pulse = raw(PfMath::pfSinTurns(phase));
            int32_t height01 = (pulse + S0X16_ONE) >> 1; // [0, 65536]
            // Per-cell pillar: bright disc whose brightness rides the pulse.
            int32_t dx = static_cast<int32_t>(localX) - kHalf;
            int32_t dy = static_cast<int32_t>(localY) - kHalf;
            uint32_t dist = PfMath::pfDistQ16(dx, dy);
            uint16_t pillar = PfMath::pfBump(static_cast<int32_t>(dist), state->sizeRaw);
            uint32_t scaled = (static_cast<uint32_t>(pillar) * static_cast<uint32_t>(height01)) >> 16;
            if (scaled > U0X16_MAX) scaled = U0X16_MAX;
            return PatternNormU16(static_cast<uint16_t>(scaled));
        }

        PatternNormU16 operator()(UV uv) const {
            // Renderer feeds normalised [0,1] UVs (polarToCartesianUV). The cell
            // grid is periodic, so wrap the coordinate modulo one unit: zoomed-out
            // samples outside [0,1] repeat the grid instead of collapsing onto the
            // border cell. (U0X16_MAX+1 == 65536 == 1.0, so & U0X16_MAX == mod 1.0.)
            int32_t u01 = static_cast<int32_t>(static_cast<uint32_t>(raw(uv.u)) & U0X16_MAX);
            int32_t v01 = static_cast<int32_t>(static_cast<uint32_t>(raw(uv.v)) & U0X16_MAX);

            uint16_t cellX, localX, cellY, localY;
            PfMath::pfCell(u01, state->cellCount, cellX, localX);
            PfMath::pfCell(v01, state->cellCount, cellY, localY);

            switch (state->variant) {
                case Variant::ConcentricGrid: return concentricGrid(localX, localY);
                case Variant::RowSegments:    return rowSegments(u01, cellY);
                case Variant::Shapes:         return shapes(cellX, cellY, localX, localY);
                case Variant::Dots:           return dots(cellX, cellY, localX, localY);
                case Variant::WaveMatrix:     return waveMatrix(cellX, cellY);
                case Variant::RadialPulse:    return radialPulse(cellX, cellY, localX, localY);
            }
            return PatternNormU16(0);
        }
    };

    PfCellularField::PfCellularField(
        Variant variant,
        uint8_t cellCount,
        S0x16Signal phaseSpeed,
        S0x16Signal warp,
        S0x16Signal thickness
    ) : state(std::make_shared<State>(
        variant,
        cellCount,
        std::move(phaseSpeed),
        std::move(warp),
        std::move(thickness)
    )) {}

    void PfCellularField::advanceFrame(u0x16 progress, TimeMillis elapsedMs) {
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

        int32_t warpClamped = warpRaw < 0 ? 0 : (warpRaw > static_cast<int32_t>(U0X16_MAX) ? U0X16_MAX : warpRaw);
        s.warpRaw = warpClamped;

        int32_t thickClamped = thickRaw < 0 ? 0 : (thickRaw > static_cast<int32_t>(U0X16_MAX) ? U0X16_MAX : thickRaw);
        // Feature radius: 0.125 .. ~0.75 of a cell half-span as thickness goes 0 -> 1.
        s.sizeRaw = (U0X16_MAX >> 3) + static_cast<int32_t>(
            (static_cast<int64_t>(thickClamped) * (U0X16_MAX >> 1)) >> 16
        );
        // Activation threshold: thicker features -> lower gate -> more coverage.
        s.threshRaw = U0X16_MAX - thickClamped;
    }

    UVMap PfCellularField::layer(const std::shared_ptr<PipelineContext> &context) const {
        (void)context;
        return Functor{state.get()};
    }
}
