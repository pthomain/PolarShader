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

}
#endif //LED_SEGMENTS_SPECS_POLARUTILS_H
