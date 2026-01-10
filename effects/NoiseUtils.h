#ifndef LED_SEGMENTS_SPECS_NOISEUTILS_H
#define LED_SEGMENTS_SPECS_NOISEUTILS_H

#include "engine/utils/Utils.h"

namespace LEDSegments {
    // Normalised 8 bit noise.
    // Maps the 16-bit output of inoise16 to a full 0-255 range.
    inline uint8_t nnoise8(uint32_t x, uint32_t y, uint32_t z) {
        return normaliseNoise(map16_to_8(inoise16(x, y, z)));
    }
}
#endif //LED_SEGMENTS_SPECS_NOISEUTILS_H
