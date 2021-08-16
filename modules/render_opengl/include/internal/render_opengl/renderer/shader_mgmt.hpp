/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
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

    void deinit_shader(RendererState &state, const std::string &shader_uid);

    void deinit_program(program_handle_t program);
}
