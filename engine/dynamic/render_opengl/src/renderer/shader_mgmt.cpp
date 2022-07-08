/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"

#include "argus/render/common/material.hpp"
#include "argus/render/common/shader.hpp"

#include "internal/render_opengl/defines.hpp"
#include "internal/render_opengl/gl_util.hpp"
#include "internal/render_opengl/types.hpp"
#include "internal/render_opengl/renderer/shader_mgmt.hpp"
#include "internal/render_opengl/state/renderer_state.hpp"

#include "aglet/aglet.h"

#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace argus {
    shader_handle_t compile_shader(const Shader &shader) {
        auto stage = shader.get_stage();
        auto &src = shader.get_source();

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

        const auto src_c = reinterpret_cast<const char*>(src.data());
        const int src_len = int(src.size());

        if (shader.get_type() == SHADER_TYPE_SPIR_V) {
            glShaderBinary(1, &shader_handle, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, src_c, src_len);
            glSpecializeShaderARB(shader_handle, "main", 0, nullptr, nullptr);
        } else if (shader.get_type() ==  SHADER_TYPE_GLSL) {
            glShaderSource(shader_handle, 1, &src_c, &src_len);
            glCompileShader(shader_handle);
        } else {
            Logger::default_logger().fatal("Unknown shader type %s", shader.get_type().c_str());
        }

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

        return shader_handle;
    }

    // it is expected that the shaders will already be attached to the program when this function is called
    LinkedProgram link_program(program_handle_t program, const std::string &shader_type) {
        glBindFragDataLocation(program, 0, SHADER_ATTRIB_OUT_FRAGDATA);

        glLinkProgram(program);

        int res;
        glGetProgramiv(program, GL_LINK_STATUS, &res);
        if (res == GL_FALSE) {
            int log_len;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
            char *log = new char[log_len];
            glGetProgramInfoLog(program, GL_INFO_LOG_LENGTH, nullptr, log);
            get_gl_logger().fatal([log] () { delete[] log; }, "Failed to link program: %s", log);
        }

        //auto proj_mat_loc = glGetUniformLocation(program, SHADER_UNIFORM_VIEW_MATRIX);
        auto proj_mat_loc = 0;
        int uni_size = 0;
        GLenum uni_type = 0;
        glGetActiveUniform(program, proj_mat_loc, 0, nullptr, &uni_size, &uni_type, nullptr);
        if (uni_type != GL_FLOAT_MAT4) {
            proj_mat_loc = -1;
        }

        attribute_location_t attr_pos;
        attribute_location_t attr_norm;
        attribute_location_t attr_color;
        attribute_location_t attr_texcoord;
        if (shader_type == SHADER_TYPE_GLSL) {
            attr_pos = glGetAttribLocation(program, SHADER_ATTRIB_IN_POSITION);
            attr_norm = glGetAttribLocation(program, SHADER_ATTRIB_IN_NORMAL);
            attr_color = glGetAttribLocation(program, SHADER_ATTRIB_IN_COLOR);
            attr_texcoord = glGetAttribLocation(program, SHADER_ATTRIB_IN_TEXCOORD);
        } else if (shader_type == SHADER_TYPE_SPIR_V) {
            attr_pos = 0;
            attr_norm = 1;
            attr_color = 2;
            attr_texcoord = 3;
        } else {
            Logger::default_logger().fatal("Unrecognized shader type %s", shader_type.c_str());
        }

        return LinkedProgram(program, attr_pos, attr_norm, attr_color, attr_texcoord, proj_mat_loc);
    }

    void build_shaders(RendererState &state, const Resource &material_res) {
        auto existing_program_it = state.linked_programs.find(material_res.uid);
        if (existing_program_it != state.linked_programs.end()) {
            return;
        }

        auto program_handle = glCreateProgram();
        if (!glIsProgram(program_handle)) {
            Logger::default_logger().fatal("Failed to create program: %d", glGetError());
        }

        auto &material = material_res.get<Material>();

        std::string shader_type;

        for (auto &shader_uid : material.get_shader_uids()) {
            shader_handle_t shader_handle;

            auto &shader_res = ResourceManager::instance().get_resource_weak(shader_uid);
            auto &shader = shader_res.get<Shader>();
            shader_type = shader.get_type();

            auto existing_shader_it = state.compiled_shaders.find(shader_uid);
            if (existing_shader_it != state.compiled_shaders.end()) {
                shader_handle = existing_shader_it->second;
            } else {
                shader_handle = compile_shader(shader);

                state.compiled_shaders.insert({ shader_uid, shader_handle });
            }

            glAttachShader(program_handle, shader_handle);
        }

        auto program = link_program(program_handle, shader_type);

        state.linked_programs.insert({ material_res.uid, program });

        for (auto &shader_uid : material.get_shader_uids()) {
            glDetachShader(program_handle, state.compiled_shaders[shader_uid]);
        }
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
