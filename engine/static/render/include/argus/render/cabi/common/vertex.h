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

typedef struct ArgusVertex2d {
    argus_vector_2f_t position;
    argus_vector_2f_t normal;
    argus_vector_4f_t color;
    argus_vector_2f_t tex_coord;
} ArgusVertex2d;

typedef struct ArgusVertex3d {
    argus_vector_3f_t position;
    argus_vector_3f_t normal;
    argus_vector_4f_t color;
    argus_vector_2f_t tex_coord;
} ArgusVertex3d;

#ifdef __cplusplus
}
#endif
