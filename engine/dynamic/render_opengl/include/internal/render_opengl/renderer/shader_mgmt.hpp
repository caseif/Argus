/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

// module render
#include "argus/render/common/material.hpp"

// module render_opengl
#include "internal/render_opengl/types.hpp"

#include "aglet/aglet.h"

#include <string>

namespace argus {
    // forward declarations
    struct RendererState;
    class Resource;
    class Shader;

    shader_handle_t compile_shader(const Shader &shader);

    // it is expected that the shaders will already be attached to the program when this function is called
    void link_program(program_handle_t program, VertexAttributes attrs);

    void build_shaders(RendererState &state, const Resource &material_res);

    void deinit_shader(shader_handle_t shader);

    void remove_shader(RendererState &state, const std::string &shader_uid);

    void deinit_program(program_handle_t program);
}
