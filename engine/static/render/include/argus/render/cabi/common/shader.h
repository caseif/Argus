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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const char *const k_shader_type_glsl = "glsl";
static const char *const k_shader_type_spirv = "spirv";

typedef void *argus_shader_t;
typedef const void *argus_shader_const_t;

typedef void *argus_shader_refl_info_t;
typedef const void *argus_shader_refl_info_const_t;

typedef enum ArgusShaderStage {
    ARGUS_SHADER_STAGE_VERTEX = 0x01,
    ARGUS_SHADER_STAGE_FRAGMENT = 0x02,
} ArgusShaderStage;

void argus_shader_refl_info_delete(argus_shader_refl_info_t refl);

bool argus_shader_refl_info_has_attr(argus_shader_refl_info_const_t refl, const char *name);

uint32_t argus_shader_refl_info_get_attr_loc(argus_shader_refl_info_const_t refl, const char *name, bool *out_found);

void argus_shader_refl_info_set_attr_loc(argus_shader_refl_info_t refl, const char *name, uint32_t loc);

bool argus_shader_refl_info_has_output(argus_shader_refl_info_const_t refl, const char *name);

uint32_t argus_shader_refl_info_get_output_loc(argus_shader_refl_info_const_t refl, const char *name, bool *out_found);

void argus_shader_refl_info_set_output_loc(argus_shader_refl_info_t refl, const char *name, uint32_t loc);

bool argus_shader_refl_info_has_uniform_var(argus_shader_refl_info_const_t refl, const char *name);

bool argus_shader_refl_info_has_uniform(argus_shader_refl_info_const_t refl, const char *ubo, const char *name);

uint32_t argus_shader_refl_info_get_uniform_var_loc(argus_shader_refl_info_const_t refl, const char *name,
        bool *out_found);

uint32_t argus_shader_refl_info_get_uniform_loc(argus_shader_refl_info_const_t refl, const char *ubo, const char *name,
        bool *out_found);

void argus_shader_refl_info_set_uniform_var_loc(argus_shader_refl_info_t refl, const char *name, uint32_t loc);

void argus_shader_refl_info_set_uniform_loc(argus_shader_refl_info_t refl, const char *ubo, const char *name,
        uint32_t loc);

bool argus_shader_refl_info_has_ubo(argus_shader_refl_info_const_t refl, const char *name);

uint32_t argus_shader_refl_info_get_ubo_binding(argus_shader_refl_info_const_t refl, const char *name, bool *out_found);

void argus_shader_refl_info_set_ubo_binding(argus_shader_refl_info_t refl, const char *name, uint32_t binding);

const char *argus_shader_refl_info_get_ubo_instance_name(argus_shader_refl_info_const_t refl, const char *name,
        bool *out_found);

void argus_shader_refl_info_set_ubo_instance_name(argus_shader_refl_info_t refl, const char *ubo_name,
        const char *instance_name);

argus_shader_t argus_shader_new(const char *uid, const char *type, ArgusShaderStage stage,
        const unsigned char *src, size_t src_len);

argus_shader_t argus_shader_copy(argus_shader_const_t shader);

void argus_shader_delete(argus_shader_t shader);

const char *argus_shader_get_uid(argus_shader_const_t shader);

const char *argus_shader_get_type(argus_shader_const_t shader);

ArgusShaderStage argus_shader_get_stage(argus_shader_const_t shader);

void argus_shader_get_source(argus_shader_const_t shader, const unsigned char **out_ptr, size_t *out_len);

#ifdef __cplusplus
}
#endif
