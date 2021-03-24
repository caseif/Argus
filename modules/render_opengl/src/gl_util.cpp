#include "internal/render_opengl/gl_util.hpp"
#include "internal/render_opengl/glext.hpp"

namespace argus {
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
