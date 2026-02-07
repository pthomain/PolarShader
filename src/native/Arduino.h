//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#ifndef POLAR_SHADER_NATIVE_ARDUINO_H
#define POLAR_SHADER_NATIVE_ARDUINO_H

#include <cstdint>
#include <algorithm>
#include <iostream>

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

class SerialMock {
public:
    void print(const char* msg) {
        std::cout << msg;
    }
    void println(const char* msg) {
        std::cout << msg << std::endl;
    }
};

static SerialMock Serial;

#endif // POLAR_SHADER_NATIVE_ARDUINO_H
