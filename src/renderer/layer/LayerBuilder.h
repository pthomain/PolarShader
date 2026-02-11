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

#ifndef POLAR_SHADER_PIPELINE_LAYERBUILDER_H
#define POLAR_SHADER_PIPELINE_LAYERBUILDER_H

#include <memory>
#include <type_traits>
#include <utility>
#ifdef ARDUINO
#include "FastLED.h"
#else
#include "native/FastLED.h"
#endif
#include "../pipeline/PipelineContext.h"
#include "Layer.h"
#include "../pipeline/patterns/base/UVPattern.h"
#include "renderer/pipeline/signals/accumulators/Accumulators.h"
#include "renderer/pipeline/transforms/PaletteTransform.h"
#include "renderer/pipeline/signals/Signals.h"

namespace PolarShader {
    class LayerBuilder {
        std::unique_ptr<UVPattern> pattern;
        CRGBPalette16 palette;
        fl::vector<PipelineStep> steps;
        bool built = false;
        // Must point to a string with static storage duration (presets use literals).
        const char *name;
        std::shared_ptr<PipelineContext> context = std::make_shared<PipelineContext>();
        DepthSignal depthSignal = constantDepth(static_cast<uint32_t>(random16()) << CARTESIAN_FRAC_BITS);
        FracQ0_16 alpha{0xFFFFu};
        BlendMode blendMode{BlendMode::Normal};

    public:
        LayerBuilder(
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

        LayerBuilder &setAlpha(FracQ0_16 a) & {
            alpha = a;
            return *this;
        }

        LayerBuilder &&setAlpha(FracQ0_16 a) && {
            alpha = a;
            return std::move(*this);
        }

        LayerBuilder &setBlendMode(BlendMode mode) & {
            blendMode = mode;
            return *this;
        }

        LayerBuilder &&setBlendMode(BlendMode mode) && {
            blendMode = mode;
            return std::move(*this);
        }

        LayerBuilder &setDepthSignal(DepthSignal signal) & {
            if (built) return *this;
            if (!signal) {
                return *this;
            }
            depthSignal = std::move(signal);
            return *this;
        }

        LayerBuilder &&setDepthSignal(DepthSignal signal) && {
            if (built) return std::move(*this);
            if (!signal) return std::move(*this);

            depthSignal = std::move(signal);
            return std::move(*this);
        }

        LayerBuilder &setDepthSignal(SFracQ0_16Signal signal) & {
            if (built) return *this;
            if (!signal) {
                return *this;
            }
            depthSignal = depth(std::move(signal));
            return *this;
        }

        LayerBuilder &&setDepthSignal(SFracQ0_16Signal signal) && {
            if (built) return std::move(*this);
            if (!signal) return std::move(*this);

            depthSignal = depth(std::move(signal));
            return std::move(*this);
        }

        template<typename T, typename = std::enable_if_t<std::is_base_of<UVTransform, T>::value> >
        LayerBuilder &addTransform(T transform) & {
            if (built) return *this;
            steps.push_back(PipelineStep::uv(std::make_unique<T>(std::move(transform))));
            return *this;
        }

        template<typename T, typename = std::enable_if_t<std::is_base_of<UVTransform, T>::value> >
        LayerBuilder &&addTransform(T transform) && {
            if (built) return std::move(*this);
            steps.push_back(PipelineStep::uv(std::make_unique<T>(std::move(transform))));
            return std::move(*this);
        }

        LayerBuilder &addPaletteTransform(PaletteTransform transform) & {
            if (built) return *this;
            steps.push_back(PipelineStep::palette(std::make_unique<PaletteTransform>(std::move(transform))));
            return *this;
        }

        LayerBuilder &&addPaletteTransform(PaletteTransform transform) && {
            if (built) return std::move(*this);
            steps.push_back(PipelineStep::palette(std::make_unique<PaletteTransform>(std::move(transform))));
            return std::move(*this);
        }

        // duplicates
        LayerBuilder &withDepth(DepthSignal signal) & {
            return setDepthSignal(std::move(signal));
        }

        LayerBuilder &&withDepth(DepthSignal signal) && {
            return std::move(setDepthSignal(std::move(signal)));
        }

        Layer build();
    };
}

#endif //POLAR_SHADER_PIPELINE_LAYERBUILDER_H
