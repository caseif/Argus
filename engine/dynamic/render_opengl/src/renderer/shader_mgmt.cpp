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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/time.hpp"

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"

#include "argus/render/common/material.hpp"
#include "argus/render/common/shader.hpp"
#include "argus/render/common/shader_compilation.hpp"
#include "argus/render/defines.hpp"
#include "argus/render/util/linked_program.hpp"

#include "internal/render_opengl/gl_util.hpp"
#include "internal/render_opengl/types.hpp"
#include "internal/render_opengl/renderer/shader_mgmt.hpp"
#include "internal/render_opengl/state/renderer_state.hpp"

#include "aglet/aglet.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"

#include "spirv_glsl.hpp"

#pragma GCC diagnostic pop

#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <cassert>
#include <climits>

namespace argus {
    std::pair<std::vector<std::pair<Shader, shader_handle_t>>, ShaderReflectionInfo> _compile_shaders(
            const std::vector<Shader> &shaders) {
        std::vector<std::pair<Shader, shader_handle_t>> handles;

        if (shaders.empty()) {
            return std::make_pair(handles, ShaderReflectionInfo{});
        }

        std::vector<std::string> shader_sources;
        for (auto &shader : shaders) {
            shader_sources.emplace_back(shader.get_source().begin(), shader.get_source().end());
        }

        auto comp_res = compile_glsl_to_spirv(shaders, glslang::EShClientOpenGL,
                glslang::EShTargetOpenGL_450, glslang::EShTargetSpv_1_0);
        auto spirv_shaders = std::move(comp_res.first);
        auto refl_info = comp_res.second;

        for (auto &shader : spirv_shaders) {
            auto stage = shader.get_stage();
            auto spirv_src = shader.get_source();

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

            if (AGLET_GL_VERSION_4_1 && AGLET_GL_ARB_gl_spirv) {
                char *spirv_src_c = new char[spirv_src.size()];
                memcpy(spirv_src_c, spirv_src.data(), spirv_src.size());
                const auto spirv_src_len = GLsizei(spirv_src.size());

                glShaderBinary(1, &shader_handle, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, spirv_src_c, spirv_src_len);
                glSpecializeShaderARB(shader_handle, "main", 0, nullptr, nullptr);
            } else {
                spirv_cross::CompilerGLSL glsl_compiler(reinterpret_cast<const uint32_t *>(spirv_src.data()),
                        spirv_src.size() / 4);
                spirv_cross::CompilerGLSL::Options options;
                options.version = 430; //TODO: may want to reduce this requirement and just do runtime uniform reflection
                options.es = false;
                glsl_compiler.set_common_options(options);

                auto glsl_src = glsl_compiler.compile();

                char *glsl_src_c = new char[glsl_src.size()];
                memcpy(glsl_src_c, glsl_src.data(), glsl_src.size());
                const auto glsl_src_len = GLsizei(glsl_src.size());

                Logger::default_logger().debug("GLSL source:\n%s", glsl_src_c);

                glShaderSource(shader_handle, 1, &glsl_src_c, &glsl_src_len);
                glCompileShader(shader_handle);
            }

            int res;
            glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &res);
            if (res == GL_FALSE) {
                int log_len;
                glGetShaderiv(shader_handle, GL_INFO_LOG_LENGTH, &log_len);
                assert(log_len >= 0);
                char *log = new char[size_t(log_len) + 1];
                log[0] = '\0';
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
                get_gl_logger().fatal([log]() { delete[] log; }, "Failed to compile %s shader: %s",
                        stage_str.c_str(), log);
            }
            handles.emplace_back(shader, shader_handle);
        }

        return std::make_pair(handles, refl_info);
    }

    template<typename K, typename V, typename K2, typename V2>
    static V find_or_default(std::map<K, V> haystack, K2 &needle, V2 def) {
        auto it = haystack.find(needle);
        return it != haystack.end() ? it->second : static_cast<V>(def);
    }

    LinkedProgram link_program(std::initializer_list<std::string> shader_uids) {
        return link_program(std::vector<std::string>(shader_uids));
    }

    LinkedProgram link_program(const std::vector<std::string> &shader_uids) {
        auto program_handle = glCreateProgram();
        if (!glIsProgram(program_handle)) {
            Logger::default_logger().fatal("Failed to create program: %d", glGetError());
        }

        std::vector<Resource *> shader_resources;
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

        glBindFragDataLocation(program_handle, 0, SHADER_ATTRIB_OUT_FRAGDATA);

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
            assert(log_len >= 0);
            char *log = new char[size_t(log_len)];
            glGetProgramInfoLog(program_handle, GL_INFO_LOG_LENGTH, nullptr, log);
            get_gl_logger().fatal([log]() { delete[] log; }, "Failed to link program: %s", log);
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

        state.linked_programs.insert({material_res.uid, program});
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
        program.get_uniform_loc_and_then(SHADER_UNIFORM_TIME, [](auto time_loc) {
            affirm_precond(time_loc <= INT_MAX, "Global uniform '" SHADER_UNIFORM_TIME "' location is too big");
            glUniform1f(GLint(time_loc),
                    float(
                            double(
                                    std::chrono::time_point_cast<std::chrono::microseconds>(
                                            now()).time_since_epoch().count()
                            ) / 1000.0
                    )
            );
        });
    }
}
