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

#include <memory>
#include <type_traits>
#include <utility>
#include "FastLED.h"
#include "PipelineContext.h"
#include "PolarPipeline.h"
#include "patterns/UVPattern.h"
#include "renderer/pipeline/signals/Accumulators.h"
#include "renderer/pipeline/transforms/PaletteTransform.h"
#include "signals/Signals.h"

namespace PolarShader {
    class PolarPipelineBuilder {
        std::unique_ptr<UVPattern> pattern;
        CRGBPalette16 palette;
        fl::vector<PipelineStep> steps;
        bool built = false;
        // Must point to a string with static storage duration (presets use literals).
        const char *name;
        std::shared_ptr<PipelineContext> context = std::make_shared<PipelineContext>();
        DepthSignal depthSignal = constantDepth(static_cast<uint32_t>(random16()) << CARTESIAN_FRAC_BITS);

    public:
        PolarPipelineBuilder(
            std::unique_ptr<UVPattern> pattern,
            const CRGBPalette16 &palette,
            const char *name
        ) : pattern(std::move(pattern)),
            palette(palette),
            name(name ? name : "unnamed") {
        }

        /**
         * @brief The following methods use C++ ref-qualifiers (& and &&) to optimize 
         * both named (lvalue) and temporary (rvalue) builder usage.
         * 
         * - Methods ending in '&' (Lvalue): Support step-by-step building where the 
         *   builder is stored in a named variable.
         * - Methods ending in '&&' (Rvalue): Support fluent "one-liner" chaining. 
         *   They enable move semantics for internal containers (fl::vector, unique_ptr), 
         *   minimizing memory allocations and copies on microcontrollers.
         */

        PolarPipelineBuilder &setDepthSignal(DepthSignal signal) & {
            if (built) return *this;
            if (!signal) {
                return *this;
            }
            depthSignal = std::move(signal);
            return *this;
        }

        PolarPipelineBuilder &&setDepthSignal(DepthSignal signal) && {
            if (built) return std::move(*this);
            if (!signal) return std::move(*this);

            depthSignal = std::move(signal);
            return std::move(*this);
        }

        PolarPipelineBuilder &setDepthSignal(SFracQ0_16Signal signal) & {
            if (built) return *this;
            if (!signal) {
                return *this;
            }
            depthSignal = depth(std::move(signal));
            return *this;
        }

        PolarPipelineBuilder &&setDepthSignal(SFracQ0_16Signal signal) && {
            if (built) return std::move(*this);
            if (!signal) return std::move(*this);

            depthSignal = depth(std::move(signal));
            return std::move(*this);
        }

        template<typename T, typename = std::enable_if_t<std::is_base_of<UVTransform, T>::value> >
        PolarPipelineBuilder &addTransform(T transform) & {
            if (built) return *this;
            steps.push_back(PipelineStep::uv(std::make_unique<T>(std::move(transform))));
            return *this;
        }

        template<typename T, typename = std::enable_if_t<std::is_base_of<UVTransform, T>::value> >
        PolarPipelineBuilder &&addTransform(T transform) && {
            if (built) return std::move(*this);
            steps.push_back(PipelineStep::uv(std::make_unique<T>(std::move(transform))));
            return std::move(*this);
        }

        PolarPipelineBuilder &addPaletteTransform(PaletteTransform transform) & {
            if (built) return *this;
            steps.push_back(PipelineStep::palette(std::make_unique<PaletteTransform>(std::move(transform))));
            return *this;
        }

        PolarPipelineBuilder &&addPaletteTransform(PaletteTransform transform) && {
            if (built) return std::move(*this);
            steps.push_back(PipelineStep::palette(std::make_unique<PaletteTransform>(std::move(transform))));
            return std::move(*this);
        }

        PolarPipeline build();
    };
}

#endif //POLAR_SHADER_SPECS_POLARPIPELINEBUILDER_H
