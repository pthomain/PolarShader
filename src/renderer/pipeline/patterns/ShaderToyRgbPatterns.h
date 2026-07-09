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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_SHADERTOYRGBPATTERNS_H
#define POLAR_SHADER_PIPELINE_PATTERNS_SHADERTOYRGBPATTERNS_H

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include "renderer/pipeline/signals/Signals.h"
#include <memory>

namespace PolarShader {
    /**
     * Fixed-point RGB UV ports of selected ShaderToy fragment shaders.
     *
     * They emit full RGB samples through UVLayer::Rgb. ShaderToy UI sliders,
     * where present, are represented by pattern-owned signals sampled once per
     * frame.
     */

    // Port of https://www.shadertoy.com/view/sX2SzG
    // Interactive Rocaille version, based on the XorDev original linked there.
    class RocaillePattern : public UVPattern {
    public:
        explicit RocaillePattern(
            Sf16Signal scale = constant(333),
            Sf16Signal length = constant(429),
            Sf16Signal detail = constant(222),
            Sf16Signal turbulence = constant(500),
            Sf16Signal frequency = constant(333),
            Sf16Signal speed = constant(333),
            Sf16Signal layers = constant(727),
            Sf16Signal hue = constant(500),
            Sf16Signal glow = constant(429)
        );

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;
        UVMap layer(const std::shared_ptr<PipelineContext> &context) const override;
        UVLayer uvLayer(const std::shared_ptr<PipelineContext> &context) const override;

    private:
        struct State;
        struct Functor;
        std::shared_ptr<State> state;
    };

    // Port of https://www.shadertoy.com/view/XsXXDn
    // Original source requests credit to Danilo Guanabara.
    class ProteanCloudsPattern : public UVPattern {
    public:
        explicit ProteanCloudsPattern(
            Sf16Signal speed = constant(1000),
            Sf16Signal warp = constant(500),
            Sf16Signal frequency = constant(500),
            Sf16Signal brightness = constant(500)
        );

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;
        UVMap layer(const std::shared_ptr<PipelineContext> &context) const override;
        UVLayer uvLayer(const std::shared_ptr<PipelineContext> &context) const override;

    private:
        struct State;
        struct Functor;
        std::shared_ptr<State> state;
    };

    // Port of https://www.shadertoy.com/view/tlVGDt
    class OctgramsPattern : public UVPattern {
    public:
        explicit OctgramsPattern(
            Sf16Signal speed = constant(1000),
            Sf16Signal travel = constant(500),
            Sf16Signal pulse = constant(500),
            Sf16Signal density = constant(500),
            Sf16Signal glow = constant(500)
        );

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;
        UVMap layer(const std::shared_ptr<PipelineContext> &context) const override;
        UVLayer uvLayer(const std::shared_ptr<PipelineContext> &context) const override;

    private:
        struct State;
        struct Functor;
        std::shared_ptr<State> state;
    };

    // Port of https://www.shadertoy.com/view/7ctGz4
    class RotatingSquaresPattern : public UVPattern {
    public:
        explicit RotatingSquaresPattern(
            Sf16Signal speed = constant(1000),
            Sf16Signal thickness = constant(375),
            Sf16Signal pulse = constant(333),
            Sf16Signal brightness = constant(500)
        );

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;
        UVMap layer(const std::shared_ptr<PipelineContext> &context) const override;
        UVLayer uvLayer(const std::shared_ptr<PipelineContext> &context) const override;

    private:
        struct State;
        struct Functor;
        std::shared_ptr<State> state;
    };

    // Port of https://www.shadertoy.com/view/MfjyWK
    // "Starry planes", CC0 per source header.
    class StarryPlanesPattern : public UVPattern {
    public:
        explicit StarryPlanesPattern(
            Sf16Signal speed = constant(1000),
            Sf16Signal planeSpacing = constant(500),
            Sf16Signal starSize = constant(400),
            Sf16Signal path = constant(500),
            Sf16Signal brightness = constant(500)
        );

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;
        UVMap layer(const std::shared_ptr<PipelineContext> &context) const override;
        UVLayer uvLayer(const std::shared_ptr<PipelineContext> &context) const override;

    private:
        struct State;
        struct Functor;
        std::shared_ptr<State> state;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_SHADERTOYRGBPATTERNS_H
