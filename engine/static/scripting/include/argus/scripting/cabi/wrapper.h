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

#include "argus/scripting/cabi/types.h"

#ifdef __cplusplus
extern "C" {
#endif

ArgusObjectWrapperOrReflectiveArgsError argus_create_object_wrapper(argus_object_type_const_t ty, void *ptr,
        size_t size);

void argus_copy_bound_type(const char *type_id, void *dst, const void *src);

void argus_move_bound_type(const char *type_id, void *dst, void *src);

#ifdef __cplusplus
}
#endif
