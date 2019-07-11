#include "argus/core.hpp"
#include "argus/lowlevel.hpp"
#include "argus/renderer.hpp"

#include <algorithm>
#include <iostream>
#include <vector>
#include <SDL2/SDL_render.h>

namespace argus {

    bool g_renderer_initialized = false;

    static std::vector<Renderer*> g_renderers;

    extern std::vector<Window*> g_windows;

    void _clean_up(void) {
        for (Renderer *renderer : g_renderers) {
            renderer->destroy();
        }

        for (Window *window : g_windows) {
            window->destroy();
        }

        SDL_VideoQuit();

        return;
    }

    void init_module_renderer() {
        register_close_callback(_clean_up);

        g_renderer_initialized = true;
    }

    Renderer::Renderer(Window *window) {
        ASSERT(g_renderer_initialized, "Cannot create renderer before module is initialized.");

        handle = SDL_CreateRenderer(window->get_sdl_window(), -1, SDL_RENDERER_ACCELERATED);
    }

    Renderer::~Renderer(void) = default;
    
    void Renderer::destroy(void) {
        g_renderers.erase(std::remove(g_renderers.begin(), g_renderers.end(), this));
        delete this;
        return;
    }

    SDL_Renderer *Renderer::get_sdl_renderer(void) {
        return handle;
    }

}
