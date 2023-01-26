/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include <string>

namespace argus {
    #pragma pack(push, 1)

    struct Matrix4Row {
        float data[4]{};

        float &operator[](int i) {
            return data[i];
        }

        float operator[](int i) const {
            return data[i];
        }

        Matrix4Row(float a, float b, float c, float d) {
            data[0] = a;
            data[1] = b;
            data[2] = c;
            data[3] = d;
        }

        Matrix4Row(void) : Matrix4Row(0, 0, 0, 0) {
        }
    };

    struct Matrix4 {
        union {
            float data[16]{};
            Matrix4Row rows[4];
        };

        Matrix4(Matrix4Row a, Matrix4Row b, Matrix4Row c, Matrix4Row d) {
            rows[0] = a;
            rows[1] = b;
            rows[2] = c;
            rows[3] = d;
        }

        Matrix4(void) : Matrix4({}, {}, {}, {}) {
        }

        Matrix4Row &operator[](int i) {
            return rows[i];
        }

        const Matrix4Row &operator[](int i) const {
            return rows[i];
        }

        static Matrix4 identity() {
            return {
                    {1, 0, 0, 0},
                    {0, 1, 0, 0},
                    {0, 0, 1, 0},
                    {0, 0, 0, 1}
            };
        }
    };

    #pragma pack(pop)

    typedef float mat4_flat_t[16];

    std::string mat4_to_str(Matrix4 matrix);

    std::string mat4_to_str(mat4_flat_t matrix);

    void multiply_matrices(const Matrix4 &a, const Matrix4 &b, Matrix4 &res);

    void multiply_matrices(Matrix4 &a, const Matrix4 &b);

    Vector4f multiply_matrix_and_vector(const Vector4f &vec, const Matrix4 &mat);

    void transpose_matrix(Matrix4 &mat);
}
