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

#include "argus/wm/cabi/window.h"

#include "argus/lowlevel/cabi/math/vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ArgusMouseButton {
    MOUSE_BUTTON_PRIMARY = 1,
    MOUSE_BUTTON_SECONDARY = 2,
    MOUSE_BUTTON_MIDDLE = 3,
    MOUSE_BUTTON_BACK = 4,
    MOUSE_BUTTON_FORWARD = 5,
} ArgusMouseButton;

typedef enum ArgusMouseAxis {
    MOUSE_AXIS_HORIZONTAL,
    MOUSE_AXIS_VERTICAL,
} ArgusMouseAxis;

argus_vector_2d_t argus_mouse_delta(void);

argus_vector_2d_t argus_mouse_pos(void);

double argus_get_mouse_axis(ArgusMouseAxis axis);

double argus_get_mouse_axis_delta(ArgusMouseAxis axis);

bool argus_is_mouse_button_pressed(ArgusMouseButton button);

#ifdef __cplusplus
}
#endif
