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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/logging.hpp"

#include "argus/core/engine.hpp"

#include "internal/render_opengl_legacy/gl_util.hpp"
#include "internal/render_opengl_legacy/types.hpp"

#include "aglet/aglet.h"

#include <climits>

namespace argus {
    static Logger g_gl_logger("GL");

    void set_attrib_pointer(buffer_handle_t buffer_obj, GLuint vertex_len, GLuint attr_len, GLuint attr_index,
            GLuint *attr_offset) {
        argus_assert(attr_len <= INT_MAX);

        auto stride = vertex_len * uint32_t(sizeof(GLfloat));
        argus_assert(stride <= INT_MAX);

        glBindBuffer(GL_ARRAY_BUFFER, buffer_obj);
        glEnableVertexAttribArray(attr_index);
        glVertexAttribPointer(attr_index, GLint(attr_len), GL_FLOAT, GL_FALSE,
                GLsizei(stride), reinterpret_cast<GLvoid *>(uint64_t(*attr_offset)));

        *attr_offset += attr_len * GLuint(sizeof(GLfloat));
    }

    void try_delete_buffer(buffer_handle_t buffer) {
        if (buffer == 0) {
            return;
        }

        glDeleteBuffers(1, &buffer);
    }

    Logger &get_gl_logger(void) {
        return g_gl_logger;
    }
}
