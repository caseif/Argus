// module core
#include "internal/util.hpp"

// module renderer
#include "internal/glext.hpp"

#include <SDL2/SDL_video.h>

namespace argus {

    namespace glext {
        void (*glGenFramebuffers)(GLsizei n, GLuint* framebuffers);
        void (*glFramebufferTexture)(GLenum target, GLenum attachment, GLuint texture, GLint level);
    }

    template <typename FunctionType>
    static void _load_opengl_extension(const char *func_name, FunctionType *target) {
        void *function = SDL_GL_GetProcAddress(func_name);
        if (!function) {
            FATAL("Failed to load OpenGL extension: %s\n", func_name);
        } else {
        }
        *target = reinterpret_cast<FunctionType>(function);
    }

    void load_opengl_extensions(void) {
        SDL_GL_LoadLibrary(NULL);

        _load_opengl_extension<>("glGenFramebuffers", &glext::glGenFramebuffers);
        _load_opengl_extension<>("glFramebufferTexture", &glext::glFramebufferTexture);
    }

}