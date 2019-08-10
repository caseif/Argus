#pragma once

#include <SDL2/SDL_opengl.h>

namespace argus {

    namespace glext {
        extern void (*glFramebufferTexture)(GLenum target, GLenum attachment, GLuint texture, GLint level);
        extern void (*glGenFramebuffers)(GLsizei n, GLuint* framebuffers);

        extern void (*glBindBuffer)(GLenum target, GLuint buffer);
        extern void (*glBufferData)(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
        extern void (*glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
        extern void (*glGenBuffers)(GLsizei n, GLuint *buffers);

        extern void (*glBindVertexArray)(GLuint array);
        extern void (*glDeleteVertexArrays)(GLsizei n, const GLuint *arrays);
        extern void (*glEnableVertexAttribArray)(GLuint index);
        extern void (*glGenVertexArrays)(GLsizei n, GLuint *arrays);
        extern void (*glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride);
    }

    void load_opengl_extensions(void);

}
