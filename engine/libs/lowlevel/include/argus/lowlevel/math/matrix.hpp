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

#include "argus/lowlevel/math/vector.hpp"

#include <array>
#include <string>

#include <cstring>

namespace argus {
    #pragma pack(push, 1)

    static constexpr const float IDENTITY[16] = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
    };

    struct Matrix4 {
        static Matrix4 from_row_major(const float elements[16]);

        static Matrix4 from_row_major(const std::array<float, 16> &&elements);

        static Matrix4 identity() {
            return { IDENTITY };
        }

        float data[16] {};

        Matrix4(const float elements[16]) {
            if (elements != nullptr) {
                memcpy(data, elements, sizeof(data));
            } else {
                memset(data, 0, sizeof(data));
            }
        }

        Matrix4(const std::array<float, 16> elements) {
            memcpy(data, elements.data(), sizeof(data));
        }

        Matrix4(void) = default;

        float operator()(int r, int c) const;

        float &operator()(int r, int c);

        std::string to_string(void);

        void transpose(void);
    };

    #pragma pack(pop)

    Matrix4 operator*(const Matrix4 &a, const Matrix4 &b);

    Matrix4 &operator*=(Matrix4 &a, const Matrix4 &b);

    Vector4f operator*(const Matrix4 &mat, const Vector4f &vec);
}
