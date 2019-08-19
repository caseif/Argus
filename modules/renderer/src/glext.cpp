// module core
#include "internal/util.hpp"

// module renderer
#include "internal/glext.hpp"

#include <SDL2/SDL_video.h>

namespace argus {

    namespace glext {
        void (APIENTRY *glFramebufferTexture)(GLenum target, GLenum attachment, GLuint texture, GLint level);
        void (APIENTRY *glGenFramebuffers)(GLsizei n, GLuint* framebuffers);

        void (APIENTRY *glBindBuffer)(GLenum target, GLuint buffer);
        void (APIENTRY *glBufferData)(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
        void (APIENTRY *glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
        void (APIENTRY *glDeleteBuffers)(GLsizei n, const GLuint *buffers);
        void (APIENTRY *glGenBuffers)(GLsizei n, GLuint *buffers);

        void (APIENTRY *glBindVertexArray)(GLuint array);
        void (APIENTRY *glDeleteVertexArrays)(GLsizei n, const GLuint *arrays);
        void (APIENTRY *glEnableVertexAttribArray)(GLuint index);
        void (APIENTRY *glGenVertexArrays)(GLsizei n, GLuint *arrays);
        void (APIENTRY *glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);

        void (APIENTRY *glAttachShader)(GLuint program, GLuint shader);
        void (APIENTRY *glBindAttribLocation)(GLuint program, GLuint index, const GLchar *name);
        void (APIENTRY *glCompileShader)(GLuint shader);
        GLuint (APIENTRY *glCreateProgram)(void);
        GLuint (APIENTRY *glCreateShader)(GLenum shaderType);
        void (APIENTRY *glDeleteProgram)(GLuint program);
        void (APIENTRY *glDeleteShader)(GLuint shader);
        void (APIENTRY *glDetachShader)(GLuint program, GLuint shader);
        void (APIENTRY *glGetProgramiv)(GLuint program, GLenum pname, GLint *params);
        void (APIENTRY *glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
        void (APIENTRY *glGetShaderInfoLog)(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
        GLint (APIENTRY *glGetUniformLocation)(GLuint program, const GLchar *name);
        GLboolean (APIENTRY *glIsShader)(GLuint shader);
        void (APIENTRY *glLinkProgram)(GLuint program);
        void (APIENTRY *glShaderSource)(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
        void (APIENTRY *glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
        void (APIENTRY *glUseProgram)(GLuint program);

        void (APIENTRY *glDebugMessageCallback)(DEBUGPROC callback, void *userParam);
    }

    template <typename FunctionType>
    static void _load_gl_ext(const char *const func_name, FunctionType *target) {
        //TODO: verify the extension for each given function is supported
        void *function = SDL_GL_GetProcAddress(func_name);

        if (!function) {
            _ARGUS_FATAL("Failed to load OpenGL extension: %s\n", func_name);
        }

        *target = reinterpret_cast<FunctionType>(function);
    }

    void load_opengl_extensions(void) {
        SDL_GL_LoadLibrary(NULL);

        using namespace glext;

        _load_gl_ext<>("glGenFramebuffers", &glGenFramebuffers);
        _load_gl_ext<>("glFramebufferTexture", &glFramebufferTexture);

        _load_gl_ext<>("glBindBuffer", &glBindBuffer);
        _load_gl_ext<>("glBufferData", &glBufferData);
        _load_gl_ext<>("glBufferSubData", &glBufferSubData);
        _load_gl_ext<>("glDeleteBuffers", &glDeleteBuffers);
        _load_gl_ext<>("glGenBuffers", &glGenBuffers);

        _load_gl_ext<>("glGenVertexArrays", &glGenVertexArrays);
        _load_gl_ext<>("glDeleteVertexArrays", &glDeleteVertexArrays);
        _load_gl_ext<>("glBindVertexArray", &glBindVertexArray);
        _load_gl_ext<>("glEnableVertexAttribArray", &glEnableVertexAttribArray);
        _load_gl_ext<>("glVertexAttribPointer", &glVertexAttribPointer);

        _load_gl_ext<>("glAttachShader", &glAttachShader);
        _load_gl_ext<>("glBindAttribLocation", &glBindAttribLocation);
        _load_gl_ext<>("glCompileShader", &glCompileShader);
        _load_gl_ext<>("glCreateProgram", &glCreateProgram);
        _load_gl_ext<>("glCreateShader", &glCreateShader);
        _load_gl_ext<>("glDeleteProgram", &glDeleteProgram);
        _load_gl_ext<>("glDeleteShader", &glDeleteShader);
        _load_gl_ext<>("glDetachShader", &glDetachShader);
        _load_gl_ext<>("glGetProgramiv", &glGetProgramiv);
        _load_gl_ext<>("glGetShaderiv", &glGetShaderiv);
        _load_gl_ext<>("glGetShaderInfoLog", &glGetShaderInfoLog);
        _load_gl_ext<>("glGetUniformLocation", &glGetUniformLocation);
        _load_gl_ext<>("glIsShader", &glIsShader);
        _load_gl_ext<>("glLinkProgram", &glLinkProgram);
        _load_gl_ext<>("glShaderSource", &glShaderSource);
        _load_gl_ext<>("glUniformMatrix4fv", &glUniformMatrix4fv);
        _load_gl_ext<>("glUseProgram", &glUseProgram);

        _load_gl_ext<>("glDebugMessageCallbackARB", &glDebugMessageCallback);
    }

}