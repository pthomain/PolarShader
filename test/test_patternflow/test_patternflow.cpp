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

/*
 * Native invariant tests for the PatternFlow field family.
 * Pattern-functor invariants cover the pf* factories (range, purity,
 * animation, deterministic replay).
 */

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#include "native/FastLED.h"
#endif
#include <unity.h>
#include <vector>
#include <set>
#include <functional>

#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/patterns/patternflow/PfFieldMaths.h"
#include "renderer/pipeline/patterns/patternflow/PatternFlow.h"

#ifndef ARDUINO
#include "renderer/pipeline/maths/src/PolarMaths.cpp"
#include "renderer/pipeline/maths/src/NoiseMaths.cpp"
#include "renderer/pipeline/maths/src/PatternMaths.cpp"
#include "renderer/pipeline/maths/src/TimeMaths.cpp"
#include "renderer/pipeline/maths/src/TilingMaths.cpp"
#include "renderer/pipeline/signals/src/Signals.cpp"
#include "renderer/pipeline/signals/src/SignalSamplers.cpp"
#include "renderer/pipeline/signals/src/accumulators/Accumulators.cpp"
#include "renderer/pipeline/patterns/src/base/UVPattern.cpp"
#include "renderer/pipeline/patterns/patternflow/src/PfInterferenceField.cpp"
#include "renderer/pipeline/patterns/patternflow/src/PfRadialField.cpp"
#include "renderer/pipeline/patterns/patternflow/src/PfContourField.cpp"
#include "renderer/pipeline/patterns/patternflow/src/PfPlasmaWarp.cpp"
#include "renderer/pipeline/patterns/patternflow/src/PfCellularField.cpp"
#include "renderer/pipeline/patterns/patternflow/src/PfSourceField.cpp"
#include "renderer/pipeline/patterns/patternflow/src/PatternFlow.cpp"
#endif

using namespace PolarShader;

// --- Shared sampling helpers -------------------------------------------------

using Factory = std::function<std::unique_ptr<UVPattern>()>;

static const char *kFactoryNames[] = {
    "pfConcentricGrid", "pfDualAxis", "pfRowSegments", "pfPetals", "pfShapes",
    "pfCounterRibbons", "pfQuadDirectional", "pfOrganic", "pfTendrils",
    "pfRipple", "pfDots", "pfTopographic", "pfLiquidGate", "pfPosterized",
    "pfWaveMatrix", "pfPlasma", "pfCross", "pfRadialPulse",
    "pfLattice", "pfMoire", "pfChladni", "pfChirp", "pfSpiralArms", "pfRippleTank"
};

static std::vector<Factory> makeFactories() {
    return {
        []{ return pfConcentricGrid(); },
        []{ return pfDualAxis(); },
        []{ return pfRowSegments(); },
        []{ return pfPetals(); },
        []{ return pfShapes(); },
        []{ return pfCounterRibbons(); },
        []{ return pfQuadDirectional(); },
        []{ return pfOrganic(); },
        []{ return pfTendrils(); },
        []{ return pfRipple(); },
        []{ return pfDots(); },
        []{ return pfTopographic(); },
        []{ return pfLiquidGate(); },
        []{ return pfPosterized(); },
        []{ return pfWaveMatrix(); },
        []{ return pfPlasma(); },
        []{ return pfCross(); },
        []{ return pfRadialPulse(); },
        []{ return pfLattice(); },
        []{ return pfMoire(); },
        []{ return pfChladni(); },
        []{ return pfChirp(); },
        []{ return pfSpiralArms(); },
        []{ return pfRippleTank(); },
    };
}

// Sample a UV functor over the renderer's normalised [0,1] domain (raw
// [0, U0X16_MAX]) in both axes, matching what polarToCartesianUV feeds patterns.
static void gridSample(const UVMap &m, uint16_t &mn, uint16_t &mx, uint64_t &sum) {
    for (int i = 0; i <= 16; ++i) {
        for (int j = 0; j <= 16; ++j) {
            UV uv(fl::s16x16::from_raw(i * 4095), fl::s16x16::from_raw(j * 4095));
            uint16_t v = raw(m(uv));
            if (v < mn) mn = v;
            if (v > mx) mx = v;
            sum += v;
        }
    }
}

static void advanceN(UVPattern &p, int frames, TimeMillis step) {
    for (int i = 1; i <= frames; ++i) {
        p.advanceFrame(u0x16(0), static_cast<TimeMillis>(i) * step);
    }
}

// --- PfFieldMaths unit tests -------------------------------------------------

void test_pffieldmaths_bump() {
    // Centre is (near) full, boundary and beyond are zero, monotone decay.
    // The (1 - t^2)^2 window rounds to 65534 at t == 0, not exactly 65535.
    TEST_ASSERT_TRUE(PfMath::pfBump(0, 1000) >= 65000);
    TEST_ASSERT_EQUAL_UINT16(0, PfMath::pfBump(1000, 1000));
    TEST_ASSERT_EQUAL_UINT16(0, PfMath::pfBump(2000, 1000));
    TEST_ASSERT_EQUAL_UINT16(0, PfMath::pfBump(500, 0)); // degenerate width
    TEST_ASSERT_TRUE(PfMath::pfBump(250, 1000) > PfMath::pfBump(750, 1000));
}

