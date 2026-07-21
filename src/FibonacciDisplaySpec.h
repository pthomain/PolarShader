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

#ifndef POLARSHADER_FIBONACCIDISPLAYSPEC_H
#define POLARSHADER_FIBONACCIDISPLAYSPEC_H

#include "display/PolarDisplaySpec.h"
#include "renderer/pipeline/maths/Maths.h"

namespace PolarShader {
    /**
     * Fibonacci phyllotaxis display.
     *
     * The physical panel is woven from 12 clockwise and 12 counter-clockwise
     * logarithmic ("Fibonacci") spirals. Their intersections bound diamond
     * cells; an LED sits in the centre of each cell. Every arm is one of the
     * 12 clockwise spirals, segmented into 20 cells running from the centre
     * (cell 0) to the rim (cell 19).
     *
     * Cell LED counts (per arm): cells 0..14 hold 1 LED, cells 15..17 hold 2,
     * cells 18..19 hold 3 -> 27 LEDs/arm, 324 LEDs total.
     *
     * Geometry (screen convention, angle raw 0..65535 == 0..2PI):
     *   angle(deg) = 30*armSlot + 15*cell + 225   (+/- tangential offset)
     *   radius     = RADIUS_U0X16[cell]              (normalised, rim == 1.0)
     * where the arm slot maps the wired segment through the assembly swap
     * (segments 0 and 1 are physically exchanged) and a reversed placement
     * order so that clockwise the arm start indices read
     * 0, 27, 297, 270, 243, 216, 189, 162, 135, 108, 81, 54.
     *
     * Wiring: arm-by-arm, centre -> rim; multi-LED cells left -> right.
     */
    class FibonacciDisplaySpec : public PolarDisplaySpec {
    public:
        static constexpr int LED_PIN = D1;
        static constexpr EOrder RGB_ORDER = GRB;
        static constexpr uint16_t NB_ARMS = 12;
        static constexpr uint16_t CELLS_PER_ARM = 20;
        static constexpr uint16_t LEDS_PER_ARM = 27;
        static constexpr uint16_t NB_SEGMENTS = NB_ARMS;
        static constexpr uint16_t NB_LEDS = NB_ARMS * LEDS_PER_ARM; // 324

        // Physical radii of the spiral (millimetres): inner edge of cell 0 to
        // outer edge (spiral end). Kept for documentation / web geometry.
        static constexpr float RADIUS_MIN_MM = 40.37f;
        static constexpr float RADIUS_MAX_MM = 228.35f;

        uint16_t numSegments() const override {
            return NB_SEGMENTS;
        }

        uint16_t nbLeds() const override {
            return NB_LEDS;
        }

        uint16_t segmentSize(uint16_t segmentIndex) const override {
            return segmentIndex < NB_SEGMENTS ? LEDS_PER_ARM : 0;
        }

        PolarCoords toPolarCoords(uint16_t pixelIndex) const override {
            if (pixelIndex >= NB_LEDS) {
                return {u0x16(0), u0x16(0)};
            }

            const uint16_t arm = pixelIndex / LEDS_PER_ARM;
            const uint16_t local = pixelIndex % LEDS_PER_ARM;

            // Resolve the cell and the LED position within a multi-LED cell.
            uint16_t cell = 0;
            uint16_t ledInCell = 0;
            for (uint16_t acc = 0; cell < CELLS_PER_ARM; ++cell) {
                const uint16_t n = ledsInCell(cell);
                if (local < acc + n) {
                    ledInCell = local - acc;
                    break;
                }
                acc += n;
            }

            // Assembly swap: wired segments 0 and 1 are physically exchanged.
            const uint16_t physArm = (arm == 0) ? 1 : (arm == 1 ? 0 : arm);
            // Reversed placement so clockwise start indices descend after 0,27.
            const uint16_t armSlot = static_cast<uint16_t>((20u - physArm) % NB_ARMS);

            const uint16_t baseDeg = static_cast<uint16_t>(30u * armSlot + 15u * cell + 225u);
            uint32_t baseRaw = (static_cast<uint32_t>(baseDeg) * 0x10000u + 180u) / 360u;

            // Tangential spread inside a multi-LED cell (0.22 of the 30deg cell
            // width per step): +/-3.3deg for pairs, +/-6.6deg for triples.
            int32_t offsetRaw = 0;
            const uint16_t n = ledsInCell(cell);
            if (n == 2) {
                offsetRaw = (ledInCell == 0) ? -601 : 601;
            } else if (n == 3) {
                offsetRaw = (static_cast<int32_t>(ledInCell) - 1) * 1201;
            }

            const uint16_t angle_raw = static_cast<uint16_t>(
                (static_cast<int32_t>(baseRaw) + offsetRaw) & 0xFFFF);

            return {u0x16(angle_raw), u0x16(RADIUS_U0X16[cell])};
        }

        // Normalised cell radii (u0x16 raw, rim == 65535). Log-spiral growth
        // radius(cell) = (Rmax/Rmin)^((cell-19)/20), so cell 19 == 1.0.
        static constexpr uint16_t RADIUS_U0X16[CELLS_PER_ARM] = {
            12635u, 13778u, 15025u, 16385u, 17868u, 19485u, 21248u, 23171u, 25268u, 27555u,
            30049u, 32768u, 35734u, 38968u, 42495u, 46341u, 50535u, 55109u, 60096u, 65535u,
        };

        static constexpr uint16_t ledsInCell(uint16_t cell) {
            if (cell >= 18) return 3; // cells 18,19 (the last two)
            if (cell >= 15) return 2; // cells 15,16,17
            return 1;
        }
    };
}
#endif //POLARSHADER_FIBONACCIDISPLAYSPEC_H
