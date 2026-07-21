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

/*
 * .PDS exporter (native desktop tool, own main()).
 *
 * Dumps each built-in display spec to displays/<name>.pds, so the web composer
 * (and codec test fixtures) have canonical .PDS files for every hardware
 * layout. Geometry + raster come straight from the specs; the non-geometry
 * OUTPUT_CONFIG fields (which do NOT exist on the specs) come from the explicit
 * BackendConfig table below — the single documented source of those values,
 * mirroring the driver / platformio.ini constants they claim to reflect.
 *
 * Compiled ONLY by [env:native_export_pds]; every other env excludes
 * tools/export_pds.cpp so no second main() is ever linked.
 * Run:
 *   pio run -e native_export_pds && .pio/build/native_export_pds/program
 */

#include "native/Arduino.h"
#include "native/FastLED.h"

#include "FabricDisplaySpec.h"
#include "Fabric32x8DisplaySpec.h"
#include "FibonacciDisplaySpec.h"
#include "Matrix128x128DisplaySpec.h"
#include "RoundDisplaySpec.h"
#include "display/DisplaySpecCodec.h"
#include "display/LoadedDisplaySpec.h"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace PolarShader;

namespace {

    const char *kOutDir = "displays";

    // Convert any built-in PolarDisplaySpec into an in-memory LoadedDisplaySpec
    // (geometry + optional raster only). OUTPUT_CONFIG is filled by the caller.
    LoadedDisplaySpec fromSpec(const PolarDisplaySpec &spec) {
        LoadedDisplaySpec out;
        const uint16_t n = spec.nbLeds();
        for (uint16_t i = 0; i < n; ++i) {
            const RenderPoint rp = spec.toRenderPoint(i);
            out.leds.push_back({rp.angle, rp.radius});
            if (rp.raster.valid) {
                if (!out.hasRaster) {
                    out.hasRaster = true;
                    out.rasterWidth = rp.raster.width;
                    out.rasterHeight = rp.raster.height;
                }
                out.rasterCells.push_back({rp.raster.x, rp.raster.y});
            }
        }
        return out;
    }

    void setFastLed(LoadedDisplaySpec &s, const std::string &targetId,
                    uint16_t dataPin, uint8_t rgbOrder) {
        s.output.backend_kind = static_cast<uint8_t>(PdsBackendKind::FASTLED);
        s.output.fastled.target_id = targetId;
        s.output.fastled.chipset = 0;      // PDS_CHIPSET WS2812 (FastLedDisplay hardcodes WS2812)
        s.output.fastled.data_pin = dataPin;
        s.output.fastled.clock_pin = 0xFFFF; // clockless
        s.output.fastled.rgb_order = rgbOrder;
        s.output.fastled.brightness = 20;  // FastLedDisplay default
        s.output.fastled.refresh_ms = 30;  // FastLedDisplay default
        s.output.fastled.color_correction = 0; // TypicalLEDStrip = default
    }

    void setSmartMatrix(LoadedDisplaySpec &s) {
        s.output.backend_kind = static_cast<uint8_t>(PdsBackendKind::SMARTMATRIX);
        s.output.smartmatrix.target_id = "teensy41_matrix";
        // Matrix128x128DisplaySpec: PANEL 64x64, chain 2x2, subsample 2 →
        // (64*2)/2 = 64 → 64×64 render grid.
        s.output.smartmatrix.panel_width = Matrix128x128DisplaySpec::PANEL_WIDTH;
        s.output.smartmatrix.panel_height = Matrix128x128DisplaySpec::PANEL_HEIGHT;
        s.output.smartmatrix.chain_width = 2;
        s.output.smartmatrix.chain_height = 2;
        s.output.smartmatrix.subsample = Matrix128x128DisplaySpec::SUBSAMPLE;
        s.output.smartmatrix.refresh_depth = 24;  // SmartMatrixDisplay.cpp kColorDepth
        s.output.smartmatrix.dma_buffer_rows = 8;  // SmartMatrixDisplay.cpp kDmaBufferRows
        s.output.smartmatrix.panel_type = 2;       // PDS_PANEL_TYPE HUB75_64x64
        // Library-specific raw payloads (deploy tool owns interpretation):
        // SM_HUB75_OPTIONS_C_SHAPE_STACKING = 1, SM_BACKGROUND_OPTIONS_NONE = 0.
        s.output.smartmatrix.matrix_options = 1;
        s.output.smartmatrix.background_options = 0;
        s.output.smartmatrix.brightness = 20;
        s.output.smartmatrix.refresh_ms = 30;
    }

