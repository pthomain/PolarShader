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

#ifndef POLAR_SHADER_TRANSFORMS_BASE_TRANSFORMS_H
#define POLAR_SHADER_TRANSFORMS_BASE_TRANSFORMS_H

#include <memory>
#include <utility>
#include "Layers.h"
#include "renderer/pipeline/PipelineContext.h"
#include "renderer/pipeline/maths/units/Units.h"

namespace PolarShader {
    class FrameTransform {
    public:
        virtual ~FrameTransform() = default;

        virtual void advanceFrame(f16 progress, TimeMillis elapsedMs) {
        };

        virtual void setContext(std::shared_ptr<PipelineContext> context) { this->context = std::move(context); }

    protected:
        std::shared_ptr<PipelineContext> context;
    };

    /**
     * @brief Standard interface for all spatial transformations in the unified UV pipeline.
     * 
     * UVTransform is the target architecture for all transforms.
     */
    class UVTransform : public virtual FrameTransform {
    public:
        UVTransform() = default;

        /**
         * @brief Composes this transform's coordinate warp under a leaf map.
         *
         * A transform is a pure coordinate warp; it never touches the sampled
         * value. Two overloads let the SAME warp compose against either the
         * scalar (UVMap) or colour (UVColourMap) leaf.
         *
         * DESIGN (see the WASM ABI NOTE on `UV` in Units.h): the warp is NOT
         * exposed as an `fl::function<UV(UV)>` and composed generically in this
         * base. Returning a two-field `UV` through fl::function's type-erased
         * invoker traps `call_indirect` under the Emscripten ABI, and — worse —
         * corrupts the module heap so a *later* recompile crashes. Instead each
         * transform implements both overloads itself, applying its warp via a
         * DIRECT static call (its private `warp(const State&, UV)`), capturing
         * only its `state` shared_ptr and the leaf — the shape of the original
         * pre-colour code. No `UV` ever flows through an fl::function.
         */
        virtual UVMap operator()(const UVMap &layer) const = 0;

        /** @brief Composes the warp under a colour leaf. */
        virtual UVColourMap operator()(const UVColourMap &layer) const = 0;
    };
}

#endif //POLAR_SHADER_TRANSFORMS_BASE_TRANSFORMS_H
