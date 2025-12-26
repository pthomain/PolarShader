#ifndef LED_SEGMENTS_SPECS_NOISEUTILS_H
#define LED_SEGMENTS_SPECS_NOISEUTILS_H

#include "FastLED.h"
#include "engine/utils/Utils.h"

namespace LEDSegments {
    // Controls base noise wavelength.
    // Larger = smoother, smaller = noisier.
    constexpr uint8_t NOISE_SCALE_SHIFT = 7;

    // Normalised 8 bit noise
    inline uint8_t nnoise8(uint32_t x, uint32_t y, uint32_t z) {
        return normaliseNoise(map16_to_8(inoise16(x, y, z)));
    }

    inline uint16_t foldAngleKaleidoscope(
        fl::u16 angle,
        fl::u8 nbSegments,
        bool isMirroring = true
    ) {
        if (nbSegments <= 1) return angle;

        // Exact angular partitioning
        uint16_t segmentAngle = 0x10000 / nbSegments;
        uint16_t foldedAngle = angle % segmentAngle;

        if (isMirroring) {
            // Mirror every other segment
            if ((angle / segmentAngle) & 1) {
                return segmentAngle - foldedAngle;
            }
        }

        return foldedAngle;
    }

    inline fl::pair<uint32_t, uint32_t> cartesianCoords(
        fl::u16 angle,
        fract16 radius,
        fl::u16 freqMultiplier,
        fl::u8 kaleidoscopeSegments = 5,
        bool isMirroring = false
    ) {
        // Fold angle for exact repetition
        uint16_t foldedAngle = (kaleidoscopeSegments > 1)
                                   ? foldAngleKaleidoscope(angle, kaleidoscopeSegments, isMirroring)
                                   : angle;

        // Base x,y in noise space
        int32_t x = scale_i16_by_f16(cos16(foldedAngle), radius);
        int32_t y = scale_i16_by_f16(sin16(foldedAngle), radius);

        if (kaleidoscopeSegments > 1) {
            // angular dominance
            x += cos16(foldedAngle) >> 2;
            y += sin16(foldedAngle) >> 2;
        }

        // Reduce to noise resolution
        x >>= NOISE_SCALE_SHIFT;
        y >>= NOISE_SCALE_SHIFT;

        // Convert to unsigned and scale frequency
        uint32_t ux = ((uint32_t) x + 0x800000) * freqMultiplier;
        uint32_t uy = ((uint32_t) y + 0x800000) * freqMultiplier;

        return {ux, uy};
    }
}
#endif //LED_SEGMENTS_SPECS_NOISEUTILS_H
