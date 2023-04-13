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
#include "argus/lowlevel/macros.hpp"

#include "internal/render_opengl/gl_util.hpp"
#include "internal/render_opengl/types.hpp"
#include "internal/render_opengl/renderer/buffer.hpp"

#include "aglet/aglet.h"
#include "GLFW/glfw3.h"

#include <cassert>
#include <climits>

namespace argus {
    static Logger g_gl_logger("GL");

    void activate_gl_context(GLFWwindow *window) {
        if (glfwGetCurrentContext() == window) {
            // already current
            return;
        }

        glfwMakeContextCurrent(window);
        if (glfwGetCurrentContext() != window) {
            Logger::default_logger().fatal("Failed to make GL context current");
        }
    }

    void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
            const GLchar *message, const void *userParam) {
        UNUSED(source);
        UNUSED(type);
        UNUSED(id);
        UNUSED(length);
        UNUSED(userParam);
        #ifndef _ARGUS_DEBUG_MODE
        if (severity == GL_DEBUG_SEVERITY_NOTIFICATION || severity == GL_DEBUG_SEVERITY_LOW) {
            return;
        }
        #endif
        char const *level;
        bool is_error = false;
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:
                level = "SEVERE";
                is_error = true;
                break;
            case GL_DEBUG_SEVERITY_MEDIUM:
                level = "WARN";
                is_error = true;
                break;
            case GL_DEBUG_SEVERITY_LOW:
                level = "INFO";
                break;
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                level = "TRACE";
                break;
            default: // shouldn't happen
                level = "UNKNOWN";
                is_error = true;
                break;
        }
        if (is_error) {
            g_gl_logger.log_error(level, "%s", message);
        } else {
            g_gl_logger.log(level, "%s", message);
        }
    }

    void set_attrib_pointer(array_handle_t array_obj, buffer_handle_t buffer_obj, binding_index_t binding_index,
            GLuint vertex_len, GLuint attr_len, GLuint attr_index, GLuint *attr_offset) {
        assert(attr_len <= INT_MAX);

        if (AGLET_GL_ARB_direct_state_access) {
            glEnableVertexArrayAttrib(array_obj, attr_index);
            glVertexArrayAttribFormat(array_obj, attr_index, GLint(attr_len), GL_FLOAT, GL_FALSE, *attr_offset);
            glVertexArrayAttribBinding(array_obj, attr_index, binding_index);
        } else {
            auto stride = vertex_len * uint32_t(sizeof(GLfloat));
            assert(stride <= INT_MAX);

            glBindBuffer(GL_ARRAY_BUFFER, buffer_obj);
            glEnableVertexAttribArray(attr_index);
            glVertexAttribPointer(attr_index, GLint(attr_len), GL_FLOAT, GL_FALSE,
                    GLsizei(stride), reinterpret_cast<GLvoid *>(*attr_offset));
        }

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
