// module core
#include "internal/util.hpp"

// module renderer
#include "internal/glext.hpp"

#include <SDL2/SDL_video.h>

namespace argus {

    template <typename FunctionType>
    static void _load_opengl_extension(const char *func_name, FunctionType *target) {
        void *function = SDL_GL_GetProcAddress(func_name);
        if (!function) {
            FATAL("Failed to load OpenGL extension: %s\n", func_name);
        }
        *target = reinterpret_cast<FunctionType>(function);
    }

    void load_opengl_extensions(void) {
        _load_opengl_extension<>("glGenFramebuffers", &glGenFramebuffers);
    }

}