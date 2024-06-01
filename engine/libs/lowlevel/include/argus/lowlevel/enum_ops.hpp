/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <type_traits>

namespace argus::enum_ops {
    template<typename T, typename U = std::underlying_type_t<T>>
    std::enable_if_t<std::is_enum_v<T>, T> operator&(T a, T b) {
        return T(U(a) & U(b));
    }

    template<typename T, typename U = std::underlying_type_t<T>>
    std::enable_if_t<std::is_enum_v<T>, T> operator&(T a, U b) {
        return T(U(a) & b);
    }

    template<typename T, typename U = std::underlying_type_t<T>>
    std::enable_if_t<std::is_enum_v<T>, T &> operator&=(T &a, T b) {
        return (a = T(U(a) & U(b)));
    }

    template<typename T, typename U = std::underlying_type_t<T>>
    std::enable_if_t<std::is_enum_v<T>, T &> operator&=(T &a, U b) {
        return (a = T(U(a) & b));
    }

    template<typename T, typename U = std::underlying_type_t<T>>
    std::enable_if_t<std::is_enum_v<T>, T> operator|(T a, T b) {
        return T(U(a) | U(b));
    }

    template<typename T, typename U = std::underlying_type_t<T>>
    std::enable_if_t<std::is_enum_v<T>, T> operator|(T a, U b) {
        return T(U(a) | b);
    }

    template<typename T, typename U = std::underlying_type_t<T>>
    std::enable_if_t<std::is_enum_v<T>, T &> operator|=(T &a, T b) {
        return (a = T(U(a) | U(b)));
    }

    template<typename T, typename U = std::underlying_type_t<T>>
    std::enable_if_t<std::is_enum_v<T>, T &> operator|=(T &a, U b) {
        return (a = T(U(a) | b));
    }

    template<typename T, typename U = std::underlying_type_t<T>>
    std::enable_if_t<std::is_enum_v<T>, T> operator^(T a, T b) {
        return T(U(a) ^ U(b));
    }

    template<typename T, typename U = std::underlying_type_t<T>>
    std::enable_if_t<std::is_enum_v<T>, T> operator^(T a, U b) {
        return T(U(a) ^ b);
    }

    template<typename T, typename U = std::underlying_type_t<T>>
    std::enable_if_t<std::is_enum_v<T>, T &> operator^=(T &a, T b) {
        return (a = T(U(a) ^ U(b)));
    }

    template<typename T, typename U = std::underlying_type_t<T>>
    std::enable_if_t<std::is_enum_v<T>, T &> operator^=(T &a, U b) {
        return (a = T(U(a) ^ b));
    }

    template<typename T, typename U = std::underlying_type_t<T>>
    std::enable_if_t<std::is_enum_v<T>, T> operator~(T a) {
        return T(~U(a));
    }

    template<typename T, typename U = std::underlying_type_t<T>>
    std::enable_if_t<std::is_enum_v<T>, bool> operator==(T a, U b) {
        return U(a) == b;
    }

    template<typename T, typename U = std::underlying_type_t<T>>
    std::enable_if_t<std::is_enum_v<T>, bool> operator==(U a, T b) {
        return a == U(b);
    }

    template<typename T, typename U = std::underlying_type_t<T>>
    std::enable_if_t<std::is_enum_v<T>, bool> operator!=(T a, U b) {
        return U(a) != b;
    }

    template<typename T, typename U = std::underlying_type_t<T>>
    std::enable_if_t<std::is_enum_v<T>, bool> operator!=(U a, T b) {
        return a != U(b);
    }
}
