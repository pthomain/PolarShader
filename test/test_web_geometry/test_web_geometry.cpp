//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#include <cmath>

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif
#include <unity.h>

#include "display/WebDisplayGeometry.h"

using namespace PolarShader;

namespace {
    float ringRadius(const WebDisplayGeometry &geometry, const WebDisplayPoint &point) {
        const float dx = point.x - geometry.centerX;
        const float dy = point.y - geometry.centerY;
        return std::sqrt((dx * dx) + (dy * dy));
    }
}

void test_fabric_geometry_matches_serpentine_wiring() {
    const WebDisplayGeometry geometry = buildFabricWebGeometry();

    TEST_ASSERT_EQUAL_UINT16(FabricDisplaySpec::WIDTH * FabricDisplaySpec::HEIGHT, geometry.points.size());
    TEST_ASSERT_TRUE(geometry.diameter > 0.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, geometry.points[0].x);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, geometry.points[0].y);

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 19.0f, geometry.points[19].x);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, geometry.points[19].y);

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 19.0f, geometry.points[20].x);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, geometry.points[20].y);

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, geometry.points[39].x);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, geometry.points[39].y);
}

void test_round_geometry_matches_ring_counts_and_radii() {
    RoundDisplaySpec spec;
    const WebDisplayGeometry geometry = buildRoundWebGeometry(spec);

    TEST_ASSERT_EQUAL_UINT16(spec.nbLeds(), geometry.points.size());
    TEST_ASSERT_TRUE(geometry.diameter > 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, geometry.centerX, geometry.points[0].x);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, geometry.centerY, geometry.points[0].y);

    std::size_t offset = 0;
    float previousRadius = -1.0f;

    for (uint16_t segmentIndex = 0; segmentIndex < spec.numSegments(); ++segmentIndex) {
        const uint16_t segmentLength = spec.segmentSize(segmentIndex);
        const float expectedRadius = static_cast<float>(segmentIndex);

        for (uint16_t pixelInSegment = 0; pixelInSegment < segmentLength; ++pixelInSegment) {
            const float actualRadius = ringRadius(geometry, geometry.points[offset + pixelInSegment]);
            TEST_ASSERT_FLOAT_WITHIN(0.001f, expectedRadius, actualRadius);
        }

        if (segmentIndex > 0) {
            TEST_ASSERT_TRUE(expectedRadius > previousRadius);
        }

        previousRadius = expectedRadius;
        offset += segmentLength;
    }

    TEST_ASSERT_EQUAL_UINT16(spec.nbLeds(), offset);
}

#ifdef ARDUINO
void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_fabric_geometry_matches_serpentine_wiring);
    RUN_TEST(test_round_geometry_matches_ring_counts_and_radii);
    UNITY_END();
}

void loop() {}
#else
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_fabric_geometry_matches_serpentine_wiring);
    RUN_TEST(test_round_geometry_matches_ring_counts_and_radii);
    return UNITY_END();
}
#endif
