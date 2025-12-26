#ifndef LED_SEGMENTS_SPECS_POLARLAYERS_H
#define LED_SEGMENTS_SPECS_POLARLAYERS_H

#include "crgb.h"
#include "fl/function.h"
#include "MathUtils.h"
#include "engine/utils/Utils.h"

namespace LEDSegments {
    using PolarLayer = fl::function<CRGB(
        uint16_t angle,
        fract16 radius,
        int16_t o1,
        int16_t o2,
        int16_t o3,
        unsigned long timeInMillis
    )>;

    inline std::pair<uint32_t, uint32_t> scaleNoiseCoords(
        fl::u16 x,
        fl::u16 y,
        fl::u8 freqMultiplier
    ) {
        uint32_t ux = static_cast<uint32_t>(x) + 0x8000; // shift 0 → mid-range
        uint32_t uy = static_cast<uint32_t>(y) + 0x8000; // shift 0 → mid-range

        return {
            freqMultiplier * ux,
            freqMultiplier * uy
        };
    }

    inline std::pair<int16_t, int16_t> getCartesian(
        uint16_t angle,
        fract16 radius
    ) {
        return {
            scale_i16_by_f16(cos16(angle), radius),
            scale_i16_by_f16(sin16(angle), radius)
        };
    }

    inline CRGB simpleColourNoise(
        uint16_t angle,
        fract16 radius,
        int16_t o1,
        int16_t o2,
        int16_t o3,
        unsigned long timeInMillis
    ) {
        auto [x, y] = getCartesian(angle, radius);

        fl::u8 freqMultiplier = 100;
        fl::u8 speed = 50;

        auto [scaledX, scaledY] = scaleNoiseCoords(x, y, freqMultiplier);

        uint16_t noiseR = inoise16(scaledX, scaledY, timeInMillis * speed);

        return CHSV(
            normaliseNoise(map16_to_8(noiseR)),
            255,
            255
        );
    }


    //
    // inline fl::pair<int16_t, int16_t> getKaleidoscopeCoords(
    //     uint16_t angle
    //     fract16 radius,
    //     uint16_t zoom,
    //     uint8_t nbSegments = 1 // Number of kaleidoscope segments
    // ) {
    //     // Calculate the angle for one segment
    //     uint16_t segmentAngle = UINT16_MAX / nbSegments;
    //
    //     // Fold the angle into a single segment
    //     uint16_t foldedAngle = angle % segmentAngle;
    //     if ((angle / segmentAngle) % 2 == 1) {
    //         foldedAngle = segmentAngle - foldedAngle;
    //     }
    //
    //     int16_t x = getX(foldedAngle, radius, zoom);
    //     int16_t y = getY(foldedAngle, radius, zoom);
    //
    //     return {x, y};
    // }

    // inline CRGB kaleidoscopeColourNoise(
    //     uint16_t angle,
    //     fract16 radius,
    //     int16_t o1,
    //     int16_t o2,
    //     int16_t o3,
    //     unsigned long timeInMillis,
    //     uint8_t nbSegments = 4
    // ) {
    //     // This parameter controls the scale of the noise pattern.
    //     // Higher values produce smaller, denser blobs (a more "zoomed out" view of the noise field).
    //     // Lower values produce larger, smoother blobs (a more "zoomed in" view).
    //     uint16_t zoom = 32767;
    //
    //     auto coords = getKaleidoscopeCoords(angle, radius, zoom, nbSegments);
    //     int16_t x = coords.first;
    //     int16_t y = coords.second;
    //
    //     uint16_t noise = inoise16(x + 32768, y + 32768, timeInMillis << 3);
    //     return CHSV(
    //         map16_to_8(noise),
    //         255,
    //         255
    //     );
    // }

    // inline CRGB lavaLampLayer(
    //     uint16_t angle,
    //     fract16 radius,
    //     int16_t o1,
    //     int16_t o2,
    //     int16_t o3,
    //     unsigned long timeInMillis
    // ) {
    //     // This parameter controls the scale of the noise pattern.
    //     // Higher values produce smaller, denser blobs (a more "zoomed out" view of the noise field).
    //     // Lower values produce larger, smoother blobs (a more "zoomed in" view).
    //     uint16_t zoom = 8192;
    //     int16_t x = getX(angle, radius, zoom);
    //     int16_t y = getY(angle, radius, zoom);
    //
    //     uint32_t t = timeInMillis << 2;
    //
    //     // Generate three different noise values for R, G, B
    //     uint16_t noiseR = inoise16(x + 32768, y + 32768, t);
    //     uint16_t noiseG = inoise16(x + 32768 + 20000, y + 32768 + 20000, t + 20000);
    //     // Offset coords for different patterns
    //     uint16_t noiseB = inoise16(x + 32768 - 20000, y + 32768 - 20000, t + 40000);
    //
    //     // Map noise to color channels
    //     uint8_t r = map16_to_8(noiseR);
    //     uint8_t g = map16_to_8(noiseG);
    //     uint8_t b = map16_to_8(noiseB);
    //
    //     // Scale down to reduce brightness
    //     return CRGB(r / 2, g / 2, b / 2);
    // }
}
#endif //LED_SEGMENTS_SPECS_POLARLAYERS_H