    bool writeFile(const std::string &path, const std::vector<uint8_t> &bytes) {
        std::ofstream f(path, std::ios::binary);
        if (!f) return false;
        f.write(reinterpret_cast<const char *>(bytes.data()),
                static_cast<std::streamsize>(bytes.size()));
        return static_cast<bool>(f);
    }

    bool dump(const std::string &name, uint8_t sourceKind, LoadedDisplaySpec spec) {
        spec.hasInfo = true;
        spec.sourceKind = sourceKind;
        spec.name = name;
        const auto bytes = encodeDisplaySpec(spec);
        const std::string path = std::string(kOutDir) + "/" + name + ".pds";
        if (!writeFile(path, bytes)) {
            std::fprintf(stderr, "failed to write %s\n", path.c_str());
            return false;
        }
        std::printf("wrote %s (%zu bytes, %u leds, raster=%s)\n", path.c_str(),
                    bytes.size(), spec.nbLeds(), spec.hasRaster ? "yes" : "no");
        return true;
    }
}

int main() {
    std::error_code ec;
    std::filesystem::create_directories(kOutDir, ec);

    bool ok = true;

    {
        RoundDisplaySpec spec;
        LoadedDisplaySpec loaded = fromSpec(spec);
        setFastLed(loaded, "seeed_xiao_rp2040_round",
                   static_cast<uint16_t>(RoundDisplaySpec::LED_PIN),
                   static_cast<uint8_t>(RoundDisplaySpec::RGB_ORDER));
        ok &= dump("round", 1, std::move(loaded));
    }
    {
        FibonacciDisplaySpec spec;
        LoadedDisplaySpec loaded = fromSpec(spec);
        setFastLed(loaded, "seeed_xiao_rp2040_fibonacci",
                   static_cast<uint16_t>(FibonacciDisplaySpec::LED_PIN),
                   static_cast<uint8_t>(FibonacciDisplaySpec::RGB_ORDER));
        ok &= dump("fibonacci", 2, std::move(loaded));
    }
    {
        FabricDisplaySpec spec;
        LoadedDisplaySpec loaded = fromSpec(spec);
        setFastLed(loaded, "seeed_xiao_rp2040_fabric",
                   static_cast<uint16_t>(FabricDisplaySpec::LED_PIN),
                   static_cast<uint8_t>(FabricDisplaySpec::RGB_ORDER));
        ok &= dump("fabric", 3, std::move(loaded));
    }
    {
        Fabric32x8DisplaySpec spec;
        LoadedDisplaySpec loaded = fromSpec(spec);
        setFastLed(loaded, "seeed_xiao_rp2040_matrix32x8",
                   static_cast<uint16_t>(Fabric32x8DisplaySpec::LED_PIN),
                   static_cast<uint8_t>(Fabric32x8DisplaySpec::RGB_ORDER));
        ok &= dump("fabric32x8", 4, std::move(loaded));
    }
    {
        Matrix128x128DisplaySpec spec;
        LoadedDisplaySpec loaded = fromSpec(spec);
        setSmartMatrix(loaded);
        ok &= dump("matrix128", 5, std::move(loaded));
    }

    return ok ? 0 : 1;
}
