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

#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/time.hpp"

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"

#include "argus/render/common/material.hpp"
#include "argus/render/common/shader.hpp"
#include "argus/render/common/shader_compilation.hpp"
#include "argus/render/defines.hpp"

#include "internal/render_opengles/gl_util.hpp"
#include "internal/render_opengles/types.hpp"
#include "internal/render_opengles/renderer/shader_mgmt.hpp"
#include "internal/render_opengles/state/renderer_state.hpp"

#include "aglet/aglet.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"

#include "spirv_glsl.hpp"

#pragma GCC diagnostic pop

#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace argus {
    struct CompiledShader {
        Shader shader;
        shader_handle_t handle;
    };

    struct ShaderCompilationResult {
        std::vector<CompiledShader> shaders;
        ShaderReflectionInfo reflection;
        bool explicit_attrib_locations;
        bool explicit_uniform_locations;
    };

    static ShaderCompilationResult _compile_shaders(
            const std::vector<Shader> &shaders) {
        std::vector<std::pair<Shader, shader_handle_t>> handles;

        ShaderCompilationResult res;
        res.explicit_attrib_locations = true;
        res.explicit_uniform_locations = true;

        if (shaders.empty()) {
            return res;
        }

        auto type = shaders.at(0).get_type();

        std::vector<std::string> shader_uids;
        std::vector<std::string> shader_sources;
        for (auto &shader : shaders) {
            shader_uids.emplace_back(shader.get_uid());
            shader_sources.emplace_back(shader.get_source().begin(), shader.get_source().end());
        }

        std::ostringstream shader_uids_oss;
        std::copy(shader_uids.begin(), shader_uids.end(),
                std::ostream_iterator<std::string>(shader_uids_oss, ", "));
        auto shader_uids_str = shader_uids_oss.str().substr(0, shader_uids_oss.str().size() - strlen(", "));
        Logger::default_logger().debug("Transpiling shader set [%s]", shader_uids_str.c_str());

        auto [spirv_shaders, refl_info] = compile_glsl_to_spirv(shaders, glslang::EShClientOpenGL,
                glslang::EShTargetOpenGL_450, glslang::EShTargetSpv_1_0);

        res.reflection = refl_info;

        spirv_cross::CompilerGLSL::Options options;

        if (AGLET_GL_ES_VERSION_3_1) {
            options.version = 310;
        } else {
            options.version = 300;
            // need 310 support for uniform location decorations
            res.explicit_uniform_locations = false;
            // we get explicit attrib locations with our minimum profile (3.0)
            // so no need to touch that flag
        }

        options.es = true;

        for (auto &shader : spirv_shaders) {
            Logger::default_logger().debug("Creating shader %s", shader.get_uid().c_str());

            auto stage = shader.get_stage();
            auto &spirv_src = shader.get_source();

            spirv_cross::CompilerGLSL essl_compiler(reinterpret_cast<const uint32_t *>(spirv_src.data()),
                    spirv_src.size() / 4);
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
            const int src_len = GLint(essl_src.size());

            Logger::default_logger().debug("ESSL source:\n%s", src_c);

            glShaderSource(shader_handle, 1, &src_c, &src_len);
            glCompileShader(shader_handle);

            int gl_res;
            glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &gl_res);
            if (gl_res == GL_FALSE) {
                GLint log_len;
                glGetShaderiv(shader_handle, GL_INFO_LOG_LENGTH, &log_len);
                assert(log_len >= 0);
                char *log = new char[size_t(log_len + 1)];
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

            res.shaders.emplace_back(CompiledShader { shader, shader_handle });
        }

        return res;
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
        bool have_vert = false;
        bool have_frag = false;
        for (auto &shader_uid : shader_uids) {
            auto &shader_res = ResourceManager::instance().get_resource(shader_uid);
            auto &shader = shader_res.get<Shader>();

            shader_resources.push_back(&shader_res);
            shaders.push_back(shader);

            if (shader.get_stage() == ShaderStage::Vertex) {
                have_vert = true;
            } else if (shader.get_stage() == ShaderStage::Fragment) {
                have_frag = true;
            }
        }

        if (!have_vert) {
            shaders.push_back(ResourceManager::instance().get_resource(SHADER_STD_VERT));
        }
        if (!have_frag) {
            shaders.push_back(ResourceManager::instance().get_resource(SHADER_STD_FRAG));
        }

        auto comp_res = _compile_shaders(shaders);
        auto compiled_shaders = comp_res.shaders;
        auto refl_info = comp_res.reflection;

        for (auto &compiled_shader : compiled_shaders) {
            glAttachShader(program_handle, compiled_shader.handle);
        }

        glLinkProgram(program_handle);

        for (auto &compiled_shader : compiled_shaders) {
            glDetachShader(program_handle, compiled_shader.handle);
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

        if (!comp_res.explicit_uniform_locations) {
            GLint uniform_max_len;
            GLint uniform_count;

            glGetProgramiv(program_handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniform_max_len);
            assert(uniform_max_len >= 0);
            glGetProgramiv(program_handle, GL_ACTIVE_UNIFORMS, &uniform_count);

            GLsizei uniform_name_len;
            GLsizei uniform_size;
            GLenum uniform_type;
            char *uniform_name = new char[uint32_t(uniform_max_len)];

            for (int i = 0; i < uniform_count; i++) {
                glGetActiveUniform(program_handle, uint32_t(i), uniform_max_len, &uniform_name_len,
                        &uniform_size, &uniform_type, uniform_name);
                assert(uniform_name_len <= uniform_max_len);
                GLint uniform_loc = glGetUniformLocation(program_handle, uniform_name);
                assert(uniform_loc >= 0);
                refl_info.uniform_variable_locations[std::string(uniform_name)] = uint32_t(uniform_loc);
            }
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
}
