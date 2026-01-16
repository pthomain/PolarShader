//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#include "MirrorTransform.h"
#include <cstdint>

namespace LEDSegments {

    MirrorTransform::MirrorTransform(bool mirrorX, bool mirrorY)
        : mirrorX(mirrorX), mirrorY(mirrorY) {}

    CartesianLayer MirrorTransform::operator()(const CartesianLayer &layer) const {
        return [mirrorX = this->mirrorX, mirrorY = this->mirrorY, layer](int32_t x, int32_t y, Units::TimeMillis t) {
            auto abs_i32 = [](int32_t v) -> int32_t {
                if (v == INT32_MIN) return INT32_MAX; // saturate to avoid UB on negation
                return v < 0 ? -v : v;
            };
            int32_t mx = mirrorX && x < 0 ? abs_i32(x) : x;
            int32_t my = mirrorY && y < 0 ? abs_i32(y) : y;
            return layer(mx, my, t);
        };
    }
}
