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

#include "argus/lowlevel/math/matrix.hpp"
#include "argus/lowlevel/math/vector.hpp"

#include <array>
#include <string>

#include <cstring>

namespace argus {
    static inline void _swap_f(float *a, float *b) {
        float temp = *a;
        *a = *b;
        *b = temp;
    }

    Matrix4 Matrix4::from_row_major(const float elements[16]) {
        return { {
            elements[0], elements[4], elements[8],  elements[12],
            elements[1], elements[5], elements[9],  elements[13],
            elements[2], elements[6], elements[10], elements[14],
            elements[3], elements[7], elements[11], elements[15]
        } };
    }

    Matrix4 Matrix4::from_row_major(const std::array<float, 16> &&elements) {
        return from_row_major(elements.data());
    }

    float Matrix4::operator()(int r, int c) const {
        return this->data[c * 4 + r];
    }

    float &Matrix4::operator()(int r, int c) {
        return this->data[c * 4 + r];
    }

    std::string Matrix4::to_string(void) {
        std::string text;
        text.reserve(36 * 6);

        text += "-----------------------------------\n";
        for (int r = 0; r < 4; r++) {
            text += "| ";
            for (int c = 0; c < 4; c++) {
                std::string el_str(9, '\0');
                snprintf(el_str.data(), el_str.size(), "%7.2f ", (*this)(r, c));
                text += el_str.substr(0, 8);
            }
            text += "|\n";
        }
        text += "-----------------------------------";

        return text;
    }

    Matrix4 operator*(const Matrix4 &a, const Matrix4 &b) {
        Matrix4 res;

        // naive implementation
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                res(i, j) = 0;
                for (int k = 0; k < 4; k++) {
                    res(i, j) += a(i, k) * b(k, j);
                }
            }
        }

        return res;
    }

    Matrix4 &operator*=(Matrix4 &a, const Matrix4 &b) {
        Matrix4 res = a * b;
        std::memcpy(a.data, res.data, sizeof(a.data));
        return a;
    }

    Vector4f operator*(const Matrix4 &mat, const Vector4f &vec) {
        return Vector4f{
                mat(0, 0) * vec.x + mat(0, 1) * vec.y + mat(0, 2) * vec.z + mat(0, 3) * vec.w,
                mat(1, 0) * vec.x + mat(1, 1) * vec.y + mat(1, 2) * vec.z + mat(1, 3) * vec.w,
                mat(2, 0) * vec.x + mat(2, 1) * vec.y + mat(2, 2) * vec.z + mat(2, 3) * vec.w,
                mat(3, 0) * vec.x + mat(3, 1) * vec.y + mat(3, 2) * vec.z + mat(3, 3) * vec.w
        };
    }

    Vector4f &operator*=(const Matrix4 &mat, Vector4f &vec) {
        vec = mat * vec;
        return vec;
    }

    void Matrix4::transpose() {
        Matrix4 &mat = *this;
        _swap_f(&(mat(0, 1)), &(mat(1, 0)));
        _swap_f(&(mat(0, 2)), &(mat(2, 0)));
        _swap_f(&(mat(0, 3)), &(mat(3, 0)));
        _swap_f(&(mat(1, 2)), &(mat(2, 1)));
        _swap_f(&(mat(1, 3)), &(mat(3, 1)));
        _swap_f(&(mat(2, 3)), &(mat(3, 2)));
    }
}
