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

#pragma once

namespace PolarShader {
    template<typename Rep, typename Tag>
    class Strong {
    public:
        using rep_type = Rep;

        constexpr Strong() : v_(0) {}
        explicit constexpr Strong(Rep raw) : v_(raw) {}

        constexpr Rep raw() const { return v_; }

        friend constexpr bool operator==(Strong a, Strong b) { return a.v_ == b.v_; }
        friend constexpr bool operator!=(Strong a, Strong b) { return a.v_ != b.v_; }
        friend constexpr bool operator<(Strong a, Strong b) { return a.v_ < b.v_; }
        friend constexpr bool operator>(Strong a, Strong b) { return a.v_ > b.v_; }
        friend constexpr bool operator<=(Strong a, Strong b) { return a.v_ <= b.v_; }
        friend constexpr bool operator>=(Strong a, Strong b) { return a.v_ >= b.v_; }

    private:
        Rep v_;
    };

    inline constexpr int32_t raw(int32_t v) { return v; }
    inline constexpr uint32_t raw(uint32_t v) { return v; }
    inline constexpr uint16_t raw(uint16_t v) { return v; }
    inline constexpr uint8_t raw(uint8_t v) { return v; }

    template<typename T>
    class MappedValue {
    public:
        constexpr MappedValue() : value_() {}
        explicit constexpr MappedValue(T value) : value_(value) {}

        constexpr T get() const { return value_; }

    private:
        T value_;
    };
} // namespace PolarShader::Units
