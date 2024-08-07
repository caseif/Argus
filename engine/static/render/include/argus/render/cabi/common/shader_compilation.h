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

#include "argus/render/cabi/common/shader.h"

#include "glslang/Include/glslang_c_interface.h"

void argus_compile_glsl_to_spirv(const argus_shader_const_t *glsl_sources, size_t glsl_sources_count, glslang_client_t client,
        glslang_target_client_version_t client_version, glslang_target_language_version_t spirv_version,
        argus_shader_t *out_shaders, argus_shader_refl_info_t *out_refl);

#ifdef __cplusplus
}
#endif
