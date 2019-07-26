// module core
#include "argus/core.hpp"
#include "internal/util.hpp"

// module renderer
#include "argus/renderer.hpp"
#include "internal/glext.hpp"

#include <algorithm>
#include <iostream>
#include <vector>
#include <SDL2/SDL_render.h>

namespace argus {

    bool g_renderer_initialized = false;

    extern std::vector<Window*> g_windows;

    void _clean_up(void) {
        // use a copy since Window::destroy modifies the global list
        auto windows_copy = g_windows;
        // doing this in reverse ensures that child windows are destroyed before their parents
        for (std::vector<Window*>::reverse_iterator it = windows_copy.rbegin();
                it != windows_copy.rend(); it++) { 
            (*it)->destroy();
        }

        SDL_VideoQuit();

        return;
    }

    static void _init_opengl(void) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

        load_opengl_extensions();
    }

    void init_module_renderer(void) {
        register_close_callback(_clean_up);

        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            FATAL("Failed to initialize SDL video\n");
        }

        _init_opengl();

        g_renderer_initialized = true;
    }

    Renderer::Renderer(Window *window) {
        ASSERT(g_renderer_initialized, "Cannot create renderer before module is initialized.");

        handle = SDL_CreateRenderer(window->handle, -1, SDL_RENDERER_ACCELERATED);
    }

    Renderer::~Renderer(void) = default;

    void Renderer::destroy(void) {
        SDL_DestroyRenderer(handle);

        delete this;
        return;
    }

}
