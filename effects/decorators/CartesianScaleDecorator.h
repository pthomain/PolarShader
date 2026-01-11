#ifndef LED_SEGMENTS_EFFECTS_DECORATORS_CARTESIANSCALEDECORATOR_H
#define LED_SEGMENTS_EFFECTS_DECORATORS_CARTESIANSCALEDECORATOR_H

#include "PolarLayerDecorator.h"

namespace LEDSegments {
    /**
     * @class CartesianScaleDecorator
     * @brief A stateless Cartesian decorator that scales the input coordinates.
     *
     * This transformation effectively changes the "zoom" level of the noise
     * texture. Higher scale values result in a more zoomed-out, detailed
     * pattern, while lower values create a zoomed-in, smoother appearance.
     * This operation happens in the signed 32-bit Cartesian space.
     */
    class CartesianScaleDecorator : public CartesianDecorator, public StatelessDecorator {
        uint16_t scale;
        // This shift prevents overflow when multiplying coordinates by the scale factor.
        // It's a fixed-point scaling operation, effectively dividing the result by 256.
        static constexpr uint8_t DOMAIN_SCALE_SHIFT = 8;

    public:
        explicit CartesianScaleDecorator(uint16_t scale) : scale(scale) {
        }

        CartesianLayer operator()(CartesianLayer layer) const override {
            return [this, layer](int32_t x, int32_t y, unsigned long timeInMillis) {
                // Scale the coordinates before passing them to the next layer.
                // The multiplication is promoted to 64-bit to prevent overflow before
                // being shifted back down, ensuring safety even with large coordinates.
                return layer(
                    (int32_t) (((int64_t) x * scale) >> DOMAIN_SCALE_SHIFT),
                    (int32_t) (((int64_t) y * scale) >> DOMAIN_SCALE_SHIFT),
                    timeInMillis
                );
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_CARTESIANSCALEDECORATOR_H
