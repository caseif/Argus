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

#pragma once

#include "argus/lowlevel/logging.hpp"

#include "internal/render_opengles/types.hpp"

#include "aglet/aglet.h"

namespace argus {
    void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
            const GLchar *message, const void *user_param);

    void set_attrib_pointer(buffer_handle_t buffer_obj, binding_index_t binding_index, GLuint vertex_len,
            GLuint attr_len, GLuint attr_index, GLuint *attr_offset);

    void try_delete_buffer(buffer_handle_t buffer);

    Logger &get_gl_logger(void);
}
