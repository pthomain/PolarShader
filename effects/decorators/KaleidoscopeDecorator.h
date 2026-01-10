#ifndef LED_SEGMENTS_EFFECTS_DECORATORS_KALEIDOSCOPEDECORATOR_H
#define LED_SEGMENTS_EFFECTS_DECORATORS_KALEIDOSCOPEDECORATOR_H

#include "PolarLayerDecorator.h"

namespace LEDSegments {
    /**
     * @class KaleidoscopeDecorator
     * @brief A stateless Polar decorator that folds the angular space to create symmetry.
     *
     * This decorator manipulates the angle coordinate to create symmetrical patterns.
     * It supports two distinct modes:
     * - **Kaleidoscope**: Folds the space into mirrored segments. This creates perfect,
     *   sharp symmetry lines, like a traditional kaleidoscope.
     * - **Mandala**: Multiplies the angle, causing the underlying pattern to repeat
     *   rotationally. This creates a continuous, flowing symmetry without hard edges.
     */
    class KaleidoscopeDecorator : public PolarDecorator, public StatelessDecorator {
        uint8_t segments;
        bool isMandala;
        bool isMirroring;

        static constexpr uint8_t MAX_SEGMENTS = 8;

        /**
         * @brief Folds an angle into a smaller sector for kaleidoscope symmetry.
         * @note The division `0x10000 / nbSegments` uses integer arithmetic, which
         * may introduce minor quantization errors for segment counts that are not
         * powers of two. This is generally visually acceptable.
         */
        static uint16_t foldAngleKaleidoscope(
            fl::u16 angle,
            fl::u8 nbSegments,
            bool isMirroring
        ) {
            if (nbSegments <= 1) return angle;

            uint16_t segmentAngle = 0x10000 / nbSegments;
            uint16_t foldedAngle = angle % segmentAngle;

            bool isOdd = (angle / segmentAngle) & 1;
            if (isMirroring && isOdd) return segmentAngle - foldedAngle;

            return foldedAngle;
        }

    public:
        explicit KaleidoscopeDecorator(
            uint8_t segments,
            bool isMandala = false,
            bool isMirroring = true
        ) : segments(min(segments, MAX_SEGMENTS)),
            isMandala(isMandala),
            isMirroring(isMirroring) {
        }

        PolarLayer operator()(const PolarLayer &layer) const override {
            return [this, layer](uint16_t angle, fract16 radius, unsigned long timeInMillis) {
                uint16_t newAngle = isMandala
                                        ? angle * segments
                                        : foldAngleKaleidoscope(angle, segments, isMirroring);

                return layer(newAngle, radius, timeInMillis);
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_KALEIDOSCOPEDECORATOR_H
