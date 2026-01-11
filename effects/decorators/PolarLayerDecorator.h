#ifndef LED_SEGMENTS_EFFECTS_DECORATORS_POLARLAYERDECORATOR_H
#define LED_SEGMENTS_EFFECTS_DECORATORS_POLARLAYERDECORATOR_H

namespace LEDSegments {
    // A PolarLayer is a function that takes polar coordinates and returns a scalar value (palette index).
    using PolarLayer = fl::function<uint16_t(
        uint16_t angle,
        fract16 radius,
        unsigned long timeInMillis
    )>;

    // A CartesianLayer is a function that takes Cartesian coordinates and returns a scalar value.
    using CartesianLayer = fl::function<uint16_t(
        uint32_t x,
        uint32_t y,
        unsigned long timeInMillis
    )>;

    // A ColorLayer is the final output of the pipeline, a function that returns a CRGB color.
    using ColourLayer = fl::function<CRGB(
        uint16_t angle,
        fract16 radius,
        unsigned long timeInMillis
    )>;

    /**
     * @class FrameDecorator
     * @brief The abstract base for all decorators.
     */
    class FrameDecorator {
    public:
        virtual ~FrameDecorator() = default;
    };

    /**
     * @class StatefulDecorator
     * @brief A decorator that maintains state which evolves over time.
     */
    class StatefulDecorator : public virtual FrameDecorator {
    public:
        /**
         * @brief Evolves the decorator's internal state for the next frame.
         * @param timeInMillis The current animation time.
         * This is a pure virtual function, forcing all derived stateful decorators
         * to implement their state transition logic.
         */
        virtual void advanceFrame(unsigned long timeInMillis) = 0;
    };

    /**
     * @class StatelessDecorator
     * @brief A decorator that has no time-varying state.
     * Its transformation logic is based solely on its configuration parameters.
     */
    class StatelessDecorator : public virtual FrameDecorator {
    };


    /**
     * @class CartesianDecorator
     * @brief An interface for decorators that operate in the Cartesian (X, Y) domain.
     */
    class CartesianDecorator : public virtual FrameDecorator {
    public:
        virtual CartesianLayer operator()(CartesianLayer layer) const = 0;
    };

    /**
     * @class PolarDecorator
     * @brief An interface for decorators that operate in the Polar (Angle, Radius) domain.
     */
    class PolarDecorator : public virtual FrameDecorator {
    public:
        virtual PolarLayer operator()(const PolarLayer &layer) const = 0;
    };
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_POLARLAYERDECORATOR_H
