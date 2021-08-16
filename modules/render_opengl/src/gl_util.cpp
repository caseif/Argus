/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/macros.hpp"
#include "internal/lowlevel/logging.hpp"

// module render_opengl
#include "internal/render_opengl/gl_util.hpp"

#include "aglet/aglet.h"

namespace argus {
    void activate_gl_context(GLFWwindow *window) {
        if (glfwGetCurrentContext() == window) {
            // already current
            return;
        }

        glfwMakeContextCurrent(window);
        if (glfwGetCurrentContext() != window) {
            _ARGUS_FATAL("Failed to make GL context current\n");
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
        auto stream = stdout;
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:
                level = "SEVERE";
                stream = stderr;
                break;
            case GL_DEBUG_SEVERITY_MEDIUM:
                level = "WARN";
                stream = stderr;
                break;
            case GL_DEBUG_SEVERITY_LOW:
                level = "INFO";
                break;
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                level = "TRACE";
                break;
            default: // shouldn't happen
                level = "UNKNOWN";
                stream = stderr;
                break;
        }
        _GENERIC_PRINT(stream, level, "GL", "%s\n", message);
    }

    void set_attrib_pointer(GLuint vertex_len, GLuint attr_len, GLuint attr_index, GLuint *attr_offset) {
        glEnableVertexAttribArray(attr_index);
        glVertexAttribPointer(attr_index, attr_len, GL_FLOAT, GL_FALSE, vertex_len * sizeof(GLfloat),
                reinterpret_cast<GLvoid*>(*attr_offset));
        *attr_offset += attr_len * sizeof(GLfloat);
    }

    void try_delete_buffer(buffer_handle_t buffer) {
        if (buffer == 0) {
            return;
        }

        glDeleteBuffers(1, &buffer);
    }
}
