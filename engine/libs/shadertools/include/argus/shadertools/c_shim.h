/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include "glslang/Include/glslang_c_interface.h"

#include <cstddef>
#include <cstdint>

extern "C" {
struct SizedByteArray {
    size_t size;
    const uint8_t data[0];
};

struct SizedByteArrayWithIndex {
    size_t size;
    size_t index;
    const uint8_t data[0];
};

struct InteropShaderCompilationResult {
    bool success;
    size_t shader_count;
    const glslang_stage_t *stages;
    const SizedByteArray *const *spirv_binaries;
    size_t attrib_count;
    unsigned char *attribs;
    size_t output_count;
    unsigned char *outputs;
    size_t uniform_count;
    unsigned char *uniforms;
    size_t buffer_count;
    unsigned char *buffers;
    size_t ubo_count;
    unsigned char *ubo_bindings;
    unsigned char *ubo_names;
};

InteropShaderCompilationResult *transpile_glsl(const glslang_stage_t *stages, const char *const *glsl_sources,
        size_t count, glslang_client_t client, glslang_target_client_version_t client_version,
        glslang_target_language_version_t spirv_version);


void free_compilation_result(InteropShaderCompilationResult *result);
}
