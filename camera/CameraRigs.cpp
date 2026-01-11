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
    CameraRig SlowDriftRig() {
        // Very slow smooth drift in X and Y, small zoom oscillation
        return CameraRig(
            LinearSignal(
                random16(),
                Waveforms::Noise(
                    Waveforms::ConstantWaveform(500), // phase velocity
                    Waveforms::ConstantWaveform(200) // amplitude
                )),
            LinearSignal(
                random16(),
                Waveforms::Noise(
                    Waveforms::ConstantWaveform(500),
                    Waveforms::ConstantWaveform(200)
                )),
            BoundedSignal(
                0,
                Waveforms::Noise(Waveforms::ConstantWaveform(200), Waveforms::ConstantWaveform(10)),
                950,
                -256, // 0.25x zoom
                256 // 2x zoom
            ),
            AngularSignal(
                0,
                Waveforms::Constant(0)
            ), // No rotation
            BoundedSignal(
                0,
                Waveforms::Constant(0),
                950,
                -32768,
                32767
            ) // No vortex
        );
    }

    CameraRig OrbitRig() {
        // Circular orbit with constant rotation
        return CameraRig(
            LinearSignal(
                32768,
                Waveforms::Sine(Waveforms::ConstantWaveform(2000), Waveforms::ConstantWaveform(5000))),
            LinearSignal(
                32768,
                Waveforms::Sine(Waveforms::ConstantWaveform(2000), Waveforms::ConstantWaveform(5000))),
            BoundedSignal(0, Waveforms::Constant(0), 950, -256, 256), // No zoom change
            AngularSignal(0, Waveforms::Constant(1024)), // Slow rotation per frame
            BoundedSignal(0, Waveforms::Constant(0), 950, -32768, 32767) // No vortex
        );
    }

    CameraRig BreathingRig() {
        // Zoom in/out while stationary
        return CameraRig(
            LinearSignal(32768, Waveforms::Constant(0)),
            LinearSignal(32768, Waveforms::Constant(0)),
            BoundedSignal(
                0,
                Waveforms::Sine(Waveforms::ConstantWaveform(1000), Waveforms::ConstantWaveform(128)),
                950,
                -512,
                512
            ),
            AngularSignal(0, Waveforms::Constant(0)), // No rotation
            BoundedSignal(0, Waveforms::Constant(0), 950, -32768, 32767) // No vortex
        );
    }

    CameraRig ChaosRig() {
        // Fast random movements, rotation, and vortex
        return CameraRig(
            LinearSignal(random16(), Waveforms::Noise(
                             Waveforms::ConstantWaveform(8000),
                             Waveforms::ConstantWaveform(2000)
                         )),
            LinearSignal(random16(), Waveforms::Noise(
                             Waveforms::ConstantWaveform(8000),
                             Waveforms::ConstantWaveform(2000)
                         )),
            BoundedSignal(
                0,
                Waveforms::Noise(Waveforms::ConstantWaveform(2000), Waveforms::ConstantWaveform(50)),
                950,
                -1024,
                1024
            ),
            AngularSignal(
                0, Waveforms::Noise(Waveforms::ConstantWaveform(3000), Waveforms::ConstantWaveform(16384))),
            BoundedSignal(
                0,
                Waveforms::Noise(Waveforms::ConstantWaveform(4000), Waveforms::ConstantWaveform(16384)),
                950,
                -32768,
                32767
            )
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

    CameraRig SpiralVortexRig() {
        // Combination of rotation + vortex + slight drift
        return CameraRig(
            LinearSignal(
                random16(), Waveforms::Noise(Waveforms::ConstantWaveform(500), Waveforms::ConstantWaveform(200))),
            LinearSignal(
                random16(), Waveforms::Noise(Waveforms::ConstantWaveform(500), Waveforms::ConstantWaveform(200))),
            BoundedSignal(0, Waveforms::Noise(Waveforms::ConstantWaveform(200), Waveforms::ConstantWaveform(20)),
                          950, -512, 512),
            AngularSignal(0, Waveforms::Constant(1024)), // slow rotation
            BoundedSignal(
                16384, Waveforms::Sine(Waveforms::ConstantWaveform(1000), Waveforms::ConstantWaveform(8192)), 950,
                -32768, 32767) // vortex
        );
    }

    CameraRig ZoomOnlyRig() {
        return CameraRig(
            LinearSignal(32768, Waveforms::Constant(0)), // X fixed
            LinearSignal(32768, Waveforms::Constant(0)), // Y fixed
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

    CameraRig RandomPopRig() {
        return CameraRig(
            LinearSignal(random16(), Waveforms::Noise(Waveforms::ConstantWaveform(10000),
                                                      Waveforms::ConstantWaveform(5000))),
            LinearSignal(random16(), Waveforms::Noise(Waveforms::ConstantWaveform(10000),
                                                      Waveforms::ConstantWaveform(5000))),
            BoundedSignal(0, Waveforms::Noise(Waveforms::ConstantWaveform(8000), Waveforms::ConstantWaveform(256)),
                          950, -1024, 1024),
            AngularSignal(
                0, Waveforms::Noise(Waveforms::ConstantWaveform(4000), Waveforms::ConstantWaveform(16384))),
            BoundedSignal(
                0, Waveforms::Noise(Waveforms::ConstantWaveform(4000), Waveforms::ConstantWaveform(16384)), 950,
                -32768, 32767)
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

    CameraRig VortexSwirlRig() {
        return CameraRig(
            LinearSignal(
                32768, Waveforms::Noise(Waveforms::ConstantWaveform(500), Waveforms::ConstantWaveform(200))),
            LinearSignal(
                32768, Waveforms::Noise(Waveforms::ConstantWaveform(500), Waveforms::ConstantWaveform(200))),
            BoundedSignal(0, Waveforms::Noise(Waveforms::ConstantWaveform(200), Waveforms::ConstantWaveform(50)),
                          950, -512, 512),
            AngularSignal(0, Waveforms::Constant(2048)), // rotation
            BoundedSignal(
                16384, Waveforms::Sine(Waveforms::ConstantWaveform(1000), Waveforms::ConstantWaveform(8192)), 950,
                -32768, 32767) // strong vortex
        );
    }

    CameraRig SlowSpiralRig() {
        return CameraRig(
            LinearSignal(32768, Waveforms::Noise(Waveforms::ConstantWaveform(200),
                                                 Waveforms::ConstantWaveform(1000))), // X slow drift
            LinearSignal(32768, Waveforms::Noise(Waveforms::ConstantWaveform(200),
                                                 Waveforms::ConstantWaveform(1000))), // Y slow drift
            BoundedSignal(
                0,
                Waveforms::Noise(Waveforms::ConstantWaveform(200), Waveforms::ConstantWaveform(50)),
                950,
                -512, 512
            ), // slow zoom
            AngularSignal(0, Waveforms::Constant(1024)), // slow rotation
            BoundedSignal(
                0,
                Waveforms::Sine(Waveforms::ConstantWaveform(500), Waveforms::ConstantWaveform(8192)),
                950,
                -32768,
                32767
            ) // gradually increasing vortex
        );
    }

    CameraRig HeartbeatRig() {
        return CameraRig(
            LinearSignal(32768, Waveforms::Constant(0)), // X fixed
            LinearSignal(32768, Waveforms::Constant(0)), // Y fixed
            BoundedSignal(
                0,
                Waveforms::Pulse(Waveforms::ConstantWaveform(2000), Waveforms::ConstantWaveform(512)),
                950,
                -512, 512
            ), // pulse zoom in/out like heartbeat
            AngularSignal(0, Waveforms::Constant(0)), // no rotation
            BoundedSignal(0, Waveforms::Constant(0), 950, -32768, 32767) // no vortex
        );
    }

    CameraRig WaveDriftRig() {
        return CameraRig(
            LinearSignal(
                32768, Waveforms::Sine(Waveforms::ConstantWaveform(800), Waveforms::ConstantWaveform(4000))),
            // X wave motion
            LinearSignal(
                32768, Waveforms::Sine(Waveforms::ConstantWaveform(600), Waveforms::ConstantWaveform(2000))),
            // Y gentle drift
            BoundedSignal(0, Waveforms::Constant(0), 950, -256, 256), // zoom fixed
            AngularSignal(0, Waveforms::Constant(512)), // slow rotation
            BoundedSignal(0, Waveforms::Constant(0), 950, -32768, 32767) // no vortex
        );
    }

    CameraRig RandomDriftRig() {
        return CameraRig(
            LinearSignal(32768, Waveforms::Noise(Waveforms::ConstantWaveform(300),
                                                 Waveforms::ConstantWaveform(2000))), // X drift
            LinearSignal(32768, Waveforms::Noise(Waveforms::ConstantWaveform(300),
                                                 Waveforms::ConstantWaveform(2000))), // Y drift
            BoundedSignal(
                0,
                Waveforms::Noise(Waveforms::ConstantWaveform(200), Waveforms::ConstantWaveform(128)),
                950,
                -512, 512
            ), // zoom drift
            AngularSignal(0, Waveforms::Noise(Waveforms::ConstantWaveform(200), Waveforms::ConstantWaveform(512))),
            // slow rotation drift
            BoundedSignal(0, Waveforms::Noise(Waveforms::ConstantWaveform(200), Waveforms::ConstantWaveform(1024)),
                          950, -32768, 32767) // gentle vortex
        );
    }
}
