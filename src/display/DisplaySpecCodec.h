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

#ifndef POLARSHADER_DISPLAYSPECCODEC_H
#define POLARSHADER_DISPLAYSPECCODEC_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "display/LoadedDisplaySpec.h"
#include "renderer/pipeline/patterns/base/RasterAutomaton.h"

// Header-only .PDS ("Polar Display Spec") codec. Sectioned little-endian
// binary format modelled on .PSC. See docs/pds-format.md for the full spec.
//
// Being header-only + inline keeps it out of MCU firmware (nothing new
// auto-compiles); it is compiled only into native tests and the WASM composer.

namespace PolarShader {
    // --- Format constants ---------------------------------------------------
    static constexpr uint8_t PDS_MAGIC[4] = {0x50, 0x44, 0x53, 0x00}; // "PDS\0"
    static constexpr uint8_t PDS_VERSION = 0x01;

    enum class PdsSectionType : uint8_t {
        DISPLAY_INFO = 0x01,
        GEOMETRY_POLAR = 0x02,
        RASTER_GRID = 0x03,
        OUTPUT_CONFIG = 0x04,
    };

    static constexpr uint8_t PDS_SECTION_FLAG_CRITICAL = 0x01;

    // PDS-defined enum ceilings (append-only tables; never renumber).
    // PDS_CHIPSET: 0=WS2812, 1=WS2812B, 2=SK6812, 3=APA102
    static constexpr uint8_t PDS_CHIPSET_MAX = 3;
    // PDS_RGB_ORDER: 0=RGB,1=RBG,2=GRB,3=GBR,4=BRG,5=BGR
    static constexpr uint8_t PDS_RGB_ORDER_MAX = 5;
    // PDS_PANEL_TYPE: 0=HUB75_32x32,1=HUB75_64x32,2=HUB75_64x64
    static constexpr uint8_t PDS_PANEL_TYPE_MAX = 2;

    // Clock-based (SPI) chipsets require a clock pin; clockless ones forbid it.
    inline bool pdsChipsetIsClockBased(uint8_t chipset) {
        return chipset == 3; // APA102 (extend as SPI chipsets are added)
    }

    enum class DisplaySpecDecodeStatus : uint8_t {
        OK = 0,
        BAD_MAGIC,
        BAD_VERSION,
        TRUNCATED,
        TRAILING_BYTES,
        MISSING_GEOMETRY,
        EMPTY_GEOMETRY,
        TOO_LARGE,
        BAD_RASTER,
        DUP_SECTION,
        BAD_SECTION_ORDER,
        UNKNOWN_CRITICAL_SECTION,
        BAD_SECTION_FLAGS,
        BAD_OUTPUT_ENUM,
        BAD_OUTPUT_CONFIG,
        SECTION_TRAILING_BYTES,
    };

    enum class DeployStatus : uint8_t {
        DEPLOYABLE = 0,
        NO_BACKEND,          // backend_kind == NONE
        UNSUPPORTED_BACKEND, // backend_kind >= 3 (unknown/preserved)
        RASTER_NOT_DENSE,    // SmartMatrix without a dense/complete raster
    };

    namespace detail {
        // --- Byte writer ----------------------------------------------------
        class PdsWriter {
        public:
            void u8(uint8_t v) { bytes_.push_back(v); }

            void u16(uint16_t v) {
                bytes_.push_back(static_cast<uint8_t>(v & 0xFF));
                bytes_.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
            }

            void u32(uint32_t v) {
                bytes_.push_back(static_cast<uint8_t>(v & 0xFF));
                bytes_.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
                bytes_.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
                bytes_.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
            }

            void raw(const uint8_t *p, std::size_t n) {
                bytes_.insert(bytes_.end(), p, p + n);
            }

            void stringU8(const std::string &s) {
                u8(static_cast<uint8_t>(s.size()));
                raw(reinterpret_cast<const uint8_t *>(s.data()), s.size());
            }

            void stringU16(const std::string &s) {
                u16(static_cast<uint16_t>(s.size()));
                raw(reinterpret_cast<const uint8_t *>(s.data()), s.size());
            }

            std::size_t size() const { return bytes_.size(); }
            std::vector<uint8_t> &buffer() { return bytes_; }

        private:
            std::vector<uint8_t> bytes_;
        };

        // --- Byte reader (failure via ok flag; no exceptions) ---------------
        class PdsReader {
        public:
            PdsReader(const uint8_t *data, std::size_t len)
                : data_(data), len_(len) {}

