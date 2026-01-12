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

#ifndef LED_SEGMENTS_SPECS_CAMERAPRESET_H
#define LED_SEGMENTS_SPECS_CAMERAPRESET_H

#include "CameraRig.h"
#include "CameraRigs.h"

namespace LEDSegments {
    enum class CameraPreset {
        ZoomOnly,
        OscillatingOrbit,
        EllipticalOrbit,
        PendulumDrift,
        PulseZoom,
    };

    namespace CameraRigPresets {
        static CameraRig create(CameraPreset preset) {
            switch (preset) {
                case CameraPreset::ZoomOnly: return ZoomOnlyRig();
                case CameraPreset::OscillatingOrbit: return OscillatingOrbitRig();
                case CameraPreset::EllipticalOrbit: return EllipticalOrbitRig();
                case CameraPreset::PendulumDrift: return PendulumDriftRig();
                case CameraPreset::PulseZoom: return PulseZoomRig();
                default:
                    return ZoomOnlyRig();
            }
        }
    }
}

#endif //LED_SEGMENTS_SPECS_CAMERAPRESET_H
