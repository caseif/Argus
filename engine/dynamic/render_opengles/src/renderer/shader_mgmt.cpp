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
#include "argus/lowlevel/time.hpp"

#include "argus/shadertools.hpp"

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"

#include "argus/render/common/material.hpp"
#include "argus/render/common/shader.hpp"
#include "argus/render/common/shader_compilation.hpp"
#include "argus/render/defines.hpp"

#include "internal/render_opengles/defines.hpp"
#include "internal/render_opengles/gl_util.hpp"
#include "internal/render_opengles/types.hpp"
#include "internal/render_opengles/renderer/shader_mgmt.hpp"
#include "internal/render_opengles/state/renderer_state.hpp"

#include "aglet/aglet.h"
#include "spirv_glsl.hpp"

#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace argus {
    std::pair<std::vector<std::pair<Shader, shader_handle_t>>, ShaderReflectionInfo> _compile_shaders(
            const std::vector<Shader> &shaders) {
        std::vector<std::pair<Shader, shader_handle_t>> handles;

        ShaderReflectionInfo refl_info;

        if (shaders.size() == 0) {
            return std::make_pair(handles, refl_info);
        }

        auto type = shaders.at(0).get_type();

        std::vector<std::string> shader_sources;
        for (auto &shader : shaders) {
            shader_sources.push_back(std::string(shader.get_source().begin(), shader.get_source().end()));
        }

        auto comp_res = compile_glsl_to_spirv(shaders, glslang::EShClientOpenGL,
                glslang::EShTargetOpenGL_450, glslang::EShTargetSpv_1_0);

        auto spirv_shaders = std::move(comp_res.first);
        refl_info = comp_res.second;

        for (auto &shader : spirv_shaders) {
            auto stage = shader.get_stage();
            auto &spirv_src = shader.get_source();

            spirv_cross::CompilerGLSL essl_compiler(reinterpret_cast<const uint32_t*>(spirv_src.data()),
                spirv_src.size() / 4);
            spirv_cross::CompilerGLSL::Options options;
            options.version = 310; // look into reducing this requirement with runtime uniform reflection
            options.es = true;
            essl_compiler.set_common_options(options);

            auto essl_src = essl_compiler.compile();

            GLuint gl_shader_stage;
            switch (stage) {
                case ShaderStage::Vertex:
                    gl_shader_stage = GL_VERTEX_SHADER;
                    break;
                case ShaderStage::Fragment:
                    gl_shader_stage = GL_FRAGMENT_SHADER;
                    break;
                default:
                    Logger::default_logger().fatal("Unrecognized shader stage ordinal %d",
                            static_cast<std::underlying_type<ShaderStage>::type>(stage));
            }

            auto shader_handle = glCreateShader(gl_shader_stage);
            if (!glIsShader(shader_handle)) {
                Logger::default_logger().fatal("Failed to create shader: %d", glGetError());
            }

            const auto src_c = reinterpret_cast<const char *>(essl_src.data());
            const int src_len = int(essl_src.size());

            glShaderSource(shader_handle, 1, &src_c, &src_len);

            glCompileShader(shader_handle);

            int res;
            glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &res);
            if (res == GL_FALSE) {
                int log_len;
                glGetShaderiv(shader_handle, GL_INFO_LOG_LENGTH, &log_len);
                char *log = new char[log_len + 1];
                glGetShaderInfoLog(shader_handle, log_len, nullptr, log);
                std::string stage_str;
                switch (stage) {
                    case ShaderStage::Vertex:
                        stage_str = "vertex";
                        break;
                    case ShaderStage::Fragment:
                        stage_str = "fragment";
                        break;
                    default:
                        stage_str = "unknown";
                        break;
                }
                get_gl_logger().fatal([log] () { delete[] log; }, "Failed to compile %s shader: %s", stage_str.c_str(), log);
            }

            handles.push_back(std::make_pair(shader, shader_handle));
        }

        return std::make_pair(handles, refl_info);
    }
