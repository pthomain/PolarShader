//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2023 Pierre Thomain

#ifndef LED_SEGMENTS_PIPELINE_SIGNALS_WAVEFORMS_H
#define LED_SEGMENTS_PIPELINE_SIGNALS_WAVEFORMS_H

#include <functional>
#include "../utils/NoiseUtils.h"
#include "../utils/Units.h"

namespace LEDSegments {
    /// Low-level waveform builders used by signals to derive acceleration and phase over time.
    /// All functions return fixed-point Q16.16 values and are safe on MCU targets.
    namespace Waveforms {
        /**
         * @brief A function that takes a phase (Q16.16) and returns an acceleration value (Q16.16).
         * Use this to shape how fast a signal speeds up/slows down across its phase.
         */
        using AccelerationWaveform = std::function<Units::SignalQ16_16(Units::PhaseTurnsUQ16_16)>;

        /**
         * @brief A function that takes a phase (Q16.16) and returns a phase velocity (AngleTurns16/sec Q16.16).
         * Governs how quickly phase advances; 1.0 == 1 angle unit per second (1/65536 of a full wrap).
         */
        using PhaseVelocityWaveform = std::function<Units::PhaseVelAngleUnitsQ16_16(Units::PhaseTurnsUQ16_16)>; // Q16.16-scaled AngleTurns16 per second

        /// Constant acceleration: use when you want a steady push (linear ramps).
        /// Acceleration is specified as the integer part of a Q16.16 value (value << 16).
        inline AccelerationWaveform ConstantAccelerationWaveform(int32_t value) {
            return [val_q16 = Units::SignalQ16_16(value)](Units::PhaseTurnsUQ16_16) { return val_q16; };
        }

        /// Constant acceleration (raw Q16.16): use when you want a fractional push or already have a raw Q16.16 literal.
        inline AccelerationWaveform ConstantAccelerationWaveformRaw(int32_t rawValue) {
            return [val_q16 = Units::SignalQ16_16::fromRaw(rawValue)](Units::PhaseTurnsUQ16_16) { return val_q16; };
        }

        /// Constant phase velocity: sets how quickly the waveform phase advances.
        /// Units are AngleTurns16/sec scaled Q16.16 (1.0 == 1/65536 revolution per second).
        /// Use to make sine/pulse/noise evolve at a fixed speed.
        inline PhaseVelocityWaveform ConstantPhaseVelocityWaveform(int32_t value) {
            return [val_q16 = Units::PhaseVelAngleUnitsQ16_16(value)](Units::PhaseTurnsUQ16_16) { return val_q16; };
        }
    }
}

#endif //LED_SEGMENTS_PIPELINE_SIGNALS_WAVEFORMS_H
