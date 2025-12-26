#ifndef LED_SEGMENTS_SPECS_POLARUTILS_H
#define LED_SEGMENTS_SPECS_POLARUTILS_H

#include "FastLED.h"
#include "PolarLayers.h"

namespace LEDSegments {

    struct CRGB16 {
        uint16_t r;
        uint16_t g;
        uint16_t b;

        CRGB16(uint16_t r = 0, uint16_t g = 0, uint16_t b = 0) : r(r), g(g), b(b) {
        }

        CRGB16 &operator+=(const CRGB &rhs) {
            r += rhs.r;
            g += rhs.g;
            b += rhs.b;
            return *this;
        }
    };

    inline void fillPolar(
        CRGB *segmentArray,
        uint16_t pixelIndex,
        uint16_t angle,
        fract16 radius,
        unsigned long timeInMillis,
        const fl::vector<PolarLayer> &layers
    ) {
        if (layers.empty()) {
            segmentArray[pixelIndex] = CRGB::Black;
            return;
        }

        CRGB16 blended16;

        for (const auto &layer: layers) {
            CRGB value = layer(
                angle,
                radius,
                timeInMillis
            );
            blended16 += value;
        }

        uint16_t max_val = max(blended16.r, max(blended16.g, blended16.b));

        if (max_val > UINT8_MAX) {
            uint16_t scale = (UINT8_MAX * UINT16_MAX) / max_val;
            blended16.r = scale16(blended16.r, scale);
            blended16.g = scale16(blended16.g, scale);
            blended16.b = scale16(blended16.b, scale);
        }

        segmentArray[pixelIndex] = CRGB(blended16.r, blended16.g, blended16.b);
    }
}
#endif //LED_SEGMENTS_SPECS_POLARUTILS_H
