#ifndef LED_SEGMENTS_SPECS_POLARLAYERS_H
#define LED_SEGMENTS_SPECS_POLARLAYERS_H

#include <engine/palette/Palette.h>

#include "crgb.h"
#include "fl/function.h"
#include "MathUtils.h"
#include "NoiseUtils.h"
#include "engine/utils/Utils.h"

namespace LEDSegments {
    using PolarLayer = fl::function<CRGB(
        uint16_t angle,
        fract16 radius,
        unsigned long timeInMillis,
        int32_t globalPositionX,
        int32_t globalPositionY
    )>;

    inline CRGB colourNoiseLayer(
        uint16_t angle,
        fract16 radius,
        unsigned long timeInMillis,
        int32_t globalPositionX,
        int32_t globalPositionY
    ) {
        fl::u16 freqMultiplier = 200;
        fl::u8 speed = 10;

        auto [x, y] = cartesianCoords(
            angle,
            radius,
            freqMultiplier,
            globalPositionX,
            globalPositionY,
            1,
            false,
            false
        );

        uint8_t r = nnoise8(x, y, timeInMillis * speed);

        return ColorFromPalette(CloudColors_p, r, 255, LINEARBLEND);

        // return CHSV(r, 255, 255);
    }

    // Fractal Brownian Motion
    inline CRGB fBmLayer(
        uint16_t angle,
        fract16 radius,
        unsigned long timeInMillis,
        int32_t globalPositionX,
        int32_t globalPositionY
    ) {
        static fl::u16 freqMultiplier = 800;
        static fl::u8 speed = 30;
        static fl::u8 octaves = 4;

        auto time = timeInMillis * speed;
        auto [x, y] = cartesianCoords(
            angle,
            radius,
            freqMultiplier,
            globalPositionX,
            globalPositionY
        );

        uint16_t r = 0;
        uint16_t g = 0;
        uint16_t b = 0;
        uint16_t amplitude = U16_HALF;

        for (int o = 0; o < octaves; o++) {
            r += (inoise16(x, y, time) * amplitude) >> 16;
            x <<= 1;
            y <<= 1;
            amplitude >>= 1;
        }

        uint8_t nR = normaliseNoise(map16_to_8(r));

        return CHSV(
            nR,
            255,
            255
        );
    }

    inline CRGB turbulenceLayer(
        uint16_t angle,
        fract16 radius,
        unsigned long timeInMillis,
        int32_t globalPositionX,
        int32_t globalPositionY
    ) {
        static fl::u16 freqMultiplier = 1600;
        static fl::u8 speed = 10;

        auto time = timeInMillis * speed;
        auto [x, y] = cartesianCoords(
            angle,
            radius,
            freqMultiplier,
            globalPositionX,
            globalPositionY
        );

        int16_t r = inoise16(x, y, time) - U16_HALF;
        uint8_t absR = normaliseNoise(map16_to_8(abs(r)));

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
        unsigned long timeInMillis,
        int32_t globalPositionX,
        int32_t globalPositionY
    ) {
        static fl::u16 freqMultiplier = 1600;
        static fl::u8 speed = 10;

        auto time = timeInMillis * speed;
        auto [x, y] = cartesianCoords(
            angle,
            radius,
            freqMultiplier,
            globalPositionX,
            globalPositionY
        );

        int16_t r = inoise16(x, y, time) - U16_HALF;
        uint8_t absR = normaliseNoise(map16_to_8(255 - abs(r)));

        return CHSV(
            absR,
            255,
            255
        );
    }
}
#endif //LED_SEGMENTS_SPECS_POLARLAYERS_H
