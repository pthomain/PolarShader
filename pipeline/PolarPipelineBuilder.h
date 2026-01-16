//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2023 Pierre Thomain

/*
 * This file is part of LED Segments.
 *
 * LED Segments is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LED Segments is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LED Segments. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LED_SEGMENTS_SPECS_POLARPIPELINEBUILDER_H
#define LED_SEGMENTS_SPECS_POLARPIPELINEBUILDER_H

#include "PolarPipeline.h"
#include <memory>
#include <type_traits>
#include <utility>

namespace LEDSegments {
    class PolarPipelineBuilder {
        enum class BuilderDomain {
            Cartesian,
            Polar
        };

        NoiseLayer sourceLayer;
        CRGBPalette16 palette;
        fl::vector<PipelineStep> steps;
        bool built = false;
        BuilderDomain currentDomain = BuilderDomain::Cartesian;
        // Must point to a string with static storage duration (presets use literals).
        const char *name;

        void ensureFinalPolarDomain() {
            if (currentDomain == BuilderDomain::Cartesian) {
                steps.push_back({PipelineStepKind::ToPolar});
                currentDomain = BuilderDomain::Polar;
            }
        }

    public:
        PolarPipelineBuilder(
            NoiseLayer sourceLayer,
            const CRGBPalette16 &palette,
            const char *name
        ) : sourceLayer(std::move(sourceLayer)),
            palette(palette),
            name(name ? name : "unnamed") {
        }

        template<typename T, typename = std::enable_if_t<std::is_base_of<PolarTransform, T>::value> >
        PolarPipelineBuilder &addPolarTransform(T transform) {
            if (built) return *this;
            if (currentDomain == BuilderDomain::Cartesian) {
                steps.push_back(PipelineStep::toPolar());
                currentDomain = BuilderDomain::Polar;
            }
            steps.push_back(PipelineStep::polar(std::make_unique<T>(std::move(transform))));
            return *this;
        }

        template<typename T, typename = std::enable_if_t<std::is_base_of<CartesianTransform, T>::value> >
        PolarPipelineBuilder &addCartesianTransform(T transform) {
            if (built) return *this;
            if (currentDomain == BuilderDomain::Polar) {
                steps.push_back(PipelineStep::toCartesian());
                currentDomain = BuilderDomain::Cartesian;
            }
            steps.push_back(PipelineStep::cartesian(std::make_unique<T>(std::move(transform))));
            return *this;
        }

        PolarPipeline build();
    };
}

#endif //LED_SEGMENTS_SPECS_POLARPIPELINEBUILDER_H
