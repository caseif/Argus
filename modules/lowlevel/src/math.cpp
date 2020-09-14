/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/math.hpp"

void multiply_matrices(float (&a)[16], float (&b)[16], float (&res)[16]) {
    // naive implementation
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            int index = j * 4 + i;
            res[index] = 0;
            for (int k = 0; k < 4; k++) {
                res[index] += a[k * 4 + i] * b[j * 4 + k];
            }
        }
    }
}
