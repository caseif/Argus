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

        extern void (*glAttachShader)(GLuint program, GLuint shader);
        extern void (*glCompileShader)(GLuint shader);
        extern GLuint (*glCreateProgram)(void);
        extern GLuint (*glCreateShader)(GLenum shaderType);
        extern void (*glDeleteProgram)(GLuint program);
        extern void (*glDeleteShader)(GLuint shader);
        extern void (*glDetachShader)(GLuint program, GLuint shader);
        extern void (*glGetProgramiv)(GLuint program, GLenum pname, GLint *params);
        extern void (*glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
        extern void (*glGetShaderInfoLog)(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
        extern GLint (*glGetUniformLocation)(GLuint program, const GLchar *name);
        extern void (*glLinkProgram)(GLuint program);
        extern void (*glShaderSource)(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);

    }

    void load_opengl_extensions(void);

}
