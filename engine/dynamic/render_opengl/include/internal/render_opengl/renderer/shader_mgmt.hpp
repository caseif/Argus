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

#include "argus/render/common/material.hpp"
#include "argus/render/util/linked_program.hpp"

#include "internal/render_opengl/types.hpp"

#include <string>

namespace argus {
    // forward declarations
    struct RendererState;

    class Resource;

    class Shader;

    struct LinkedProgram {
        shader_handle_t handle;
        ShaderReflectionInfo reflection;

        LinkedProgram(shader_handle_t handle, ShaderReflectionInfo reflection) :
                handle(handle),
                reflection(std::move(reflection)) {
        }
    };

    LinkedProgram link_program(std::initializer_list<std::string> shader_uids);

    LinkedProgram link_program(const std::vector<std::string> &shader_uids);

    void build_shaders(RendererState &state, const Resource &material_res);

    void deinit_shader(shader_handle_t shader);

    void remove_shader(RendererState &state, const std::string &shader_uid);

    void deinit_program(program_handle_t program);

    void set_per_frame_global_uniforms(LinkedProgram &program);
}
