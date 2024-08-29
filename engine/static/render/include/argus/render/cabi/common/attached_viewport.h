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

#include "argus/render/cabi/common/scene.h"
#include "argus/render/cabi/common/viewport.h"

#include <stddef.h>
#include <stdint.h>

typedef void *argus_attached_viewport_t;
typedef const void *argus_attached_viewport_const_t;

ArgusSceneType argus_attached_viewport_get_type(argus_attached_viewport_const_t viewport);

uint32_t argus_attached_viewport_get_id(argus_attached_viewport_const_t viewport);

ArgusViewport argus_attached_viewport_get_viewport(argus_attached_viewport_const_t viewport);

uint32_t argus_attached_viewport_get_z_index(argus_attached_viewport_const_t viewport);

size_t argus_attached_viewport_get_postprocessing_shaders_count(argus_attached_viewport_const_t viewport);

void argus_attached_viewport_get_postprocessing_shaders(argus_attached_viewport_const_t viewport,
        const char **dest, size_t count);

void argus_attached_viewport_add_postprocessing_shader(argus_attached_viewport_t viewport, const char *shader_uid);

void argus_attached_viewport_remove_postprocessing_shader(argus_attached_viewport_t viewport, const char *shader_uid);

#ifdef __cplusplus
}
#endif
