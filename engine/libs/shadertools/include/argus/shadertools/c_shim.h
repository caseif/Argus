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
        const SizedByteArrayWithIndex *const *attrib_names;
        size_t uniform_count;
        const SizedByteArrayWithIndex *const *uniform_names;
    };

    InteropShaderCompilationResult *transpile_glsl(const glslang_stage_t *stages, const char *const *glsl_sources,
        size_t count, glslang_client_t client, glslang_target_client_version_t client_version,
        glslang_target_language_version_t spirv_version);


    void free_compilation_result(InteropShaderCompilationResult *result);
}
