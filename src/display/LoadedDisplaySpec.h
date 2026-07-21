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

#ifndef POLARSHADER_LOADEDDISPLAYSPEC_H
#define POLARSHADER_LOADEDDISPLAYSPEC_H

#include <cstdint>
#include <string>
#include <vector>

#include "display/PolarDisplaySpec.h"

namespace PolarShader {
    // Runtime-loaded display described by a .PDS blob. Holds an explicit
    // per-LED polar table, an optional per-LED raster table (all-or-nothing),
    // and a parsed OUTPUT_CONFIG. This is a web/native preview + codec vehicle,
    // NOT an on-hardware driver — real MCU deployment needs generated
    // backend/build code (see docs/pds-format.md).

    // OUTPUT_CONFIG backend kinds. Append, never renumber.
    enum class PdsBackendKind : uint8_t {
        NONE = 0,
        FASTLED = 1,
        SMARTMATRIX = 2,
        // 3.. reserved/unknown: preserved verbatim in OutputConfig::rawBody.
    };

    struct FastLedOutputConfig {
        std::string target_id;
        uint8_t chipset{0};        // PDS_CHIPSET
        uint16_t data_pin{0xFFFF}; // 0xFFFF = none
        uint16_t clock_pin{0xFFFF};// 0xFFFF = none
        uint8_t rgb_order{0};      // PDS_RGB_ORDER
        uint8_t brightness{0};
        uint8_t refresh_ms{0};
        uint32_t color_correction{0};
    };

    struct SmartMatrixOutputConfig {
        std::string target_id;
        uint16_t panel_width{0};
        uint16_t panel_height{0};
        uint16_t chain_width{0};
        uint16_t chain_height{0};
        uint8_t subsample{1};
        uint8_t refresh_depth{0};
        uint8_t dma_buffer_rows{0};
        uint8_t panel_type{0};     // PDS_PANEL_TYPE
        uint32_t matrix_options{0};
        uint32_t background_options{0};
        uint8_t brightness{0};
        uint8_t refresh_ms{0};
    };

    struct OutputConfig {
        uint8_t backend_kind{static_cast<uint8_t>(PdsBackendKind::NONE)};
        FastLedOutputConfig fastled{};
        SmartMatrixOutputConfig smartmatrix{};
        // For unknown backends (backend_kind >= 3): the verbatim backend body
        // so encode(decode(x)) is byte-identical.
        std::vector<uint8_t> rawBody{};
    };

    struct RasterCell {
        uint16_t x{0};
        uint16_t y{0};
    };

    class LoadedDisplaySpec : public PolarDisplaySpec {
    public:
        // Web-simulation compatibility values only — NOT serialized deployment
        // truth (that lives in OUTPUT_CONFIG). The composer relies on every web
        // display instantiating the same addLeds<WS2812, D1, GRB> template so
        // FastLED's strip registry tolerates destroy/reconstruct on switch.
        static constexpr int LED_PIN = D1;
        static constexpr EOrder RGB_ORDER = GRB;

        // DISPLAY_INFO (optional in the stream, recorded for round-trip).
        bool hasInfo{false};
        uint8_t sourceKind{0};
        std::string name;

        std::vector<PolarCoords> leds;
        bool hasRaster{false};
        uint16_t rasterWidth{0};
        uint16_t rasterHeight{0};
        std::vector<RasterCell> rasterCells; // size == leds.size() when hasRaster
        OutputConfig output{};

        uint16_t numSegments() const override { return 1; }

        uint16_t nbLeds() const override {
            return static_cast<uint16_t>(leds.size());
        }

        uint16_t segmentSize(uint16_t) const override {
            return static_cast<uint16_t>(leds.size());
        }

        PolarCoords toPolarCoords(uint16_t pixelIndex) const override {
            if (pixelIndex >= leds.size()) {
                return {u0x16(0), u0x16(0)};
            }
            return leds[pixelIndex];
        }

        RenderPoint toRenderPoint(uint16_t pixelIndex) const override {
            RenderPoint point = makePolarRenderPoint(toPolarCoords(pixelIndex));
            if (hasRaster && pixelIndex < rasterCells.size()) {
                point.raster.valid = true;
                point.raster.x = rasterCells[pixelIndex].x;
                point.raster.y = rasterCells[pixelIndex].y;
                point.raster.width = rasterWidth;
                point.raster.height = rasterHeight;
            }
            return point;
        }
    };
}
#endif //POLARSHADER_LOADEDDISPLAYSPEC_H