\
    template <typename K, typename V, typename K2, typename V2>
    static V find_or_default(std::map<K, V> haystack, K2 &needle, V2 def) {
        auto it = haystack.find(needle);
        return it != haystack.end() ? it->second : static_cast<V>(def);
    }

    LinkedProgram link_program(std::initializer_list<std::string> shader_uids) {
        return link_program(std::vector<std::string>(shader_uids));
    }

    LinkedProgram link_program(std::vector<std::string> shader_uids) {
        auto program_handle = glCreateProgram();
        if (!glIsProgram(program_handle)) {
            Logger::default_logger().fatal("Failed to create program: %d", glGetError());
        }

        std::vector<Resource*> shader_resources;
        std::vector<Shader> shaders;
        for (auto &shader_uid : shader_uids) {
            auto &shader_res = ResourceManager::instance().get_resource(shader_uid);
            auto &shader = shader_res.get<Shader>();

            shader_resources.push_back(&shader_res);
            shaders.push_back(shader);
        }

        auto comp_res = _compile_shaders(shaders);
        auto compiled_shaders = comp_res.first;
        auto refl_info = comp_res.second;

        for (auto &compiled_shader : compiled_shaders) {
            glAttachShader(program_handle, compiled_shader.second);
        }

        //glBindFragDataLocation(program_handle, 0, SHADER_ATTRIB_OUT_FRAGDATA);

        glLinkProgram(program_handle);

        for (auto &compiled_shader : compiled_shaders) {
            glDetachShader(program_handle, compiled_shader.second);
        }

        for (auto &shader_res : shader_resources) {
            shader_res->release();
        }

        int res;
        glGetProgramiv(program_handle, GL_LINK_STATUS, &res);
        if (res == GL_FALSE) {
            int log_len;
            glGetProgramiv(program_handle, GL_INFO_LOG_LENGTH, &log_len);
            char *log = new char[log_len];
            glGetProgramInfoLog(program_handle, GL_INFO_LOG_LENGTH, nullptr, log);
            get_gl_logger().fatal([log] () { delete[] log; }, "Failed to link program: %s", log);
        }

        //auto proj_mat_loc = glGetUniformLocation(program, SHADER_UNIFORM_VIEW_MATRIX);
        auto proj_mat_loc = 0;
        int uni_size = 0;
        GLenum uni_type = 0;
        glGetActiveUniform(program_handle, proj_mat_loc, 0, nullptr, &uni_size, &uni_type, nullptr);
        if (uni_type != GL_FLOAT_MAT4) {
            proj_mat_loc = -1;
        }

        return LinkedProgram(program_handle, refl_info);
    }

    void build_shaders(RendererState &state, const Resource &material_res) {
        auto existing_program_it = state.linked_programs.find(material_res.uid);
        if (existing_program_it != state.linked_programs.end()) {
            return;
        }

        auto &material = material_res.get<Material>();

        std::string shader_type;

        auto program = link_program(material.get_shader_uids());

        state.linked_programs.insert({ material_res.uid, program });
    }

    void deinit_shader(shader_handle_t shader) {
        glDeleteShader(shader);
    }

    void remove_shader(RendererState &state, const std::string &shader_uid) {
        Logger::default_logger().debug("De-initializing shader %s", shader_uid.c_str());
        auto &shaders = state.compiled_shaders;
        auto existing_it = shaders.find(shader_uid);
        if (existing_it != shaders.end()) {
            deinit_shader(existing_it->second);
            shaders.erase(existing_it);
        }
    }

    void deinit_program(program_handle_t program) {
        glDeleteProgram(program);
    }

    void set_per_frame_global_uniforms(LinkedProgram &program) {
        program.get_uniform_loc_and_then(SHADER_UNIFORM_TIME, [] (auto time_loc) {
            glUniform1f(time_loc,
                static_cast<float>(
                    static_cast<double>(
                        std::chrono::time_point_cast<std::chrono::microseconds>(now()).time_since_epoch().count()
                    ) / 1000.0
                )
            );
        });
    }
}
