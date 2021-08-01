/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/math.hpp"

namespace argus {
    void multiply_matrices(const mat4_flat_t a, const mat4_flat_t b, mat4_flat_t res) {
        // naive implementation
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                int index = j * 4 + i;
                res[index] = 0;
                for (int k = 0; k < 4; k++) {
                    auto a_index = k * 4 + i;
                    auto b_index = j * 4 + k;

                    res[index] += a[a_index] * b[b_index];
                }
            }
        }
    }

    Vector4f multiply_matrix_and_vector(const Vector4f &vec, const mat4_flat_t mat) {
        return Vector4f {
            { mat[0] * vec.x + mat[4] * vec.y + mat[8] * vec.z + mat[12] * vec.w },
            { mat[1] * vec.x + mat[5] * vec.y + mat[9] * vec.z + mat[13] * vec.w },
            { mat[2] * vec.x + mat[6] * vec.y + mat[10] * vec.z + mat[14] * vec.w },
            { mat[3] * vec.x + mat[7] * vec.y + mat[11] * vec.z + mat[15] * vec.w }
        };
    }

    static inline void _swap_f(float *a, float *b) {
        float temp = *a;
        *a = *b;
        *b = temp;
    }

    void transpose_matrix(mat4_flat_t mat) {
        _swap_f(&(mat[1]), &(mat[4]));
        _swap_f(&(mat[2]), &(mat[8]));
        _swap_f(&(mat[3]), &(mat[12]));
        _swap_f(&(mat[6]), &(mat[9]));
        _swap_f(&(mat[7]), &(mat[13]));
        _swap_f(&(mat[11]), &(mat[14]));
    }
}
