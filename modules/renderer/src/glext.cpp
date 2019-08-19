// module core
#include "internal/util.hpp"

// module renderer
#include "internal/glext.hpp"

#include <map>
#include <SDL2/SDL_video.h>

namespace argus {

    namespace glext {
        PTR_glFramebufferTexture glFramebufferTexture;
        PTR_glGenFramebuffers glGenFramebuffers;

        PTR_glBindBuffer glBindBuffer;
        PTR_glBufferData glBufferData;
        PTR_glBufferSubData glBufferSubData;
        PTR_glDeleteBuffers glDeleteBuffers;
        PTR_glGenBuffers glGenBuffers;

        PTR_glBindVertexArray glBindVertexArray;
        PTR_glDeleteVertexArrays glDeleteVertexArrays;
        PTR_glEnableVertexAttribArray glEnableVertexAttribArray;
        PTR_glGenVertexArrays glGenVertexArrays;
        PTR_glVertexAttribPointer glVertexAttribPointer;

        PTR_glAttachShader glAttachShader;
        PTR_glBindAttribLocation glBindAttribLocation;
        PTR_glCompileShader glCompileShader;
        PTR_glCreateProgram glCreateProgram;
        PTR_glCreateShader glCreateShader;
        PTR_glDeleteProgram glDeleteProgram;
        PTR_glDeleteShader glDeleteShader;
        PTR_glDetachShader glDetachShader;
        PTR_glGetProgramiv glGetProgramiv;
        PTR_glGetShaderiv glGetShaderiv;
        PTR_glGetShaderInfoLog glGetShaderInfoLog;
        PTR_glGetUniformLocation glGetUniformLocation;
        PTR_glIsShader glIsShader;
        PTR_glLinkProgram glLinkProgram;
        PTR_glShaderSource glShaderSource;
        PTR_glUniformMatrix4fv glUniformMatrix4fv;
        PTR_glUseProgram glUseProgram;

        PTR_glDebugMessageCallback glDebugMessageCallback;
    }

    template <typename FunctionSpec>
    static void _load_gl_ext(const char *const func_name, FunctionSpec *target) {
        //TODO: verify the extension for each given function is supported
        void *function = SDL_GL_GetProcAddress(func_name);

        if (!function) {
            _ARGUS_FATAL("Failed to load OpenGL extension: %s\n", func_name);
        }

        *target = reinterpret_cast<FunctionSpec>(function);
    }

    void load_opengl_extensions(void) {
        SDL_GL_LoadLibrary(NULL);

        using namespace glext;

        _load_gl_ext<>("glFramebufferTexture", &glFramebufferTexture);
        _load_gl_ext<>("glGenFramebuffers", &glGenFramebuffers);

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