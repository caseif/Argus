#pragma once

#include "internal/expand_dong.hpp"

#define SDL_MAIN_HANDLED

#include <SDL2/SDL_opengl.h>

#define GL_FUNCTIONS    glFramebufferTexture, \
                        glGenFramebuffers, \
                        glBindBuffer, \
                        glBufferData, \
                        glBufferSubData, \
                        glDeleteBuffers, \
                        glGenBuffers, \
                        glBindVertexArray, \
                        glDeleteVertexArrays, \
                        glEnableVertexAttribArray, \
                        glGenVertexArrays, \
                        glVertexAttribPointer, \
                        glAttachShader, \
                        glBindAttribLocation, \
                        glCompileShader, \
                        glCreateProgram, \
                        glCreateShader, \
                        glDeleteProgram, \
                        glDeleteShader, \
                        glDetachShader, \
                        glGetProgramiv, \
                        glGetShaderiv, \
                        glGetShaderInfoLog, \
                        glGetUniformLocation, \
                        glIsShader, \
                        glLinkProgram, \
                        glShaderSource, \
                        glUniformMatrix4fv, \
                        glUseProgram, \
                        glDebugMessageCallback

#define EXPAND_GL_DECLARATION(function) extern PTR_##function function;

namespace argus {

    namespace glext {
        typedef void (APIENTRY *DEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                const GLchar *message, void *userParam);

        typedef void (APIENTRY *PTR_glFramebufferTexture)(GLenum target, GLenum attachment, GLuint texture, GLint level);
        typedef void (APIENTRY *PTR_glGenFramebuffers)(GLsizei n, GLuint* framebuffers);

        typedef void (APIENTRY *PTR_glBindBuffer)(GLenum target, GLuint buffer);
        typedef void (APIENTRY *PTR_glBufferData)(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
        typedef void (APIENTRY *PTR_glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
        typedef void (APIENTRY *PTR_glDeleteBuffers)(GLsizei n, const GLuint *buffers);
        typedef void (APIENTRY *PTR_glGenBuffers)(GLsizei n, GLuint *buffers);

        typedef void (APIENTRY *PTR_glBindVertexArray)(GLuint array);
        typedef void (APIENTRY *PTR_glDeleteVertexArrays)(GLsizei n, const GLuint *arrays);
        typedef void (APIENTRY *PTR_glEnableVertexAttribArray)(GLuint index);
        typedef void (APIENTRY *PTR_glGenVertexArrays)(GLsizei n, GLuint *arrays);
        typedef void (APIENTRY *PTR_glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);

        typedef void (APIENTRY *PTR_glAttachShader)(GLuint program, GLuint shader);
        typedef void (APIENTRY *PTR_glBindAttribLocation)(GLuint program, GLuint index, const GLchar *name);
        typedef void (APIENTRY *PTR_glCompileShader)(GLuint shader);
        typedef GLuint (APIENTRY *PTR_glCreateProgram)(void);
        typedef GLuint (APIENTRY *PTR_glCreateShader)(GLenum shaderType);
        typedef void (APIENTRY *PTR_glDeleteProgram)(GLuint program);
        typedef void (APIENTRY *PTR_glDeleteShader)(GLuint shader);
        typedef void (APIENTRY *PTR_glDetachShader)(GLuint program, GLuint shader);
        typedef void (APIENTRY *PTR_glGetProgramiv)(GLuint program, GLenum pname, GLint *params);
        typedef void (APIENTRY *PTR_glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
        typedef void (APIENTRY *PTR_glGetShaderInfoLog)(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
        typedef GLint (APIENTRY *PTR_glGetUniformLocation)(GLuint program, const GLchar *name);
        typedef GLboolean (APIENTRY *PTR_glIsShader)(GLuint shader);
        typedef void (APIENTRY *PTR_glLinkProgram)(GLuint program);
        typedef void (APIENTRY *PTR_glShaderSource)(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
        typedef void (APIENTRY *PTR_glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
        typedef void (APIENTRY *PTR_glUseProgram)(GLuint program);

        typedef void (APIENTRY *PTR_glDebugMessageCallback)(DEBUGPROC callback, void *userParam);

        EXPAND_LIST(EXPAND_GL_DECLARATION, GL_FUNCTIONS);
    }

    void init_opengl_extensions(void);

    #ifdef _WIN32
    void load_gl_extensions_for_context(SDL_GLContext ctx);
    #endif

}
