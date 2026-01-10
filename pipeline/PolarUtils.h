#ifndef LED_SEGMENTS_SPECS_POLARUTILS_H
#define LED_SEGMENTS_SPECS_POLARUTILS_H

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

    /**
 * @brief Converts polar coordinates to a clean, signed Cartesian space.
 * @param angle The polar angle (uint16_t from 0-65535).
 * @param radius The polar radius (fract16 from 0-65535, representing 0.0 to ~1.0).
 * @return A pair of {x, y} coordinates in a signed 32-bit integer space.
 *
 * This function establishes the base coordinate system. All scaling and resolution
 * control is now handled by decorators like DomainScaleDecorator.
 */
    static fl::pair<uint32_t, uint32_t> cartesianCoords(
        fl::u16 angle,
        fract16 radius
    ) {
        // Convert polar to signed cartesian space at full resolution + shift to unsigned
        int32_t x = scale_i16_by_f16(cos16(angle), radius);
        int32_t y = scale_i16_by_f16(sin16(angle), radius);
        uint32_t ux = (uint32_t) x + NOISE_DOMAIN_OFFSET;
        uint32_t uy = (uint32_t) y + NOISE_DOMAIN_OFFSET;
        return {ux, uy};
    }
}
#endif //LED_SEGMENTS_SPECS_POLARUTILS_H
