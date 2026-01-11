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

#ifndef LED_SEGMENTS_CAMERA_RIG_PRESETS_H
#define LED_SEGMENTS_CAMERA_RIG_PRESETS_H

#include "CameraRig.h"

namespace LEDSegments {
    CameraRig SlowDriftRig();

    CameraRig OrbitRig();

    CameraRig BreathingRig();

    CameraRig ChaosRig();

    CameraRig PulseZoomRig();

    CameraRig SpiralVortexRig();

    CameraRig ZoomOnlyRig();

    CameraRig OscillatingOrbitRig();

    CameraRig PendulumDriftRig();

    CameraRig RandomPopRig();

    CameraRig EllipticalOrbitRig();

    CameraRig VortexSwirlRig();

    CameraRig SlowSpiralRig();

    CameraRig HeartbeatRig();

    CameraRig WaveDriftRig();

    CameraRig RandomDriftRig();
} // namespace LEDSegments

#endif // LED_SEGMENTS_CAMERA_RIG_PRESETS_H
