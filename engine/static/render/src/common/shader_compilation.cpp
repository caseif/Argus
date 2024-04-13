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

#include "argus/lowlevel/logging.hpp"

#include "argus/shadertools.hpp"

#include "argus/render/common/shader.hpp"
#include "argus/render/common/shader_compilation.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wdeprecated"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wsuggest-destructor-override"
#endif

#include "glslang/Public/ShaderLang.h"
#ifdef USING_SYSTEM_GLSLANG
#include "glslang/SPIRV/GlslangToSpv.h"
#else
#include "SPIRV/GlslangToSpv.h"
#endif

#pragma GCC diagnostic pop

#include <map>
#include <stdexcept>
#include <vector>

namespace argus {
    std::pair<std::vector<Shader>, ShaderReflectionInfo> compile_glsl_to_spirv(
            const std::vector<Shader> &glsl_shaders, glslang::EShClient client,
            glslang::EShTargetClientVersion client_version, glslang::EShTargetLanguageVersion spirv_version) {
        glslang::InitializeProcess();

        glslang::TProgram program;

        std::map<std::string, const Shader *> glsl_shaders_map;

        std::map<EShLanguage, std::string> shaders_map;
        std::map<EShLanguage, std::string> shader_uids;

        for (const auto &glsl_shader : glsl_shaders) {
            Logger::default_logger().debug("Compiling shader %s to SPIR-V", glsl_shader.get_uid().c_str());

            glsl_shaders_map[glsl_shader.get_uid()] = &glsl_shader;

            auto stage = glsl_shader.get_stage();
            auto src_str = std::string(glsl_shader.get_source().begin(), glsl_shader.get_source().end());

            EShLanguage lang;
            switch (stage) {
                case ShaderStage::Vertex:
                    lang = EShLangVertex;
                    break;
                case ShaderStage::Fragment:
                    lang = EShLangFragment;
                    break;
                default:
                    throw std::invalid_argument("Unsupported shader stage");
            }

            shaders_map[lang] = src_str;
            shader_uids[lang] = glsl_shader.get_uid();
        }

        auto comp_res = process_glsl(shaders_map, client, client_version, spirv_version);

        std::vector<Shader> spirv_shaders;
        spirv_shaders.reserve(comp_res.spirv_shaders.size());

        for (const auto &[lang, spirv_u8] : comp_res.spirv_shaders) {
            auto uid = shader_uids[lang];

            ShaderStage stage;
            switch (lang) {
                case EShLangVertex:
                    stage = ShaderStage::Vertex;
                    break;
                case EShLangFragment:
                    stage = ShaderStage::Fragment;
                    break;
                default:
                    throw std::invalid_argument("Unsupported shader stage");
            }

            spirv_shaders.emplace_back(uid, SHADER_TYPE_SPIR_V, stage, spirv_u8);
        }

        for (const auto &[attr_name, attr_loc] : comp_res.attributes) {
            Logger::default_logger().debug("Found shader program attribute %s @ location %d",
                    attr_name.c_str(), attr_loc);
        }

        for (const auto &[output_name, output_loc] : comp_res.outputs) {
            Logger::default_logger().debug("Found shader program output %s @ location %d",
                    output_name.c_str(), output_loc);
        }

        for (const auto &[uniform_name, uniform_loc] : comp_res.uniforms) {
            Logger::default_logger().debug("Found shader program uniform %s @ location %d",
                    uniform_name.c_str(), uniform_loc);
        }

        for (const auto &[buffer_name, buffer_loc] : comp_res.buffers) {
            Logger::default_logger().debug("Found shader program buffer %s @ location %d",
                    buffer_name.c_str(), buffer_loc);
        }

        for (const auto &[ubo_name, ubo_loc] : comp_res.ubo_bindings) {
            Logger::default_logger().debug("Found shader program UBO %s with binding %d",
                    ubo_name.c_str(), ubo_loc);
        }

        ShaderReflectionInfo refl;
        refl.attribute_locations = comp_res.attributes;
        refl.output_locations = comp_res.outputs;
        refl.uniform_variable_locations = comp_res.uniforms;
        refl.buffer_locations = comp_res.buffers;
        refl.ubo_bindings = comp_res.ubo_bindings;
        refl.ubo_instance_names = comp_res.ubo_names;

        return std::make_pair(spirv_shaders, refl);
    }
}
