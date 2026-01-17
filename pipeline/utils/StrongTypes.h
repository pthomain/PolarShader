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

#pragma once

namespace LEDSegments {
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

    private:
        Rep v_;
    };
} // namespace LEDSegments::Units