void test_pffieldmaths_signed_to_norm() {
    TEST_ASSERT_EQUAL_UINT16(0, raw(PfMath::pfSignedToNorm(-1000, 1000)));
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, raw(PfMath::pfSignedToNorm(1000, 1000)));
    TEST_ASSERT_UINT16_WITHIN(2, 0x7FFF, raw(PfMath::pfSignedToNorm(0, 1000)));
}

void test_pffieldmaths_posterize() {
    // Levels collapse the range into flat bands with distinct edges.
    TEST_ASSERT_EQUAL_UINT16(0, PfMath::pfPosterize(0, 4));
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, PfMath::pfPosterize(0xFFFF, 4));
    TEST_ASSERT_EQUAL_UINT16(1234, PfMath::pfPosterize(1234, 1)); // passthrough
    std::set<uint16_t> bands;
    for (uint32_t v = 0; v <= 0xFFFF; v += 257) bands.insert(PfMath::pfPosterize(v, 4));
    TEST_ASSERT_EQUAL_UINT32(4, bands.size());
}

void test_pffieldmaths_cell() {
    uint16_t idx, local;
    PfMath::pfCell(0, 4, idx, local);
    TEST_ASSERT_EQUAL_UINT16(0, idx);
    PfMath::pfCell(U0X16_MAX, 4, idx, local);
    TEST_ASSERT_EQUAL_UINT16(3, idx);
    PfMath::pfCell(U0X16_MAX / 2, 4, idx, local);
    TEST_ASSERT_TRUE(idx == 1 || idx == 2);
}

// --- Pattern-functor invariants ----------------------------------------------

void test_pattern_dynamic_range() {
    auto factories = makeFactories();
    for (size_t k = 0; k < factories.size(); ++k) {
        auto p = factories[k]();
        auto ctx = std::make_shared<PipelineContext>();
        // Accumulate range across several frames so a single sparse frame
        // (gated variants) cannot masquerade as a dead field.
        uint16_t mn = 0xFFFF, mx = 0;
        uint64_t sum = 0;
        for (int f = 0; f < 6; ++f) {
            p->advanceFrame(u0x16(0), static_cast<TimeMillis>(f) * 250u);
            gridSample(p->layer(ctx), mn, mx, sum);
        }
        char msg[64];
        snprintf(msg, sizeof(msg), "%s flat/dead field", kFactoryNames[k]);
        TEST_ASSERT_TRUE_MESSAGE(mx > 1000, msg);
        TEST_ASSERT_TRUE_MESSAGE((mx - mn) > 500, msg);
    }
}

void test_pattern_purity() {
    auto factories = makeFactories();
    auto ctx = std::make_shared<PipelineContext>();
    for (size_t k = 0; k < factories.size(); ++k) {
        auto p = factories[k]();
        advanceN(*p, 3, 200u);
        UVMap m = p->layer(ctx);
        UV uv(fl::s16x16::from_raw(12345), fl::s16x16::from_raw(-23456));
        // Same UV, no intervening advanceFrame -> identical (dual-core safe).
        TEST_ASSERT_EQUAL_UINT16(raw(m(uv)), raw(m(uv)));
    }
}

void test_pattern_animates() {
    auto factories = makeFactories();
    auto ctx = std::make_shared<PipelineContext>();
    for (size_t k = 0; k < factories.size(); ++k) {
        auto p = factories[k]();
        uint16_t mn0 = 0xFFFF, mx0 = 0; uint64_t s0 = 0;
        p->advanceFrame(u0x16(0), 100u);
        gridSample(p->layer(ctx), mn0, mx0, s0);
        uint16_t mn1 = 0xFFFF, mx1 = 0; uint64_t s1 = 0;
        p->advanceFrame(u0x16(0), 1600u);
        gridSample(p->layer(ctx), mn1, mx1, s1);
        char msg[64];
        snprintf(msg, sizeof(msg), "%s does not animate", kFactoryNames[k]);
        TEST_ASSERT_TRUE_MESSAGE(s0 != s1, msg);
    }
}

void test_pattern_deterministic_replay() {
    auto factories = makeFactories();
    auto ctx = std::make_shared<PipelineContext>();
    const TimeMillis seq[] = {0u, 137u, 400u, 900u, 1500u};
    for (size_t k = 0; k < factories.size(); ++k) {
        auto a = factories[k]();
        auto b = factories[k]();
        for (TimeMillis t : seq) {
            a->advanceFrame(u0x16(0), t);
            b->advanceFrame(u0x16(0), t);
        }
        uint16_t amn = 0xFFFF, amx = 0; uint64_t as = 0;
        uint16_t bmn = 0xFFFF, bmx = 0; uint64_t bs = 0;
        gridSample(a->layer(ctx), amn, amx, as);
        gridSample(b->layer(ctx), bmn, bmx, bs);
        char msg[64];
        snprintf(msg, sizeof(msg), "%s non-deterministic", kFactoryNames[k]);
        TEST_ASSERT_EQUAL_UINT64_MESSAGE(as, bs, msg);
    }
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_pffieldmaths_bump);
    RUN_TEST(test_pffieldmaths_signed_to_norm);
    RUN_TEST(test_pffieldmaths_posterize);
    RUN_TEST(test_pffieldmaths_cell);
    RUN_TEST(test_pattern_dynamic_range);
    RUN_TEST(test_pattern_purity);
    RUN_TEST(test_pattern_animates);
    RUN_TEST(test_pattern_deterministic_replay);
    return UNITY_END();
}
