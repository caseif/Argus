#pragma once

#include <SDL2/SDL_opengl.h>

namespace argus {

    namespace glext {
        typedef void (APIENTRY *DEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
            const GLchar *message, void *userParam);

        extern void (APIENTRY *glFramebufferTexture)(GLenum target, GLenum attachment, GLuint texture, GLint level);
        extern void (APIENTRY *glGenFramebuffers)(GLsizei n, GLuint* framebuffers);

        extern void (APIENTRY *glBindBuffer)(GLenum target, GLuint buffer);
        extern void (APIENTRY *glBufferData)(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
        extern void (APIENTRY *glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
        extern void (APIENTRY *glDeleteBuffers)(GLsizei n, const GLuint *buffers);
        extern void (APIENTRY *glGenBuffers)(GLsizei n, GLuint *buffers);

        extern void (APIENTRY *glBindVertexArray)(GLuint array);
        extern void (APIENTRY *glDeleteVertexArrays)(GLsizei n, const GLuint *arrays);
        extern void (APIENTRY *glEnableVertexAttribArray)(GLuint index);
        extern void (APIENTRY *glGenVertexArrays)(GLsizei n, GLuint *arrays);
        extern void (APIENTRY *glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);

        extern void (APIENTRY *glAttachShader)(GLuint program, GLuint shader);
        extern void (APIENTRY *glBindAttribLocation)(GLuint program, GLuint index, const GLchar *name);
        extern void (APIENTRY *glCompileShader)(GLuint shader);
        extern GLuint (APIENTRY *glCreateProgram)(void);
        extern GLuint (APIENTRY *glCreateShader)(GLenum shaderType);
        extern void (APIENTRY *glDeleteProgram)(GLuint program);
        extern void (APIENTRY *glDeleteShader)(GLuint shader);
        extern void (APIENTRY *glDetachShader)(GLuint program, GLuint shader);
        extern void (APIENTRY *glGetProgramiv)(GLuint program, GLenum pname, GLint *params);
        extern void (APIENTRY *glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
        extern void (APIENTRY *glGetShaderInfoLog)(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
        extern GLint (APIENTRY *glGetUniformLocation)(GLuint program, const GLchar *name);
        extern GLboolean (APIENTRY *glIsShader)(GLuint shader);
        extern void (APIENTRY *glLinkProgram)(GLuint program);
        extern void (APIENTRY *glShaderSource)(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
        extern void (APIENTRY *glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
        extern void (APIENTRY *glUseProgram)(GLuint program);

        extern void (APIENTRY *glDebugMessageCallback)(DEBUGPROC callback, void *userParam);
    }

    void load_opengl_extensions(void);

}
