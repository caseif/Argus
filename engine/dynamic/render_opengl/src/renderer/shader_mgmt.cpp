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
#include "internal/render_opengl/renderer/buffer.hpp"
#include "internal/render_opengl/renderer/shader_mgmt.hpp"
#include "internal/render_opengl/state/renderer_state.hpp"

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

#include <climits>

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

        if (shaders.empty()) {
            return ShaderCompilationResult {};
        }

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
        Logger::default_logger().debug("Compiling SPIR-V from shader set [%s]", shader_uids_str.c_str());

        auto [spirv_shaders, refl_info] = compile_glsl_to_spirv(shaders, glslang::EShClientOpenGL,
                glslang::EShTargetOpenGL_450, glslang::EShTargetSpv_1_0);

        ShaderCompilationResult res;
        res.reflection = refl_info;
        res.explicit_attrib_locations = true;
        res.explicit_uniform_locations = true;

        bool have_gl_spirv = AGLET_GL_VERSION_4_1 && AGLET_GL_ARB_gl_spirv;

        spirv_cross::CompilerGLSL::Options options;

        if (!have_gl_spirv) {
            if (AGLET_GL_VERSION_4_6) {
                options.version = 460;
            } else if (AGLET_GL_VERSION_4_3) {
                options.version = 430;
            } else {
                // need 430 support for uniform location/binding decorations
                res.explicit_uniform_locations = false;

                if (AGLET_GL_VERSION_4_1) {
                    options.version = 410;
                } else {
                    // need 410 support for attribute location decorations
                    res.explicit_attrib_locations = false;

                    options.version = 330;
                }
            }

            options.es = false;
        }

        for (auto &shader : spirv_shaders) {
            Logger::default_logger().debug("Creating shader %s", shader.get_uid().c_str());

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

            if (have_gl_spirv) {
                Logger::default_logger().debug("GL 4.1 profile and ARB_gl_spirv are available, "
                                               "passing compiled SPIR-V directly to OpenGL via glShaderBinary");

                char *spirv_src_c = new char[spirv_src.size()];
                memcpy(spirv_src_c, spirv_src.data(), spirv_src.size());
                const auto spirv_src_len = GLsizei(spirv_src.size());

                glShaderBinary(1, &shader_handle, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, spirv_src_c, spirv_src_len);
                glSpecializeShaderARB(shader_handle, "main", 0, nullptr, nullptr);
            } else {
                Logger::default_logger().debug("GL 4.1 profile and/or ARB_gl_spirv is not available, "
                                               "transpiling compiled SPIR-V to GLSL");

                spirv_cross::CompilerGLSL glsl_compiler(reinterpret_cast<const uint32_t *>(spirv_src.data()),
                        spirv_src.size() / sizeof(uint32_t));
                glsl_compiler.set_common_options(options);

                auto glsl_src = glsl_compiler.compile();

                char *glsl_src_c = glsl_src.data();
                const auto glsl_src_len = GLsizei(glsl_src.size());

                Logger::default_logger().debug("GLSL source:\n%s", glsl_src_c);

                glShaderSource(shader_handle, 1, &glsl_src_c, &glsl_src_len);
                glCompileShader(shader_handle);
            }

            int gl_res;
            glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &gl_res);
            if (gl_res == GL_FALSE) {
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

        glBindFragDataLocation(program_handle, 0, SHADER_OUT_COLOR);
        glBindFragDataLocation(program_handle, 1, SHADER_OUT_LIGHT_OPACITY);

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

        if (!comp_res.explicit_attrib_locations) {
            GLint attrib_max_len;
            GLint attrib_count;

            glGetProgramiv(program_handle, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &attrib_max_len);
            assert(attrib_max_len >= 0);
            glGetProgramiv(program_handle, GL_ACTIVE_ATTRIBUTES, &attrib_count);

            GLsizei attrib_name_len;
            GLint attrib_size;
            GLenum attrib_type;
            char *attrib_name = new char[uint32_t(attrib_max_len) + 1];

            for (int i = 0; i < attrib_count; i++) {
                glGetActiveAttrib(program_handle, uint32_t(i), attrib_max_len, &attrib_name_len,
                        &attrib_size, &attrib_type, attrib_name);
                assert(attrib_name_len <= attrib_max_len);
                GLint attrib_loc = glGetAttribLocation(program_handle, attrib_name);
                assert(attrib_loc >= 0);
                refl_info.attribute_locations[std::string(attrib_name)] = uint32_t(attrib_loc);
            }
        }

        if (!comp_res.explicit_uniform_locations) {
            GLint uniform_max_len;
            GLint uniform_count;

            glGetProgramiv(program_handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniform_max_len);
            assert(uniform_max_len >= 0);
            glGetProgramiv(program_handle, GL_ACTIVE_UNIFORMS, &uniform_count);

            GLsizei uniform_name_len;
            char *uniform_name = new char[uint32_t(uniform_max_len)];

            for (int i = 0; i < uniform_count; i++) {
                glGetActiveUniformName(program_handle, uint32_t(i), uniform_max_len, &uniform_name_len,
                        uniform_name);
                assert(uniform_name_len <= uniform_max_len);
                GLint uniform_loc = glGetUniformLocation(program_handle, uniform_name);
                assert(uniform_loc >= 0);
                refl_info.uniform_variable_locations[std::string(uniform_name)] = uint32_t(uniform_loc);
            }
        }

        return LinkedProgram(program_handle, refl_info, have_frag);
    }

    LinkedProgram &build_shaders(RendererState &state, const Resource &material_res) {
        auto existing_program_it = state.linked_programs.find(material_res.uid);
        if (existing_program_it != state.linked_programs.end()) {
            return existing_program_it->second;
        }

        auto &material = material_res.get<Material>();

        std::string shader_type;

        auto program = link_program(material.get_shader_uids());

        return state.linked_programs.insert({material_res.uid, program}).first->second;
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

    LinkedProgram &get_std_program(RendererState &state) {
        if (!state.std_program.has_value()) {
            state.std_program = link_program({ SHADER_STD_VERT, SHADER_STD_FRAG });
        }
        return state.std_program.value();
    }

    LinkedProgram &get_material_program(RendererState &state, const Resource &mat_res) {
        auto &mat = mat_res.get<Material>();

        if (mat.get_shader_uids().empty()) {

        }

        auto it = state.linked_programs.find(mat_res.uid);
        if (it != state.linked_programs.cend()) {
            return it->second;
        } else {
            return build_shaders(state, mat_res);
        }
    }
}
