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

#include "argus/shadertools/c_shim.h"

#include "glslang/Public/ShaderLang.h"

#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <cstring>

namespace argus {
    struct CompiledShaderSet {
        std::unordered_map<EShLanguage, std::vector<uint8_t>> spirv_shaders;
        std::map<std::string, uint32_t> attributes;
        std::map<std::string, uint32_t> outputs;
        std::map<std::string, uint32_t> uniforms;
        std::map<std::string, uint32_t> buffers;
    };

    static void _copy_compat_map_to_cpp_map(std::map<std::string, uint32_t> &dest, unsigned char *source,
            size_t count) {
        if (source == nullptr) {
            // just leave the destination map empty
            return;
        }

        size_t off = sizeof(size_t); // first bytes are used to store total size of allocated block
        for (size_t i = 0; i < count; i++) {
            auto compat_struct = reinterpret_cast<SizedByteArrayWithIndex *>(source + off);
            auto index = compat_struct->index;
            std::string name(reinterpret_cast<const char *>(compat_struct->data), compat_struct->size);
            dest.insert({name, index});

            off += sizeof(SizedByteArrayWithIndex) + compat_struct->size;
        }
    }

    CompiledShaderSet process_glsl(
            std::map<EShLanguage, std::string> glsl_sources,
            glslang::EShClient client, glslang::EShTargetClientVersion client_version,
            glslang::EShTargetLanguageVersion spirv_version) {
        auto shaders_count = static_cast<uint8_t>(glsl_sources.size());

        std::vector<glslang_stage_t> stages;
        stages.resize(glsl_sources.size());

        const char **glsl_sources_c = new const char *[shaders_count];
        unsigned int i = 0;
        for (auto &source_tuple : glsl_sources) {
            stages[i] = glslang_stage_t(source_tuple.first);
            glsl_sources_c[i] = source_tuple.second.c_str();
            i++;
        }

        auto *res = ::transpile_glsl(stages.data(), glsl_sources_c, shaders_count,
                glslang_client_t(client), glslang_target_client_version_t(client_version),
                glslang_target_language_version_t(spirv_version));
        if (!res->success) {
            free_compilation_result(res);

            throw std::runtime_error("Failed to compile GLSL");
        }

        delete[] glsl_sources_c;

        CompiledShaderSet final_set;

        for (size_t i = 0; i < res->shader_count; i++) {
            auto stage = res->stages[i];
            auto len = res->spirv_binaries[i]->size;
            auto data = res->spirv_binaries[i]->data;
            std::vector<uint8_t> shader_vec;
            shader_vec.resize(len);
            memcpy(shader_vec.data(), reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(data)), len);

            final_set.spirv_shaders.insert({static_cast<EShLanguage>(stage), shader_vec});
        }

        _copy_compat_map_to_cpp_map(final_set.attributes, res->attribs, res->attrib_count);
        _copy_compat_map_to_cpp_map(final_set.outputs, res->outputs, res->output_count);
        _copy_compat_map_to_cpp_map(final_set.uniforms, res->uniforms, res->uniform_count);
        _copy_compat_map_to_cpp_map(final_set.buffers, res->buffers, res->buffer_count);

        free_compilation_result(res);

        return final_set;
    }
}
