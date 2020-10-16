/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render
#include "internal/render/expansion_macros.hpp"

// module render_opengl
#include "internal/render_opengl/glfw_include.hpp"

#define GL_FUNCTIONS    glGetIntegerv, \
                        glGetString, \
                        glGetStringi, \
                        glClear, \
                        glClearColor, \
                        glBlendFunc, \
                        glDepthFunc, \
                        glDisable, \
                        glEnable, \
                        glPixelStore, \
                        glPixelStorei, \
                        glViewport, \
                        glGenFramebuffers, \
                        glBindBuffer, \
                        glBufferData, \
                        glBufferSubData, \
                        glCopyBufferSubData, \
                        glDeleteBuffers, \
                        glGenBuffers, \
                        glIsBuffer, \
                        glMapBuffer, \
                        glUnmapBuffer, \
                        glBindVertexArray, \
                        glDeleteVertexArrays, \
                        glDrawArrays, \
                        glEnableVertexAttribArray, \
                        glGenVertexArrays, \
                        glVertexAttribPointer, \
                        glBindTexture, \
                        glDeleteTextures, \
                        glGenTextures, \
                        glTexImage2D, \
                        glTexParameteri, \
                        glTexSubImage2D, \
                        glAttachShader, \
                        glBindAttribLocation, \
                        glBindFragDataLocation, \
                        glCompileShader, \
                        glCreateProgram, \
                        glCreateShader, \
                        glDeleteProgram, \
                        glDeleteShader, \
                        glDetachShader, \
                        glGetProgramiv, \
                        glGetProgramInfoLog, \
                        glGetShaderiv, \
                        glGetShaderInfoLog, \
                        glGetUniformLocation, \
                        glIsProgram, \
                        glIsShader, \
                        glLinkProgram, \
                        glShaderSource, \
                        glUniformMatrix4fv, \
                        glUseProgram, \
                        glDebugMessageCallback, \
                        glGetError

#define EXPAND_GL_DECLARATION(function) extern PTR_##function function;

#define ARGUS_USE_GL_TRAMPOLINE
#ifdef _WIN32
#define ARGUS_USE_GL_TRAMPOLINE
#endif

typedef void (APIENTRYP DEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);

// utility
typedef GLubyte *(APIENTRYP PTR_glGetIntegerv)(GLenum pname, GLint *data);
typedef GLubyte *(APIENTRYP PTR_glGetString)(GLenum name);
typedef GLubyte *(APIENTRYP PTR_glGetStringi)(GLenum name, GLuint index);

// rendering
typedef void (APIENTRYP PTR_glClear)(GLbitfield mask);
typedef void (APIENTRYP PTR_glClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);

// state
typedef void (APIENTRYP PTR_glBlendFunc)(GLenum sfactor, GLenum dfactor);
typedef void (APIENTRYP PTR_glDepthFunc)(GLenum func);
typedef void (APIENTRYP PTR_glDisable)(GLenum cap);
typedef void (APIENTRYP PTR_glEnable)(GLenum cap);
typedef void (APIENTRYP PTR_glPixelStore)(GLenum pname, GLfloat param);
typedef void (APIENTRYP PTR_glPixelStorei)(GLenum pname, GLint param);
typedef void (APIENTRYP PTR_glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);

// framebuffer
typedef void (APIENTRYP PTR_glGenFramebuffers)(GLsizei n, GLuint* framebuffers);

// buffer
typedef void (APIENTRYP PTR_glBindBuffer)(GLenum target, GLuint buffer);
typedef void (APIENTRYP PTR_glBufferData)(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
typedef void (APIENTRYP PTR_glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
typedef void (APIENTRYP PTR_glCopyBufferSubData)(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
typedef void (APIENTRYP PTR_glDeleteBuffers)(GLsizei n, const GLuint *buffers);
typedef void (APIENTRYP PTR_glDrawArrays)(GLenum mode, GLint first, GLsizei count);
typedef void (APIENTRYP PTR_glGenBuffers)(GLsizei n, GLuint *buffers);
typedef GLboolean (APIENTRYP PTR_glIsBuffer)(GLuint buffer);
typedef void *(APIENTRYP PTR_glMapBuffer)(GLenum target, GLenum access);
typedef GLboolean (APIENTRYP PTR_glUnmapBuffer)(GLenum target);

// vertex array
typedef void (APIENTRYP PTR_glBindVertexArray)(GLuint array);
typedef void (APIENTRYP PTR_glDeleteVertexArrays)(GLsizei n, const GLuint *arrays);
typedef void (APIENTRYP PTR_glEnableVertexAttribArray)(GLuint index);
typedef void (APIENTRYP PTR_glGenVertexArrays)(GLsizei n, GLuint *arrays);
typedef void (APIENTRYP PTR_glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);

// texture
typedef void (APIENTRYP PTR_glBindTexture)(GLenum target, GLuint texture);
typedef void (APIENTRYP PTR_glDeleteTextures)(GLsizei n, const GLuint *textures);
typedef void (APIENTRYP PTR_glGenTextures)(GLsizei n, const GLuint *textures);
typedef void (APIENTRYP PTR_glTexImage2D)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *data);
typedef void (APIENTRYP PTR_glTexParameteri)(GLenum target, GLenum pname, GLint param);
typedef void (APIENTRYP PTR_glTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);

// shader
typedef void (APIENTRYP PTR_glAttachShader)(GLuint program, GLuint shader);
typedef void (APIENTRYP PTR_glBindAttribLocation)(GLuint program, GLuint index, const GLchar *name);
typedef void (APIENTRYP PTR_glBindFragDataLocation)(GLuint program, GLuint colorNumber, const char *name);
typedef void (APIENTRYP PTR_glCompileShader)(GLuint shader);
typedef GLuint (APIENTRYP PTR_glCreateProgram)(void);
typedef GLuint (APIENTRYP PTR_glCreateShader)(GLenum shaderType);
typedef void (APIENTRYP PTR_glDeleteProgram)(GLuint program);
typedef void (APIENTRYP PTR_glDeleteShader)(GLuint shader);
typedef void (APIENTRYP PTR_glDetachShader)(GLuint program, GLuint shader);
typedef void (APIENTRYP PTR_glGetProgramiv)(GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRYP PTR_glGetProgramInfoLog)(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRYP PTR_glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRYP PTR_glGetShaderInfoLog)(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef GLint (APIENTRYP PTR_glGetUniformLocation)(GLuint program, const GLchar *name);
typedef GLboolean (APIENTRYP PTR_glIsProgram)(GLuint program);
typedef GLboolean (APIENTRYP PTR_glIsShader)(GLuint shader);
typedef void (APIENTRYP PTR_glLinkProgram)(GLuint program);
typedef void (APIENTRYP PTR_glShaderSource)(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
typedef void (APIENTRYP PTR_glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRYP PTR_glUseProgram)(GLuint program);

// utility
typedef void (APIENTRYP PTR_glDebugMessageCallback)(DEBUGPROC callback, void *userParam);
typedef GLenum (APIENTRYP PTR_glGetError)(void);

EXPAND_LIST(EXPAND_GL_DECLARATION, GL_FUNCTIONS);

namespace argus {
    void init_opengl_extensions(void);
}
