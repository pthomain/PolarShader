//  SPDX-License-Identifier: GPL-3.0-only

/*
 * PatternFlow adaptation NOTICE
 * -----------------------------
 * Pattern factories for the PatternFlow field family adapted from
 * engmung/Patternflow (author Seunghun LEE, CC-BY-SA-4.0). Algorithm only,
 * re-derived into fixed point; no source lines copied. GPL-3.0-only via the
 * CC-BY-SA-4.0 -> GPLv3 path. See patternflow/ATTRIBUTION.md.
 */

#include "renderer/pipeline/patterns/patternflow/PatternFlow.h"
#include <utility>

namespace PolarShader {
    std::unique_ptr<UVPattern> pfDualAxis(
        Sf16Signal phaseSpeed, Sf16Signal warp, Sf16Signal thickness
    ) {
        return std::make_unique<PfInterferenceField>(
            PfInterferenceField::Variant::DualAxis, 5,
            std::move(phaseSpeed), std::move(warp), std::move(thickness));
    }

    std::unique_ptr<UVPattern> pfCounterRibbons(
        Sf16Signal phaseSpeed, Sf16Signal warp, Sf16Signal thickness
    ) {
        return std::make_unique<PfInterferenceField>(
            PfInterferenceField::Variant::CounterRibbons, 5,
            std::move(phaseSpeed), std::move(warp), std::move(thickness));
    }

    std::unique_ptr<UVPattern> pfQuadDirectional(
        Sf16Signal phaseSpeed, Sf16Signal warp, Sf16Signal thickness
    ) {
        return std::make_unique<PfInterferenceField>(
            PfInterferenceField::Variant::QuadDirectional, 5,
            std::move(phaseSpeed), std::move(warp), std::move(thickness));
    }

    std::unique_ptr<UVPattern> pfPosterized(
        uint8_t posterizeLevels, Sf16Signal phaseSpeed, Sf16Signal warp, Sf16Signal thickness
    ) {
        return std::make_unique<PfInterferenceField>(
            PfInterferenceField::Variant::Posterized, posterizeLevels,
            std::move(phaseSpeed), std::move(warp), std::move(thickness));
    }

    std::unique_ptr<UVPattern> pfCross(
        Sf16Signal phaseSpeed, Sf16Signal warp, Sf16Signal thickness
    ) {
        return std::make_unique<PfInterferenceField>(
            PfInterferenceField::Variant::Cross, 5,
            std::move(phaseSpeed), std::move(warp), std::move(thickness));
    }

    std::unique_ptr<UVPattern> pfPetals(
        uint8_t petalCount, Sf16Signal phaseSpeed, Sf16Signal fold, Sf16Signal thickness
    ) {
        return std::make_unique<PfRadialField>(
            PfRadialField::Variant::Petals, petalCount,
            std::move(phaseSpeed), std::move(fold), std::move(thickness));
    }

    std::unique_ptr<UVPattern> pfRipple(
        uint8_t waveCount, Sf16Signal phaseSpeed, Sf16Signal warp, Sf16Signal thickness
    ) {
        return std::make_unique<PfRadialField>(
            PfRadialField::Variant::Ripple, waveCount,
            std::move(phaseSpeed), std::move(warp), std::move(thickness));
    }

    std::unique_ptr<UVPattern> pfOrganic(
        uint8_t contourLevels, bool hardEdges, Sf16Signal phaseSpeed, Sf16Signal tension
    ) {
        return std::make_unique<PfContourField>(
            PfContourField::Variant::Organic, contourLevels, hardEdges,
            std::move(phaseSpeed), std::move(tension));
    }

    std::unique_ptr<UVPattern> pfTopographic(
        uint8_t contourLevels, bool hardEdges, Sf16Signal phaseSpeed, Sf16Signal tension
    ) {
        return std::make_unique<PfContourField>(
            PfContourField::Variant::Topographic, contourLevels, hardEdges,
            std::move(phaseSpeed), std::move(tension));
    }

    std::unique_ptr<UVPattern> pfPlasma(
        Sf16Signal phaseSpeed, Sf16Signal warp, Sf16Signal thickness
    ) {
        return std::make_unique<PfPlasmaWarp>(
            PfPlasmaWarp::Variant::Plasma,
            std::move(phaseSpeed), std::move(warp), std::move(thickness));
    }

    std::unique_ptr<UVPattern> pfTendrils(
        Sf16Signal phaseSpeed, Sf16Signal warp, Sf16Signal thickness
    ) {
        return std::make_unique<PfPlasmaWarp>(
            PfPlasmaWarp::Variant::Tendrils,
            std::move(phaseSpeed), std::move(warp), std::move(thickness));
    }

    std::unique_ptr<UVPattern> pfLiquidGate(
        Sf16Signal phaseSpeed, Sf16Signal warp, Sf16Signal thickness
    ) {
        return std::make_unique<PfPlasmaWarp>(
            PfPlasmaWarp::Variant::LiquidGate,
            std::move(phaseSpeed), std::move(warp), std::move(thickness));
    }

    std::unique_ptr<UVPattern> pfConcentricGrid(
        uint8_t cellCount, Sf16Signal phaseSpeed, Sf16Signal warp, Sf16Signal thickness
    ) {
        return std::make_unique<PfCellularField>(
            PfCellularField::Variant::ConcentricGrid, cellCount,
            std::move(phaseSpeed), std::move(warp), std::move(thickness));
    }

    std::unique_ptr<UVPattern> pfRowSegments(
        uint8_t cellCount, Sf16Signal phaseSpeed, Sf16Signal warp, Sf16Signal thickness
    ) {
        return std::make_unique<PfCellularField>(
            PfCellularField::Variant::RowSegments, cellCount,
            std::move(phaseSpeed), std::move(warp), std::move(thickness));
    }

    std::unique_ptr<UVPattern> pfShapes(
        uint8_t cellCount, Sf16Signal phaseSpeed, Sf16Signal warp, Sf16Signal thickness
    ) {
        return std::make_unique<PfCellularField>(
            PfCellularField::Variant::Shapes, cellCount,
            std::move(phaseSpeed), std::move(warp), std::move(thickness));
    }

    std::unique_ptr<UVPattern> pfDots(
        uint8_t cellCount, Sf16Signal phaseSpeed, Sf16Signal warp, Sf16Signal thickness
    ) {
        return std::make_unique<PfCellularField>(
            PfCellularField::Variant::Dots, cellCount,
            std::move(phaseSpeed), std::move(warp), std::move(thickness));
    }

    std::unique_ptr<UVPattern> pfWaveMatrix(
        uint8_t cellCount, Sf16Signal phaseSpeed, Sf16Signal warp, Sf16Signal thickness
    ) {
        return std::make_unique<PfCellularField>(
            PfCellularField::Variant::WaveMatrix, cellCount,
            std::move(phaseSpeed), std::move(warp), std::move(thickness));
    }

    std::unique_ptr<UVPattern> pfRadialPulse(
        uint8_t cellCount, Sf16Signal phaseSpeed, Sf16Signal warp, Sf16Signal thickness
    ) {
        return std::make_unique<PfCellularField>(
            PfCellularField::Variant::RadialPulse, cellCount,
            std::move(phaseSpeed), std::move(warp), std::move(thickness));
    }
}
