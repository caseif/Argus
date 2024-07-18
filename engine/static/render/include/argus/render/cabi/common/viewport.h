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

typedef enum ViewportCoordinateSpaceMode {
    ARGUS_VCSM_INDIVIDUAL,
    ARGUS_VCSM_MIN_AXIS,
    ARGUS_VCSM_MAX_AXIS,
    ARGUS_VCSM_HORIZONTAL_AXIS,
    ARGUS_VCSM_VERTICAL_AXIS,
} ViewportCoordinateSpaceMode;

typedef struct Viewport {
    float top;
    float bottom;
    float left;
    float right;

    argus_vector_2f_t scaling;
    ViewportCoordinateSpaceMode mode;
} Viewport;

#ifdef __cplusplus
}
#endif
