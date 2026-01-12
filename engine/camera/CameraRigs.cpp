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

#include "CameraRigs.h"

namespace LEDSegments {
    CameraRig ZoomOnlyRig() {
        return CameraRig(
            LinearSignal(32768, Waveforms::Constant(500)), // X fixed
            LinearSignal(32768, Waveforms::Constant(500)), // Y fixed
            BoundedSignal(
                0,
                Waveforms::Sine(Waveforms::ConstantWaveform(1000), Waveforms::ConstantWaveform(512)),
                950,
                -1024, // zoom out min
                1024 // zoom in max
            ),
            AngularSignal(0, Waveforms::Constant(0)), // no rotation
            BoundedSignal(0, Waveforms::Constant(0), 950, -32768, 32767) // no vortex
        );
    }

    CameraRig OscillatingOrbitRig() {
        return CameraRig(
            LinearSignal(32768, Waveforms::Sine(Waveforms::ConstantWaveform(1500),
                                                Waveforms::ConstantWaveform(5000))),
            LinearSignal(32768, Waveforms::Sine(Waveforms::ConstantWaveform(1500),
                                                Waveforms::ConstantWaveform(3000))),
            BoundedSignal(0, Waveforms::Sine(Waveforms::ConstantWaveform(800), Waveforms::ConstantWaveform(128)),
                          950, -256, 256),
            AngularSignal(0, Waveforms::Constant(1024)), // slow rotation
            BoundedSignal(0, Waveforms::Constant(0), 950, -32768, 32767)
        );
    }

    CameraRig EllipticalOrbitRig() {
        return CameraRig(
            LinearSignal(32768, Waveforms::Sine(Waveforms::ConstantWaveform(1500),
                                                Waveforms::ConstantWaveform(7000))), // X moves faster
            LinearSignal(32768, Waveforms::Sine(Waveforms::ConstantWaveform(1000),
                                                Waveforms::ConstantWaveform(3000))), // Y slower
            BoundedSignal(0, Waveforms::Constant(0), 950, -256, 256), // no zoom
            AngularSignal(0, Waveforms::Constant(1024)), // slow rotation
            BoundedSignal(0, Waveforms::Constant(0), 950, -32768, 32767)
        );
    }

    CameraRig PendulumDriftRig() {
        return CameraRig(
            LinearSignal(32768, Waveforms::Sine(Waveforms::ConstantWaveform(1000),
                                                Waveforms::ConstantWaveform(3000))), // X swings
            LinearSignal(32768, Waveforms::Constant(0)), // Y fixed
            BoundedSignal(0, Waveforms::Constant(0), 950, -256, 256), // Zoom fixed
            AngularSignal(0, Waveforms::Constant(0)), // No rotation
            BoundedSignal(0, Waveforms::Constant(0), 950, -32768, 32767) // No vortex
        );
    }

    CameraRig PulseZoomRig() {
        // Zoom pulses with a sine wave while mostly stationary
        return CameraRig(
            LinearSignal(32768, Waveforms::Constant(0)),
            LinearSignal(32768, Waveforms::Constant(0)),
            BoundedSignal(
                0,
                Waveforms::Sine(Waveforms::ConstantWaveform(500), Waveforms::ConstantWaveform(512)),
                950,
                -1024,
                1024
            ),
            AngularSignal(0, Waveforms::Constant(0)), // No rotation
            BoundedSignal(0, Waveforms::Constant(0), 950, -32768, 32767) // No vortex
        );
    }
}
