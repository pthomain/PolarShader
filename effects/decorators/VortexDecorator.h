#ifndef LED_SEGMENTS_EFFECTS_DECORATORS_VORTEXDECORATOR_H
#define LED_SEGMENTS_EFFECTS_DECORATORS_VORTEXDECORATOR_H

#include "PolarLayerDecorator.h"
#include "polar/effects/MathUtils.h"

namespace LEDSegments {
    /**
     * @class VortexDecorator
     * @brief A stateless Polar decorator that applies a radius-dependent angular offset (twist).
     *
     * This creates a spiral or vortex effect by rotating pixels further from the
     * center more than pixels closer to the center. It operates on the polar
     * coordinate space, modifying the angle based on the radius.
     * The radius is expected to be a fract16 in the range [0, 1].
     */
    class VortexDecorator : public PolarDecorator, public StatelessDecorator {
        // Controls how much the angle changes per unit of radius.
        // Higher values create tighter spirals.
        fract16 anglePerRadiusQ16;

    public:
        explicit VortexDecorator(fract16 anglePerRadiusQ16) : anglePerRadiusQ16(anglePerRadiusQ16) {
        }

        PolarLayer operator()(const PolarLayer &layer) const override {
            return [this, layer](uint16_t angle, fract16 radius, unsigned long timeInMillis) {
                // Calculate the angular offset. scale_u16_by_f16 is a fixed-point
                // multiplication of a 16-bit value by a 16-bit fraction.
                uint16_t offset = scale_u16_by_f16(anglePerRadiusQ16, radius);
                uint16_t newAngle = angle + offset;
                return layer(newAngle, radius, timeInMillis);
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_VORTEXDECORATOR_H
