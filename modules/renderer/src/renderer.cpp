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

    static std::vector<Renderer*> g_renderers;

    extern std::vector<Window*> g_windows;

    void _clean_up(void) {
        // use a copy since Renderer::destroy modifies the global list
        auto renderers_copy = g_renderers;
        for (Renderer *renderer : renderers_copy) {
            renderer->destroy();
        }

        // same here with using a copy
        auto windows_copy = g_windows;
        for (Window *window : windows_copy) {
            window->destroy();
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

        g_renderers.insert(g_renderers.cend(), this);
    }

    Renderer::~Renderer(void) = default;

    void Renderer::destroy(void) {
        SDL_DestroyRenderer(handle);

        g_renderers.erase(std::remove(g_renderers.begin(), g_renderers.end(), this));

        delete this;
        return;
    }

}
