//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

/*
 * This file is part of PolarShader.
 *
 * PolarShader is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PolarShader is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PolarShader. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef POLAR_SHADER_SPECS_POLARPIPELINEBUILDER_H
#define POLAR_SHADER_SPECS_POLARPIPELINEBUILDER_H

#include "PolarPipeline.h"
#include "patterns/BasePattern.h"
#include "PipelineContext.h"
#include "FastLED.h"
#include "renderer/pipeline/signals/Accumulators.h"
#include <memory>
#include <type_traits>
#include <utility>

#include "signals/Signals.h"

namespace PolarShader {
    class PolarPipelineBuilder {
        enum class BuilderDomain {
            Cartesian,
            Polar
        };

        std::unique_ptr<BasePattern> pattern;
        CRGBPalette16 palette;
        fl::vector<PipelineStep> steps;
        bool built = false;
        BuilderDomain currentDomain = BuilderDomain::Cartesian;
        // Must point to a string with static storage duration (presets use literals).
        const char *name;
        std::shared_ptr<PipelineContext> context = std::make_shared<PipelineContext>();
        DepthSignal depthSignal = constantDepth(static_cast<uint32_t>(random16()) << CARTESIAN_FRAC_BITS);

        void ensureFinalPolarDomain() {
            if (currentDomain == BuilderDomain::Cartesian) {
                steps.push_back({PipelineStepKind::ToPolar});
                currentDomain = BuilderDomain::Polar;
            }
        }

    public:
        PolarPipelineBuilder(
            std::unique_ptr<CartesianPattern> pattern,
            const CRGBPalette16 &palette,
            const char *name
        ) : pattern(std::move(pattern)),
            palette(palette),
            name(name ? name : "unnamed") {
            currentDomain = BuilderDomain::Cartesian;
        }

        PolarPipelineBuilder(
            std::unique_ptr<PolarPattern> pattern,
            const CRGBPalette16 &palette,
            const char *name
        ) : pattern(std::move(pattern)),
            palette(palette),
            name(name ? name : "unnamed") {
            currentDomain = BuilderDomain::Polar;
        }

        PolarPipelineBuilder &setDepthSignal(DepthSignal signal) {
            if (built) return *this;
            if (!signal) {
                return *this;
            }
            depthSignal = std::move(signal);
            return *this;
        }

        PolarPipelineBuilder &setDepthSignal(SFracQ0_16Signal signal) {
            if (built) return *this;
            if (!signal) {
                return *this;
            }
            depthSignal = depth(std::move(signal));
            return *this;
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

#endif //POLAR_SHADER_SPECS_POLARPIPELINEBUILDER_H
