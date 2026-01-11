#ifndef LED_SEGMENTS_EFFECTS_DECORATORS_POLARPIPELINE_H
#define LED_SEGMENTS_EFFECTS_DECORATORS_POLARPIPELINE_H

#include "decorators/PolarLayerDecorator.h"
#include "PolarLayers.h"
#include <type_traits>

namespace LEDSegments {
    /**
     * @class PolarPipeline
     * @brief Constructs and manages a multi-stage rendering pipeline for polar effects.
     *
     * The pipeline defines a strict order of operations to transform polar pixel
     * coordinates into a final color. This architecture allows complex visual effects
     * to be built by composing modular decorators, each with a single responsibility.
     *
     * @section data_flow Data Flow and Execution Order
     * The pipeline processes coordinates in a "reverse" flow, from the pixel back
     * to the noise source. This is because each decorator wraps the previous one,
     * modifying the input for the next stage in the chain.
     *
     * The logical order of operations is as follows:
     * 1.  **Polar Decoration**: A pixel's (angle, radius) is first transformed by
     *     Polar decorators (e.g., Rotation, Vortex).
     * 2.  **Polar-to-Cartesian Conversion**: The resulting (angle, radius) is
     *     converted into a standard, signed 32-bit Cartesian space (x, y).
     * 3.  **Cartesian Decoration**: The (x, y) coordinates are then transformed by
     *     Cartesian decorators (e.g., Translation, Scaling).
     * 4.  **Noise Sampling**: The final (x, y) coordinates are passed to a noise
     *     layer (e.g., `colourNoiseLayer`), which returns a scalar value (palette index).
     * 5.  **Palette Mapping**: This final index is mapped to a CRGB color.
     *
     * @section decorator_order Decorator Application Order
     * To achieve the logical flow above, decorators are composed like nested
     * functions: `DecoratorA(DecoratorB(Source))`. This means the *last* decorator
     * added to the pipeline is the *first* one to execute. The `build()` method
     * handles this by iterating through the decorator lists in reverse.
     */
    class PolarPipeline {
        fl::vector<StatefulDecorator *> statefulDecorators;
        fl::vector<CartesianDecorator *> cartesianDecorators;
        fl::vector<PolarDecorator *> polarDecorators;

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

    public:
        /**
         * @brief Adds a Cartesian decorator to the pipeline.
         */
        template<typename T, typename = std::enable_if_t<std::is_base_of<CartesianDecorator, T>::value> >
        void addCartesianDecorator(T *decorator) {
            if constexpr (std::is_base_of_v<StatefulDecorator, T>) {
                statefulDecorators.push_back(decorator);
            }
            cartesianDecorators.push_back(decorator);
        }

        /**
         * @brief Adds a Polar decorator to the pipeline.
         */
        template<typename T, typename = std::enable_if_t<std::is_base_of<PolarDecorator, T>::value> >
        void addPolarDecorator(T *decorator) {
            if constexpr (std::is_base_of_v<StatefulDecorator, T>) {
                statefulDecorators.push_back(decorator);
            }
            polarDecorators.push_back(decorator);
        }

        void clear() {
            statefulDecorators.clear();
            cartesianDecorators.clear();
            polarDecorators.clear();
        }

        /**
         * @brief Advances the state of all stateful decorators.
         *
         * This method iterates through the pre-filtered list of stateful decorators,
         * avoiding any runtime type checking and removing the need for RTTI.
         * The order of advancement is the order of registration.
         */
        void advanceFrame(unsigned long timeInMillis) {
            for (auto *decorator: statefulDecorators) {
                decorator->advanceFrame(timeInMillis);
            }
        }

        /**
         * @brief Builds the final, renderable ColorLayer.
         */
        ColourLayer build(const CartesianLayer &sourceLayer, const CRGBPalette16 &palette) {
            CartesianLayer currentCartesian = sourceLayer;
            for (size_t i = 0; i < cartesianDecorators.size(); ++i) {
                currentCartesian = (*cartesianDecorators[cartesianDecorators.size() - 1 - i])(currentCartesian);
            }

            PolarLayer currentPolar = [layer = currentCartesian](uint16_t angle, fract16 radius,
                                                                 unsigned long timeInMillis) {
                auto [x, y] = cartesianCoords(angle, radius);
                return layer(x, y, timeInMillis);
            };

            for (size_t i = 0; i < polarDecorators.size(); ++i) {
                currentPolar = (*polarDecorators[polarDecorators.size() - 1 - i])(currentPolar);
            }

            return [palette, layer = currentPolar](uint16_t angle, fract16 radius, unsigned long timeInMillis) {
                uint16_t index = layer(angle, radius, timeInMillis);
                return ColorFromPalette(palette, index, 255, LINEARBLEND);
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_POLARPIPELINE_H
