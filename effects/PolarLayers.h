#ifndef LED_SEGMENTS_SPECS_POLARLAYERS_H
#define LED_SEGMENTS_SPECS_POLARLAYERS_H

#include "crgb.h"
#include "fl/function.h"
#include "MathUtils.h"
#include "NoiseUtils.h"
#include "engine/utils/Utils.h"

namespace LEDSegments {
    using PolarLayer = fl::function<CRGB(
        uint16_t angle,
        fract16 radius,
        unsigned long timeInMillis
    )>;

    inline CRGB colourNoiseLayer(
        uint16_t angle,
        fract16 radius,
        unsigned long timeInMillis
    ) {
        fl::u16 freqMultiplier = 600;
        fl::u8 speed = 50;

        auto [x, y] = cartesianCoords(
            angle,
            radius,
            freqMultiplier,
            4,
            false
        );

        return CHSV(
            nnoise8(x, y, timeInMillis * speed),
            255,
            255
        );
    }

    // Fractal Brownian Motion
    inline CRGB fBmLayer(
        uint16_t angle,
        fract16 radius,
        unsigned long timeInMillis
    ) {
        static fl::u16 freqMultiplier = 100;
        static fl::u8 speed = 50;
        static fl::u8 octaves = 4;

        auto time = timeInMillis * speed;
        auto [x, y] = cartesianCoords(
            angle,
            radius,
            freqMultiplier,
            4
        );

        uint16_t r = 0;
        uint16_t g = 0;
        uint16_t b = 0;
        uint16_t amplitude = U16_HALF;

        for (int o = 0; o < octaves; o++) {
            r += (inoise16(x, y, time) * amplitude) >> 16;
            // g += (inoise16(x + 1000, y + 1000, time) * amplitude) >> 16;
            // b += (inoise16(x + 2000, y + 2000, time) * amplitude) >> 16;
            x <<= 1;
            y <<= 1;
            amplitude >>= 1;
        }

        uint8_t nR = normaliseNoise(map16_to_8(r));
        // uint8_t nG = normaliseNoise(map16_to_8(g));
        // uint8_t nB = normaliseNoise(map16_to_8(b));

        // return {nR, nG, nB};
        return CHSV(
            nR,
            255,
            255
        );
    }

    inline CRGB turbulenceLayer(
        uint16_t angle,
        fract16 radius,
        unsigned long timeInMillis
    ) {
        static fl::u16 freqMultiplier = 100;
        static fl::u8 speed = 10;

        auto time = timeInMillis * speed;
        auto [x, y] = cartesianCoords(angle, radius, freqMultiplier);

        int16_t r = inoise16(x, y, time) - U16_HALF;
        // int16_t g = inoise16(x + 1000, y + 1000, time) - 32768;
        // int16_t b = inoise16(x + 2000, y + 2000, time) - 32768;

        uint8_t absR = normaliseNoise(map16_to_8(abs(r)));
        // uint8_t absG = normaliseNoise(map16_to_8(abs(g)));
        // uint8_t absB = normaliseNoise(map16_to_8(abs(b)));

        // return {absR, absG, absB};
        return CHSV(
            absR,
            255,
            255
        );
    }

    // Inverse turbulence
    inline CRGB ridgedLayer(
        uint16_t angle,
        fract16 radius,
        unsigned long timeInMillis
    ) {
        static fl::u16 freqMultiplier = 100;
        static fl::u8 speed = 10;

        auto time = timeInMillis * speed;
        auto [x, y] = cartesianCoords(angle, radius, freqMultiplier);

        int16_t r = inoise16(x, y, time) - U16_HALF;
        // int16_t g = inoise16(x + 1000, y + 1000, time) - 32768;
        // int16_t b = inoise16(x + 2000, y + 2000, time) - 32768;

        uint8_t absR = normaliseNoise(map16_to_8(255 - abs(r)));
        // uint8_t absG = normaliseNoise(map16_to_8(255 - abs(g)));
        // uint8_t absB = normaliseNoise(map16_to_8(255 - abs(b)));

        // return {absR, absG, absB};
        return CHSV(
            absR,
            255,
            255
        );
    }
}
#endif //LED_SEGMENTS_SPECS_POLARLAYERS_H