            bool ok() const { return ok_; }
            std::size_t pos() const { return pos_; }
            std::size_t remaining() const { return len_ - pos_; }

            bool need(std::size_t n) {
                if (!ok_ || pos_ + n > len_) {
                    ok_ = false;
                    return false;
                }
                return true;
            }

            uint8_t u8() {
                if (!need(1)) return 0;
                return data_[pos_++];
            }

            uint16_t u16() {
                if (!need(2)) return 0;
                uint16_t v = static_cast<uint16_t>(data_[pos_]) |
                             (static_cast<uint16_t>(data_[pos_ + 1]) << 8);
                pos_ += 2;
                return v;
            }

            uint32_t u32() {
                if (!need(4)) return 0;
                uint32_t v = static_cast<uint32_t>(data_[pos_]) |
                             (static_cast<uint32_t>(data_[pos_ + 1]) << 8) |
                             (static_cast<uint32_t>(data_[pos_ + 2]) << 16) |
                             (static_cast<uint32_t>(data_[pos_ + 3]) << 24);
                pos_ += 4;
                return v;
            }

            std::string stringU16() {
                uint16_t n = u16();
                if (!need(n)) return {};
                std::string s(reinterpret_cast<const char *>(data_ + pos_), n);
                pos_ += n;
                return s;
            }

            std::string stringU8() {
                uint8_t n = u8();
                if (!need(n)) return {};
                std::string s(reinterpret_cast<const char *>(data_ + pos_), n);
                pos_ += n;
                return s;
            }

            void copyRaw(std::vector<uint8_t> &out, std::size_t n) {
                if (!need(n)) return;
                out.insert(out.end(), data_ + pos_, data_ + pos_ + n);
                pos_ += n;
            }

        private:
            const uint8_t *data_;
            std::size_t len_;
            std::size_t pos_{0};
            bool ok_{true};
        };
    } // namespace detail

    // --- Encode -------------------------------------------------------------
    inline std::vector<uint8_t> encodeDisplaySpec(const LoadedDisplaySpec &spec) {
        using detail::PdsWriter;

        // Assemble each section body, then frame with type/flags/len.
        auto appendSection = [](PdsWriter &out, PdsSectionType type,
                                const std::vector<uint8_t> &body) {
            out.u8(static_cast<uint8_t>(type));
            out.u8(PDS_SECTION_FLAG_CRITICAL);
            out.u32(static_cast<uint32_t>(body.size()));
            out.raw(body.data(), body.size());
        };

        std::vector<std::pair<PdsSectionType, std::vector<uint8_t>>> sections;

        // DISPLAY_INFO (only when present, so decode∘encode is byte-identical)
        if (spec.hasInfo) {
            PdsWriter b;
            b.u8(spec.sourceKind);
            b.stringU8(spec.name);
            sections.emplace_back(PdsSectionType::DISPLAY_INFO, b.buffer());
        }

        // GEOMETRY_POLAR
        {
            PdsWriter b;
            b.u16(static_cast<uint16_t>(spec.leds.size()));
            for (const auto &led : spec.leds) {
                b.u16(static_cast<uint16_t>(raw(led.first)));
                b.u16(static_cast<uint16_t>(raw(led.second)));
            }
            sections.emplace_back(PdsSectionType::GEOMETRY_POLAR, b.buffer());
        }

        // RASTER_GRID (optional)
        if (spec.hasRaster) {
            PdsWriter b;
            b.u16(spec.rasterWidth);
            b.u16(spec.rasterHeight);
            for (const auto &cell : spec.rasterCells) {
                b.u16(cell.x);
                b.u16(cell.y);
            }
            sections.emplace_back(PdsSectionType::RASTER_GRID, b.buffer());
        }

        // OUTPUT_CONFIG (omitted when NONE ⇒ decodes back to NONE)
        if (spec.output.backend_kind != static_cast<uint8_t>(PdsBackendKind::NONE)) {
            PdsWriter b;
            b.u8(spec.output.backend_kind);
            if (spec.output.backend_kind == static_cast<uint8_t>(PdsBackendKind::FASTLED)) {
                const auto &f = spec.output.fastled;
                b.stringU16(f.target_id);
                b.u8(f.chipset);
                b.u16(f.data_pin);
                b.u16(f.clock_pin);
                b.u8(f.rgb_order);
                b.u8(f.brightness);
                b.u8(f.refresh_ms);
                b.u32(f.color_correction);
            } else if (spec.output.backend_kind ==
                       static_cast<uint8_t>(PdsBackendKind::SMARTMATRIX)) {
                const auto &m = spec.output.smartmatrix;
                b.stringU16(m.target_id);
                b.u16(m.panel_width);
                b.u16(m.panel_height);
                b.u16(m.chain_width);
                b.u16(m.chain_height);
                b.u8(m.subsample);
                b.u8(m.refresh_depth);
                b.u8(m.dma_buffer_rows);
                b.u8(m.panel_type);
                b.u32(m.matrix_options);
                b.u32(m.background_options);
                b.u8(m.brightness);
                b.u8(m.refresh_ms);
            } else {
                // Unknown backend: verbatim body.
                b.raw(spec.output.rawBody.data(), spec.output.rawBody.size());
            }
            sections.emplace_back(PdsSectionType::OUTPUT_CONFIG, b.buffer());
        }

        PdsWriter out;
        out.raw(PDS_MAGIC, 4);
        out.u8(PDS_VERSION);
        out.u8(static_cast<uint8_t>(sections.size()));
        for (const auto &s : sections) {
            appendSection(out, s.first, s.second);
        }
        return out.buffer();
    }

