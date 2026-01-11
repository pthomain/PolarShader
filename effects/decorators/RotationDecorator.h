#ifndef LED_SEGMENTS_EFFECTS_DECORATORS_ROTATIONDECORATOR_H
#define LED_SEGMENTS_EFFECTS_DECORATORS_ROTATIONDECORATOR_H

#include "PolarLayerDecorator.h"

namespace LEDSegments {
    /**
     * @class RotationDecorator
     * @brief A stateful Polar decorator that applies a global angular rotation to the entire frame.
     *
     * The rotation speed evolves over time using 1D noise, creating an organic,
     * wandering motion. It operates on the 16-bit angular domain, where wraparound
     * is the desired behavior.
     */
    class RotationDecorator : public PolarDecorator, public StatefulDecorator {
        uint16_t globalAngle = 0;
        int16_t angularVelocity = 0;
        uint8_t timeScale;

        static constexpr uint8_t TIME_SCALE_SHIFT = 5;

    public:
        explicit RotationDecorator(uint8_t timeScale = 1) : timeScale(timeScale) {
        }

        /**
         * @brief Evolves the global rotation angle for the next frame.
         * The angular velocity is determined by sampling 1D noise, and then
         * integrated into the current angle.
         */
        void advanceFrame(unsigned long timeInMillis) override {
            uint32_t t = (timeInMillis * timeScale) << TIME_SCALE_SHIFT;
            uint16_t noiseAngle = inoise16(t);

            // Map noise from [0, 65535] to a signed velocity in [-32768, 32767]
            auto targetAngularVelocity = (int16_t) (noiseAngle - 32768);

            const uint8_t angularSpeedShift = 3; //lower is faster
            targetAngularVelocity >>= angularSpeedShift;

            // Smooth velocity using an IIR filter to add inertia.
            angularVelocity = (angularVelocity + targetAngularVelocity) >> 1;

            globalAngle += angularVelocity;
        }

        PolarLayer operator()(const PolarLayer &layer) const override {
            // Capture 'this' to access the current globalAngle at render time.
            return [this, layer](uint16_t angle, fract16 radius, unsigned long timeInMillis) {
                // Apply the global rotation. The uint16_t arithmetic correctly
                // wraps around within the 0-65535 angular domain.
                uint16_t newAngle = angle + globalAngle;
                return layer(newAngle, radius, timeInMillis);
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_ROTATIONDECORATOR_H
