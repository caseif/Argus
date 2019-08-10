// module core
#include "internal/util.hpp"

// module renderer
#include "internal/glext.hpp"

#include <SDL2/SDL_video.h>

namespace argus {

    namespace glext {
        void (*glFramebufferTexture)(GLenum target, GLenum attachment, GLuint texture, GLint level);
        void (*glGenFramebuffers)(GLsizei n, GLuint* framebuffers);

        void (*glBindBuffer)(GLenum target, GLuint buffer);
        void (*glBufferData)(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
        void (*glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
        void (*glGenBuffers)(GLsizei n, GLuint *buffers);

        void (*glBindVertexArray)(GLuint array);
        void (*glDeleteVertexArrays)(GLsizei n, const GLuint *arrays);
        void (*glEnableVertexAttribArray)(GLuint index);
        void (*glGenVertexArrays)(GLsizei n, GLuint *arrays);
        void (*glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride);

        void (*glAttachShader)(GLuint program, GLuint shader);
        void (*glCompileShader)(GLuint shader);
        GLuint (*glCreateProgram)(void);
        GLuint (*glCreateShader)(GLenum shaderType);
        void (*glDeleteProgram)(GLuint program);
        void (*glDeleteShader)(GLuint shader);
        void (*glDetachShader)(GLuint program, GLuint shader);
        void (*glGetProgramiv)(GLuint program, GLenum pname, GLint *params);
        void (*glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
        void (*glGetShaderInfoLog)(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
        GLint (*glGetUniformLocation)(GLuint program, const GLchar *name);
        void (*glLinkProgram)(GLuint program);
        void (*glShaderSource)(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
    }

    template <typename FunctionType>
    static void _load_gl_ext(const char *const func_name, FunctionType *target) {
        void *function = SDL_GL_GetProcAddress(func_name);
        if (!function) {
            _ARGUS_FATAL("Failed to load OpenGL extension: %s\n", func_name);
        } else {
        }
        *target = reinterpret_cast<FunctionType>(function);
    }

    void load_opengl_extensions(void) {
        SDL_GL_LoadLibrary(NULL);

        _load_gl_ext<>("glGenFramebuffers", &glext::glGenFramebuffers);
        _load_gl_ext<>("glFramebufferTexture", &glext::glFramebufferTexture);

        _load_gl_ext<>("glGenBuffers", &glext::glGenBuffers);
        _load_gl_ext<>("glBindBuffer", &glext::glBindBuffer);
        _load_gl_ext<>("glBufferData", &glext::glBufferData);
        _load_gl_ext<>("glBufferSubData", &glext::glBufferSubData);

        _load_gl_ext<>("glGenVertexArrays", &glext::glGenVertexArrays);
        _load_gl_ext<>("glDeleteVertexArrays", &glext::glDeleteVertexArrays);
        _load_gl_ext<>("glBindVertexArray", &glext::glBindVertexArray);
        _load_gl_ext<>("glEnableVertexAttribArray", &glext::glEnableVertexAttribArray);
        _load_gl_ext<>("glVertexAttribPointer", &glext::glVertexAttribPointer);

        _load_gl_ext<>("glAttachShader", &glext::glAttachShader);
        _load_gl_ext<>("glCompileShader", &glext::glCompileShader);
        _load_gl_ext<>("glCreateProgram", &glext::glCreateProgram);
        _load_gl_ext<>("glCreateShader", &glext::glCreateShader);
        _load_gl_ext<>("glDeleteProgram", &glext::glDeleteProgram);
        _load_gl_ext<>("glDeleteShader", &glext::glDeleteShader);
        _load_gl_ext<>("glDetachShader", &glext::glDetachShader);
        _load_gl_ext<>("glGetProgramiv", &glext::glGetProgramiv);
        _load_gl_ext<>("glGetShaderiv", &glext::glGetShaderiv);
        _load_gl_ext<>("glGetShaderInfoLog", &glext::glGetShaderInfoLog);
        _load_gl_ext<>("glGetUniformLocation", &glext::glGetUniformLocation);
        _load_gl_ext<>("glLinkProgram", &glext::glLinkProgram);
        _load_gl_ext<>("glShaderSource", &glext::glShaderSource);
    }

}