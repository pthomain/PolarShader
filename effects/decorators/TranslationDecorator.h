#ifndef LED_SEGMENTS_EFFECTS_DECORATORS_TRANSLATIONDECORATOR_H
#define LED_SEGMENTS_EFFECTS_DECORATORS_TRANSLATIONDECORATOR_H

#include "PolarLayerDecorator.h"

namespace LEDSegments {
    /**
     * @class TranslationDecorator
     * @brief A stateful Cartesian decorator that applies a global translation to the coordinates.
     *
     * This creates a "camera movement" effect over the noise field. The movement
     * itself is guided by a 2D noise flow field, resulting in an organic,
     * wandering motion. It operates on the signed 32-bit Cartesian space.
     */
    class TranslationDecorator : public CartesianDecorator, public StatefulDecorator {
        int32_t prevVx = 0;
        int32_t prevVy = 0;

        // Global positions use uint32_t to rely on defined unsigned overflow
        // behavior, creating an infinitely wrapping toroidal space for the noise.
        uint32_t globalPositionX = random16();
        uint32_t globalPositionY = random16();

        uint8_t timeScale;

        static constexpr uint8_t TIME_SCALE_SHIFT = 5;
        static constexpr uint8_t SPEED_SHIFT = 4; // controls speed, lower is faster

    public:
        explicit TranslationDecorator(uint8_t timeScale = 1) : timeScale(timeScale) {
        }

        /**
         * @brief Evolves the global translation position for the next frame.
         * A velocity vector is calculated from a 2D noise field (a flow field)
         * and integrated into the current position with smoothing.
         */
        void advanceFrame(unsigned long timeInMillis) override {
            uint32_t t = (timeInMillis * timeScale) << TIME_SCALE_SHIFT;

            // Sample the flow field to get a direction vector.
            uint16_t directionAngle = inoise16(globalPositionX, globalPositionY, t);
            int32_t vx = cos16(directionAngle);
            int32_t vy = sin16(directionAngle);

            // Smooth velocity using an IIR filter to add inertia.
            vx = (prevVx + vx) >> 1;
            vy = (prevVy + vy) >> 1;

            // Integrate velocity into position.
            globalPositionX += (vx >> SPEED_SHIFT);
            globalPositionY += (vy >> SPEED_SHIFT);

            prevVx = vx;
            prevVy = vy;
        }

        CartesianLayer operator()(CartesianLayer layer) const override {
            // Capture 'this' to access the current state (globalPositionX/Y) at render time.
            return [this, layer](int32_t x, int32_t y, unsigned long timeInMillis) {
                // Apply the translation. The cast from unsigned global position to
                // signed is intentional; it preserves the bit pattern, allowing
                // the wrapping behavior of the unsigned position to correctly
                // translate into the signed coordinate space.
                return layer(x + (int32_t) globalPositionX, y + (int32_t) globalPositionY, timeInMillis);
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_TRANSLATIONDECORATOR_H
