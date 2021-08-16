/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/lowlevel/logging.hpp"

// module resman
#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"

// module render
#include "argus/render/common/shader.hpp"
#include "internal/render/pimpl/common/material.hpp"
#include "internal/render/pimpl/common/shader.hpp"

// module render_opengl
#include "internal/render_opengl/defines.hpp"
#include "internal/render_opengl/renderer/shader_mgmt.hpp"
#include "internal/render_opengl/state/renderer_state.hpp"

#include "aglet/aglet.h"

namespace argus {
    shader_handle_t compile_shader(const Shader &shader) {
        auto &src = shader.pimpl->src;
        auto stage = shader.pimpl->stage;

        GLuint gl_shader_stage;
        switch (stage) {
            case ShaderStage::Vertex:
                gl_shader_stage = GL_VERTEX_SHADER;
                break;
            case ShaderStage::Fragment:
                gl_shader_stage = GL_FRAGMENT_SHADER;
                break;
            default:
                _ARGUS_FATAL("Unrecognized shader stage ordinal %d\n",
                        static_cast<std::underlying_type<ShaderStage>::type>(stage));
        }

        auto shader_handle = glCreateShader(gl_shader_stage);
        if (!glIsShader(shader_handle)) {
            _ARGUS_FATAL("Failed to create shader: %d\n", glGetError());
        }

        const char *const src_c = src.c_str();
        const int src_len = int(src.length());

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
            _ARGUS_FATAL("Failed to compile %s shader: %s\n", stage_str.c_str(), log);
            delete[] log;
        }

        return shader_handle;
    }

    // it is expected that the shaders will already be attached to the program when this function is called
    void link_program(program_handle_t program, VertexAttributes attrs) {
        if (attrs & VertexAttributes::POSITION) {
            glBindAttribLocation(program, SHADER_ATTRIB_LOC_POSITION, SHADER_ATTRIB_IN_POSITION);
        }
        if (attrs & VertexAttributes::NORMAL) {
            glBindAttribLocation(program, SHADER_ATTRIB_LOC_NORMAL, SHADER_ATTRIB_IN_NORMAL);
        }
        if (attrs & VertexAttributes::COLOR) {
            glBindAttribLocation(program, SHADER_ATTRIB_LOC_COLOR, SHADER_ATTRIB_IN_COLOR);
        }
        if (attrs & VertexAttributes::TEXCOORD) {
            glBindAttribLocation(program, SHADER_ATTRIB_LOC_TEXCOORD, SHADER_ATTRIB_IN_TEXCOORD);
        }

        glBindFragDataLocation(program, 0, SHADER_ATTRIB_OUT_FRAGDATA);

        glLinkProgram(program);

        int res;
        glGetProgramiv(program, GL_LINK_STATUS, &res);
        if (res == GL_FALSE) {
            int log_len;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
            char *log = new char[log_len];
            glGetProgramInfoLog(program, GL_INFO_LOG_LENGTH, nullptr, log);
            _ARGUS_FATAL("Failed to link program: %s\n", log);
            delete[] log;
        }
    }

    void build_shaders(RendererState &state, const Resource &material_res) {
        auto existing_program_it = state.linked_programs.find(material_res.uid);
        if (existing_program_it != state.linked_programs.end()) {
            return;
        }

        auto program_handle = glCreateProgram();
        if (!glIsProgram(program_handle)) {
            _ARGUS_FATAL("Failed to create program: %d\n", glGetError());
        }

        auto &material = material_res.get<Material>();

        for (auto &shader_uid : material.pimpl->shaders) {
            shader_handle_t shader_handle;

            auto &shader_res = ResourceManager::get_global_resource_manager().get_resource_weak(shader_uid);
            auto &shader = shader_res.get<Shader>();

            auto existing_shader_it = state.compiled_shaders.find(shader_uid);
            if (existing_shader_it != state.compiled_shaders.end()) {
                shader_handle = existing_shader_it->second;
            } else {
                shader_handle = compile_shader(shader);

                state.compiled_shaders.insert({ shader_uid, shader_handle });
            }

            glAttachShader(program_handle, shader_handle);
        }

        link_program(program_handle, material.pimpl->attributes);

        auto proj_mat_loc = glGetUniformLocation(program_handle, SHADER_UNIFORM_VIEW_MATRIX);

        state.linked_programs[material_res.uid] = { program_handle, proj_mat_loc };

        for (auto &shader_uid : material.pimpl->shaders) {
            glDetachShader(program_handle, state.compiled_shaders[shader_uid]);
        }
    }

    void deinit_shader(RendererState &state, const std::string &shader_uid) {
        _ARGUS_DEBUG("De-initializing shader %s\n", shader_uid.c_str());
        auto &shaders = state.compiled_shaders;
        auto existing_it = shaders.find(shader_uid);
        if (existing_it != shaders.end()) {
            glDeleteShader(existing_it->second);
            shaders.erase(existing_it);
        }
    }

    void deinit_program(program_handle_t program) {
        glDeleteProgram(program);
    }
}