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

#include <stddef.h>

typedef void *argus_material_t;

size_t argus_material_len(void);

argus_material_t argus_material_new(const char *texture_uid, size_t shader_uids_count, const char *const *shader_uids);

void argus_material_delete(argus_material_t material);

size_t argus_material_get_ffi_size(void);

const char *argus_material_get_texture_uid(argus_material_t material);

size_t argus_material_get_shader_uids_count(argus_material_t material);

void argus_material_get_shader_uids(argus_material_t material, const char **out_uids, size_t count);

#ifdef __cplusplus
}
#endif
