#ifndef LED_SEGMENTS_SPECS_POLARUTILS_H
#define LED_SEGMENTS_SPECS_POLARUTILS_H

#include "FastLED.h"
#include "engine/utils/Utils.h"
#include "MathUtils.h"

namespace LEDSegments {
    class PolarUtils {
        const uint16_t UINT16_ZERO = UINT16_MAX / 2;

    public:
        //Animation
        uint16_t time16 = 0;
        uint16_t rotate16 = 0;

        // Noise frequency control (input scaling)
        fract16 freq_theta = 0x1000; //controls angular “wavelength”
        fract16 freq_radius = 0x1000; //controls radial “wavelength”

        // Noise offset / translation
        uint16_t offset_theta = 0; //rotates the noise field
        fract16 offset_radius = 0; //pushes the noise outward/inward

        // Contrast and radial scaling
        uint16_t contrast_low = 0;
        uint16_t contrast_high = 65535;
        fract16 radial_scale = 0xFFFF; // 1.0

        uint16_t sampleAngularNoise(
            uint16_t theta,
            fract16 radius,
            uint16_t time,
            fract16 freqT,
            fract16 freqR,
            uint16_t offTheta,
            fract16 offRadius
        ) {
            // Warp inputs
            uint16_t theta_warped = theta + rotate16 + offTheta;
            uint16_t r_warped = scale16u(radius, freqR) + offRadius;

            // Map theta to circle for seamless wrapping
            int16_t x = cos16(theta_warped); // -32768..32767
            int16_t y = sin16(theta_warped);

            // Convert to 0..65535 unsigned
            uint16_t ux = x + UINT16_ZERO;
            uint16_t uy = y + UINT16_ZERO;

            ux = scale16u(ux, freqT);
            uy = scale16u(uy, freqT);

            // 3D noise params: x, y, radius + time
            uint32_t nx = static_cast<uint32_t>(ux) << 8;
            uint32_t ny = static_cast<uint32_t>(uy) << 8;
            uint32_t nz = (static_cast<uint32_t>(r_warped) << 8) + (static_cast<uint32_t>(time) << 5);

            //Noise
            uint16_t n = inoise16(nx, ny, nz); // 0..65535

            // Apply contrast (unsigned)
            if (n < contrast_low) n = 0;
            else n = n - contrast_low;
            n = scale16u(n, contrast_high);

            // Radial falloff (vignetting)
            n = scale16u(n, scale16u(radius, radial_scale));

            return n;
        }

        void fillPolar(
            CRGB *segmentArray,
            uint16_t pixelIndex,
            uint16_t angle,
            fract16 radius,
            unsigned long timeInMillis
        ) {
            // Layer 1: base noise
            uint16_t n1 = sampleAngularNoise(
                angle,
                radius,
                time16,
                freq_theta,
                freq_radius,
                offset_theta,
                offset_radius
            );

            // Layer 2: higher-frequency noise (octave)
            uint16_t n2 = sampleAngularNoise(
                angle * 2,
                radius,
                time16 + 5000,
                freq_theta * 2,
                freq_radius,
                offset_theta,
                offset_radius
            );

            // Combine layers evenly
            uint16_t n_final = scale16u(n1, UINT16_ZERO) + scale16u(n2, UINT16_ZERO); // 50/50 blend

            // Map to HSV
            segmentArray[pixelIndex] = CHSV(
                normaliseNoise(n_final >> 8),
                255,
                255
            );
        }
    };
}
#endif //LED_SEGMENTS_SPECS_POLARUTILS_H
