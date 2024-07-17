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

#include "argus/lowlevel/cabi/math/vector.h"
#include "argus/lowlevel/math/vector.hpp"

#ifdef __is_layout_compatible
static_assert(__is_layout_compatible(argus_vector_2d_t, argus::Vector2d));
static_assert(__is_layout_compatible(argus_vector_3d_t, argus::Vector3d));
static_assert(__is_layout_compatible(argus_vector_4d_t, argus::Vector4d));

static_assert(__is_layout_compatible(argus_vector_2f_t, argus::Vector2f));
static_assert(__is_layout_compatible(argus_vector_3f_t, argus::Vector3f));
static_assert(__is_layout_compatible(argus_vector_4f_t, argus::Vector4f));

static_assert(__is_layout_compatible(argus_vector_2i_t, argus::Vector2i));
static_assert(__is_layout_compatible(argus_vector_3i_t, argus::Vector3i));
static_assert(__is_layout_compatible(argus_vector_4i_t, argus::Vector4i));

static_assert(__is_layout_compatible(argus_vector_2u_t, argus::Vector2u));
static_assert(__is_layout_compatible(argus_vector_3u_t, argus::Vector3u));
static_assert(__is_layout_compatible(argus_vector_4u_t, argus::Vector4u));
#endif
