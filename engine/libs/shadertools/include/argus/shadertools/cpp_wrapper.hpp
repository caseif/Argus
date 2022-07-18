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
    std::unordered_map<EShLanguage, std::vector<uint8_t>> process_glsl(
            std::map<EShLanguage, std::string> glsl_sources,
            glslang::EShClient client, glslang::EShTargetClientVersion client_version,
            glslang::EShTargetLanguageVersion spirv_version) {
        auto shaders_count = static_cast<uint8_t>(glsl_sources.size());

        std::vector<glslang_stage_t> stages;
        stages.resize(glsl_sources.size());

        const char **glsl_sources_c = new const char*[shaders_count];
        unsigned int i = 0;
        for (auto &source_tuple : glsl_sources) {
            stages[i] = static_cast<glslang_stage_t>(source_tuple.first);
            glsl_sources_c[i] = source_tuple.second.c_str();
            i++;
        }
        
        auto *res = ::transpile_glsl(stages.data(), glsl_sources_c, shaders_count,
            static_cast<glslang_client_t>(client), static_cast<glslang_target_client_version_t>(client_version),
            static_cast<glslang_target_language_version_t>(spirv_version));
        if (!res->success) {
            free_compilation_result(res);

            throw std::runtime_error("Failed to compile GLSL");
        }

        delete[] glsl_sources_c;

        std::unordered_map<EShLanguage, std::vector<uint8_t>> spirv_shaders;

        for (size_t i = 0; i < res->shader_count; i++) {
            auto stage = res->stages[i];
            auto len = res->spirv_binaries[i]->size;
            auto data = res->spirv_binaries[i]->data;
            std::vector<uint8_t> shader_vec;
            shader_vec.resize(len);
            memcpy(shader_vec.data(), reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(data)), len);

            spirv_shaders.insert({ static_cast<EShLanguage>(stage), shader_vec });
        }

        free_compilation_result(res);

        return spirv_shaders;
    }
}
