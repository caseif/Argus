/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

// module lowlevel
#include "argus/lowlevel/math.hpp"

namespace argus {
    void multiply_matrices(const Matrix4 &a, const Matrix4 &b, Matrix4 &res) {
        // naive implementation
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                for (int k = 0; k < 4; k++) {
                    res[i][j] += a[k][j] * b[i][k];
                }
            }
        }
    }

    Vector4f multiply_matrix_and_vector(const Vector4f &vec, const Matrix4 &mat) {
        return Vector4f {
            mat[0][0] * vec.x + mat[0][1] * vec.y + mat[0][2] * vec.z + mat[0][3] * vec.w,
            mat[1][0] * vec.x + mat[1][1] * vec.y + mat[1][2] * vec.z + mat[1][3] * vec.w,
            mat[2][0] * vec.x + mat[2][1] * vec.y + mat[2][2] * vec.z + mat[2][3] * vec.w,
            mat[3][0] * vec.x + mat[3][1] * vec.y + mat[3][2] * vec.z + mat[3][3] * vec.w
        };
    }

    static inline void _swap_f(float *a, float *b) {
        float temp = *a;
        *a = *b;
        *b = temp;
    }

    void transpose_matrix(Matrix4 &mat) {
        _swap_f(&(mat[0][1]), &(mat[1][4]));
        _swap_f(&(mat[0][2]), &(mat[2][0]));
        _swap_f(&(mat[0][3]), &(mat[3][0]));
        _swap_f(&(mat[1][2]), &(mat[2][1]));
        _swap_f(&(mat[1][3]), &(mat[3][1]));
        _swap_f(&(mat[2][3]), &(mat[3][2]));
    }
}