    // --- Decode -------------------------------------------------------------
    inline std::unique_ptr<LoadedDisplaySpec> decodeDisplaySpec(
        const uint8_t *bytes, std::size_t len,
        DisplaySpecDecodeStatus *statusOut = nullptr) {
        auto fail = [&](DisplaySpecDecodeStatus s) -> std::unique_ptr<LoadedDisplaySpec> {
            if (statusOut) *statusOut = s;
            return nullptr;
        };

        detail::PdsReader r(bytes, len);
        // Header
        for (int i = 0; i < 4; ++i) {
            if (r.u8() != PDS_MAGIC[i]) return fail(DisplaySpecDecodeStatus::BAD_MAGIC);
        }
        if (!r.ok()) return fail(DisplaySpecDecodeStatus::BAD_MAGIC);
        if (r.u8() != PDS_VERSION) return fail(DisplaySpecDecodeStatus::BAD_VERSION);
        const uint8_t sectionCount = r.u8();
        if (!r.ok()) return fail(DisplaySpecDecodeStatus::TRUNCATED);

        auto spec = std::unique_ptr<LoadedDisplaySpec>(new LoadedDisplaySpec());
        bool seenGeometry = false;
        bool seenInfo = false, seenRaster = false, seenOutput = false;
        int lastKnownType = 0; // ascending-order guard for known sections

        for (uint8_t si = 0; si < sectionCount; ++si) {
            const uint8_t type = r.u8();
            const uint8_t flags = r.u8();
            const uint32_t bodyLen = r.u32();
            if (!r.ok()) return fail(DisplaySpecDecodeStatus::TRUNCATED);
            if (flags & ~PDS_SECTION_FLAG_CRITICAL)
                return fail(DisplaySpecDecodeStatus::BAD_SECTION_FLAGS);
            const bool critical = (flags & PDS_SECTION_FLAG_CRITICAL) != 0;

            const std::size_t bodyStart = r.pos();
            if (r.remaining() < bodyLen) return fail(DisplaySpecDecodeStatus::TRUNCATED);
            const std::size_t bodyEnd = bodyStart + bodyLen;

            const bool known = (type == static_cast<uint8_t>(PdsSectionType::DISPLAY_INFO) ||
                                type == static_cast<uint8_t>(PdsSectionType::GEOMETRY_POLAR) ||
                                type == static_cast<uint8_t>(PdsSectionType::RASTER_GRID) ||
                                type == static_cast<uint8_t>(PdsSectionType::OUTPUT_CONFIG));

            if (!known) {
                if (critical) return fail(DisplaySpecDecodeStatus::UNKNOWN_CRITICAL_SECTION);
                // Skip unknown non-critical section whole (does not affect the
                // known-section ordering check).
                std::vector<uint8_t> discard;
                r.copyRaw(discard, bodyLen);
                if (!r.ok()) return fail(DisplaySpecDecodeStatus::TRUNCATED);
                continue;
            }

            // Ascending order + duplicate checks for known sections.
            if (type <= lastKnownType) {
                // Equal → duplicate singleton; less → out of order.
                if (type == lastKnownType)
                    return fail(DisplaySpecDecodeStatus::DUP_SECTION);
                return fail(DisplaySpecDecodeStatus::BAD_SECTION_ORDER);
            }
            lastKnownType = type;

            switch (static_cast<PdsSectionType>(type)) {
                case PdsSectionType::DISPLAY_INFO: {
                    if (seenInfo) return fail(DisplaySpecDecodeStatus::DUP_SECTION);
                    seenInfo = true;
                    spec->hasInfo = true;
                    spec->sourceKind = r.u8();
                    spec->name = r.stringU8();
                    break;
                }
                case PdsSectionType::GEOMETRY_POLAR: {
                    if (seenGeometry) return fail(DisplaySpecDecodeStatus::DUP_SECTION);
                    seenGeometry = true;
                    const uint16_t ledCount = r.u16();
                    if (!r.ok()) return fail(DisplaySpecDecodeStatus::TRUNCATED);
                    if (ledCount == 0) return fail(DisplaySpecDecodeStatus::EMPTY_GEOMETRY);
                    if (ledCount > POLAR_SHADER_MAX_RASTER_CELLS)
                        return fail(DisplaySpecDecodeStatus::TOO_LARGE);
                    spec->leds.reserve(ledCount);
                    for (uint16_t i = 0; i < ledCount; ++i) {
                        const uint16_t a = r.u16();
                        const uint16_t rad = r.u16();
                        spec->leds.push_back({u0x16(a), u0x16(rad)});
                    }
                    if (!r.ok()) return fail(DisplaySpecDecodeStatus::TRUNCATED);
                    break;
                }
                case PdsSectionType::RASTER_GRID: {
                    if (seenRaster) return fail(DisplaySpecDecodeStatus::DUP_SECTION);
                    seenRaster = true;
                    if (!seenGeometry) return fail(DisplaySpecDecodeStatus::MISSING_GEOMETRY);
                    const uint16_t rw = r.u16();
                    const uint16_t rh = r.u16();
                    if (!r.ok()) return fail(DisplaySpecDecodeStatus::TRUNCATED);
                    if (rw == 0 || rh == 0) return fail(DisplaySpecDecodeStatus::BAD_RASTER);
                    if (static_cast<uint32_t>(rw) * rh > POLAR_SHADER_MAX_RASTER_CELLS)
                        return fail(DisplaySpecDecodeStatus::BAD_RASTER);
                    spec->hasRaster = true;
                    spec->rasterWidth = rw;
                    spec->rasterHeight = rh;
                    spec->rasterCells.reserve(spec->leds.size());
                    for (std::size_t i = 0; i < spec->leds.size(); ++i) {
                        const uint16_t x = r.u16();
                        const uint16_t y = r.u16();
                        if (x >= rw || y >= rh) return fail(DisplaySpecDecodeStatus::BAD_RASTER);
                        spec->rasterCells.push_back({x, y});
                    }
                    if (!r.ok()) return fail(DisplaySpecDecodeStatus::TRUNCATED);
                    break;
                }
                case PdsSectionType::OUTPUT_CONFIG: {
                    if (seenOutput) return fail(DisplaySpecDecodeStatus::DUP_SECTION);
                    seenOutput = true;
                    const uint8_t backend = r.u8();
                    if (!r.ok()) return fail(DisplaySpecDecodeStatus::TRUNCATED);
                    spec->output.backend_kind = backend;
                    if (backend == static_cast<uint8_t>(PdsBackendKind::FASTLED)) {
                        auto &f = spec->output.fastled;
                        f.target_id = r.stringU16();
                        f.chipset = r.u8();
                        f.data_pin = r.u16();
                        f.clock_pin = r.u16();
                        f.rgb_order = r.u8();
                        f.brightness = r.u8();
                        f.refresh_ms = r.u8();
                        f.color_correction = r.u32();
                        if (!r.ok()) return fail(DisplaySpecDecodeStatus::TRUNCATED);
                        if (f.chipset > PDS_CHIPSET_MAX || f.rgb_order > PDS_RGB_ORDER_MAX)
                            return fail(DisplaySpecDecodeStatus::BAD_OUTPUT_ENUM);
                        if (f.data_pin == 0xFFFF)
                            return fail(DisplaySpecDecodeStatus::BAD_OUTPUT_CONFIG);
                        if (pdsChipsetIsClockBased(f.chipset)) {
                            if (f.clock_pin == 0xFFFF)
                                return fail(DisplaySpecDecodeStatus::BAD_OUTPUT_CONFIG);
                        } else if (f.clock_pin != 0xFFFF) {
                            return fail(DisplaySpecDecodeStatus::BAD_OUTPUT_CONFIG);
                        }
                    } else if (backend == static_cast<uint8_t>(PdsBackendKind::SMARTMATRIX)) {
                        auto &m = spec->output.smartmatrix;
                        m.target_id = r.stringU16();
                        m.panel_width = r.u16();
                        m.panel_height = r.u16();
                        m.chain_width = r.u16();
                        m.chain_height = r.u16();
                        m.subsample = r.u8();
                        m.refresh_depth = r.u8();
                        m.dma_buffer_rows = r.u8();
                        m.panel_type = r.u8();
                        m.matrix_options = r.u32();
                        m.background_options = r.u32();
                        m.brightness = r.u8();
                        m.refresh_ms = r.u8();
                        if (!r.ok()) return fail(DisplaySpecDecodeStatus::TRUNCATED);
                        if (m.panel_type > PDS_PANEL_TYPE_MAX)
                            return fail(DisplaySpecDecodeStatus::BAD_OUTPUT_ENUM);
                        if (m.panel_width == 0 || m.panel_height == 0 ||
                            m.chain_width == 0 || m.chain_height == 0 ||
                            m.subsample < 1 || m.refresh_depth == 0 || m.dma_buffer_rows == 0)
                            return fail(DisplaySpecDecodeStatus::BAD_OUTPUT_CONFIG);
                        // When a raster is present it must equal the post-subsample
                        // render resolution derived from the full physical size.
                        if (spec->hasRaster) {
                            const uint32_t physW = static_cast<uint32_t>(m.panel_width) * m.chain_width;
                            const uint32_t physH = static_cast<uint32_t>(m.panel_height) * m.chain_height;
                            if (physW % m.subsample != 0 || physH % m.subsample != 0)
                                return fail(DisplaySpecDecodeStatus::BAD_OUTPUT_CONFIG);
                            if (spec->rasterWidth != physW / m.subsample ||
                                spec->rasterHeight != physH / m.subsample)
                                return fail(DisplaySpecDecodeStatus::BAD_OUTPUT_CONFIG);
                        }
                    } else if (backend == static_cast<uint8_t>(PdsBackendKind::NONE)) {
                        // Explicit NONE with a (necessarily empty) body — allowed.
                    } else {
                        // Unknown backend: preserve the remaining body verbatim.
                        const std::size_t rawLen = bodyEnd - r.pos();
                        spec->output.rawBody.clear();
                        r.copyRaw(spec->output.rawBody, rawLen);
                        if (!r.ok()) return fail(DisplaySpecDecodeStatus::TRUNCATED);
                    }
                    break;
                }
                default:
                    break;
            }

            // Known section body must be consumed exactly (except unknown
            // backend, which absorbs the whole remaining body above).
            if (r.pos() != bodyEnd)
                return fail(DisplaySpecDecodeStatus::SECTION_TRAILING_BYTES);
        }

        if (!seenGeometry) return fail(DisplaySpecDecodeStatus::MISSING_GEOMETRY);
        if (r.pos() != len) return fail(DisplaySpecDecodeStatus::TRAILING_BYTES);

        if (statusOut) *statusOut = DisplaySpecDecodeStatus::OK;
        return spec;
    }

