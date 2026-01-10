#ifndef LED_SEGMENTS_SPECS_POLARLAYERS_H
#define LED_SEGMENTS_SPECS_POLARLAYERS_H

#include "crgb.h"
#include "MathUtils.h"
#include "NoiseUtils.h"
#include "engine/utils/Utils.h"

namespace LEDSegments {
    // This offset is used to map the signed 32-bit Cartesian coordinate space
    // to the unsigned 32-bit domain required by the inoise16 function.
    static constexpr uint32_t NOISE_DOMAIN_OFFSET = 0x800000;

    /**
     * @brief A base noise layer that samples 2D Perlin noise.
     * @param x The signed 32-bit x-coordinate.
     * @param y The signed 32-bit y-coordinate.
     * @param timeInMillis The current animation time.
     * @return A 16-bit scalar value (0-255) to be used as a palette index.
     *
     * This function performs the final, critical conversion from the pipeline's
     * standard signed Cartesian space to the unsigned domain required by inoise16.
     */
    inline uint16_t colourNoiseLayer(
        uint32_t x,
        uint32_t y,
        unsigned long timeInMillis
    ) {
        uint8_t speed = 10;
        uint32_t t = timeInMillis * speed;
        return nnoise8(x, y, t);
    }

    /**
     * @brief A noise layer that generates Fractal Brownian Motion (fBm).
     * This creates a more detailed, multi-layered noise pattern by summing
     * multiple "octaves" of noise at different frequencies and amplitudes.
     * @note The coordinate doubling (`x <<= 1`) can lead to rapid growth. The
     * number of octaves is intentionally limited to prevent overflow of the
     * 32-bit coordinate space, especially after upstream scaling decorators.
     */
    inline uint16_t fBmLayer(
        uint32_t x,
        uint32_t y,
        unsigned long timeInMillis
    ) {
        static fl::u8 octaves = 4;
        uint8_t speed = 30;
        uint32_t t = timeInMillis * speed;

        uint16_t r = 0;
        uint16_t amplitude = U16_HALF;

        for (int o = 0; o < octaves; o++) {
            r += (inoise16(x, y, t) * amplitude) >> 16;
            x <<= 1;
            y <<= 1;
            amplitude >>= 1;
        }

        return normaliseNoise(map16_to_8(r));
    }

    /**
     * @brief A noise layer that generates a turbulence pattern.
     * It uses the absolute value of signed noise to create sharp creases,
     * resulting in a "flame-like" or "marbled" appearance.
     */
    inline uint16_t turbulenceLayer(
        uint32_t x,
        uint32_t y,
        unsigned long timeInMillis
    ) {
        uint8_t speed = 10;
        uint32_t t = timeInMillis * speed;
        int16_t r = inoise16(x, y, t) - U16_HALF;
        return normaliseNoise(map16_to_8(abs(r)));
    }

    /**
     * @brief A noise layer that generates a ridged multifractal pattern.
     * This is effectively the inverse of turbulence, creating bright ridges
     * on a dark background, suitable for lightning or vein-like patterns.
     */
    inline uint16_t ridgedLayer(
        uint32_t x,
        uint32_t y,
        unsigned long timeInMillis
    ) {
        uint8_t speed = 10;
        uint32_t t = timeInMillis * speed;
        int16_t r = inoise16(x, y, t) - U16_HALF;
        return normaliseNoise(map16_to_8(255 - abs(r)));
    }
}
#endif //LED_SEGMENTS_SPECS_POLARLAYERS_H
