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

#ifdef __cplusplus
extern "C" {
#endif

#include "argus/lowlevel/cabi/math/vector.h"

typedef struct ArgusTransform2d {
    argus_vector_2f_t translation;
    argus_vector_2f_t scale;
    float rotation;
} ArgusTransform2d;

typedef struct argus_matrix_4x4_t {
    float cells[16];
} argus_matrix_4x4_t;

argus_matrix_4x4_t argus_transform_2d_as_matrix(const ArgusTransform2d *transform, float anchor_x, float anchor_y);

argus_matrix_4x4_t argus_transform_2d_get_translation_matrix(const ArgusTransform2d *transform);

argus_matrix_4x4_t argus_transform_2d_get_rotation_matrix(const ArgusTransform2d *transform);

argus_matrix_4x4_t argus_transform_2d_get_scale_matrix(const ArgusTransform2d *transform);

ArgusTransform2d argus_transform_2d_inverse(const ArgusTransform2d *transform);

#ifdef __cplusplus
}
#endif
