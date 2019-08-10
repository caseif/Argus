#pragma once

#include <SDL2/SDL_opengl.h>

namespace argus {

    namespace glext {
        extern void (*glGenFramebuffers)(GLsizei n, GLuint* framebuffers);
        extern void (*glFramebufferTexture)(GLenum target, GLenum attachment, GLuint texture, GLint level);

        extern void (*glGenBuffers)(GLsizei n, GLuint *buffers);
        extern void (*glBindBuffer)(GLenum target, GLuint buffer);
        extern void (*glBufferData)(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);

        extern void (*glGenVertexArrays)(GLsizei n, GLuint *arrays);
        extern void (*glDeleteVertexArrays)(GLsizei n, const GLuint *arrays);
        extern void (*glBindVertexArray)(GLuint array);
        extern void (*glEnableVertexAttribArray)(GLuint index);
        extern void (*glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride);
    }

    void load_opengl_extensions(void);

}