    // --- Deployability (layer b): NOT a decode error ------------------------
    inline DeployStatus validateDeployability(const LoadedDisplaySpec &spec) {
        const uint8_t backend = spec.output.backend_kind;
        if (backend == static_cast<uint8_t>(PdsBackendKind::NONE))
            return DeployStatus::NO_BACKEND;
        if (backend != static_cast<uint8_t>(PdsBackendKind::FASTLED) &&
            backend != static_cast<uint8_t>(PdsBackendKind::SMARTMATRIX))
            return DeployStatus::UNSUPPORTED_BACKEND;
        if (backend == static_cast<uint8_t>(PdsBackendKind::SMARTMATRIX)) {
            // Requires a dense, complete raster: a permutation of the full
            // rectangle so the row-major MatrixDisplaySpec wrapper can be
            // generated.
            if (!spec.hasRaster) return DeployStatus::RASTER_NOT_DENSE;
            const uint32_t cells = static_cast<uint32_t>(spec.rasterWidth) * spec.rasterHeight;
            if (spec.leds.size() != cells) return DeployStatus::RASTER_NOT_DENSE;
            std::vector<bool> seen(cells, false);
            for (const auto &c : spec.rasterCells) {
                if (c.x >= spec.rasterWidth || c.y >= spec.rasterHeight)
                    return DeployStatus::RASTER_NOT_DENSE;
                const uint32_t idx = static_cast<uint32_t>(c.y) * spec.rasterWidth + c.x;
                if (seen[idx]) return DeployStatus::RASTER_NOT_DENSE;
                seen[idx] = true;
            }
        }
        return DeployStatus::DEPLOYABLE;
    }
}
#endif //POLARSHADER_DISPLAYSPECCODEC_H
