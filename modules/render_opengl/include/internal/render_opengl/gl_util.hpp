/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include "internal/render_opengl/glfw_include.hpp"
#include "internal/render_opengl/globals.hpp"

namespace argus {
    void set_attrib_pointer(GLuint vertex_len, GLuint attr_len, GLuint attr_index, GLuint *attr_offset);

    void try_delete_buffer(buffer_handle_t buffer);
}
