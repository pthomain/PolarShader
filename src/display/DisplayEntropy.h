//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#ifndef POLAR_SHADER_DISPLAY_ENTROPY_H
#define POLAR_SHADER_DISPLAY_ENTROPY_H

#include <cstddef>
#include <cstdint>

#include "FastLED.h"

namespace PolarShader::DisplayEntropy {
    inline constexpr uint8_t kXiaoFloatingPins[] = {2, 3, 4, 5, 6, 7, 8, 9, 10};
    inline constexpr uint8_t kTeensySmartMatrixFloatingPins[] = {14, 15, 16, 17, 18, 19, 20, 21};

    inline void addFloatingPinEntropy(const uint8_t *pins, uint8_t pinCount, int excludedPin = -1) {
        static bool seedSet = false;
        uint8_t usableCount = 0;

        for (uint8_t i = 0; i < pinCount; ++i) {
            if (pins[i] != excludedPin) {
                ++usableCount;
            }
        }

        if (usableCount == 0) {
            return;
        }

        if (!seedSet) {
            for (uint8_t i = 0; i < pinCount; ++i) {
                if (pins[i] != excludedPin) {
                    pinMode(pins[i], INPUT);
                }
            }
        }

        uint16_t entropy = 0;
        uint8_t cursor = 0;
        for (uint8_t i = 0; i < 16; ++i) {
            uint8_t pin = pins[cursor % pinCount];
            ++cursor;
            while (pin == excludedPin) {
                pin = pins[cursor % pinCount];
                ++cursor;
            }
            entropy = static_cast<uint16_t>((entropy << 1) | (analogRead(pin) & 1));
        }

        if (seedSet) {
            random16_add_entropy(entropy);
        } else {
            random16_set_seed(entropy);
            seedSet = true;
        }
    }

    template<std::size_t N>
    inline void addFloatingPinEntropy(const uint8_t (&pins)[N], int excludedPin = -1) {
        addFloatingPinEntropy(pins, static_cast<uint8_t>(N), excludedPin);
    }
}

#endif // POLAR_SHADER_DISPLAY_ENTROPY_H
