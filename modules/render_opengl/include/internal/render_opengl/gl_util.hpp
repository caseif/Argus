#pragma once

#include "internal/render_opengl/glext.hpp"
#include "internal/render_opengl/glfw_include.hpp"
#include "internal/render_opengl/globals.hpp"

namespace argus {
    void set_attrib_pointer(GLuint vertex_len, GLuint attr_len, GLuint attr_index, GLuint *attr_offset);

    void try_delete_buffer(buffer_handle_t buffer);
}
