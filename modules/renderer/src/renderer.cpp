#include "argus/core.hpp"
#include "argus/renderer.hpp"
#include "internal/util.hpp"

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

    void init_module_renderer() {
        register_close_callback(_clean_up);

        g_renderer_initialized = true;
    }

    Renderer::Renderer(Window *window) {
        ASSERT(g_renderer_initialized, "Cannot create renderer before module is initialized.");

        handle = SDL_CreateRenderer(window->get_sdl_window(), -1, SDL_RENDERER_ACCELERATED);

        g_renderers.insert(g_renderers.cend(), this);
    }

    Renderer::~Renderer(void) = default;

    void Renderer::destroy(void) {
        SDL_DestroyRenderer(handle);

        g_renderers.erase(std::remove(g_renderers.begin(), g_renderers.end(), this));

        delete this;
        return;
    }

    SDL_Renderer *Renderer::get_sdl_renderer(void) {
        return handle;
    }

    RenderObject::RenderObject(void) {
        //TODO
    }

    void RenderObject::render_to(RenderObject &other) {
        other.queue_render_object(*this);
    }

    void RenderObject::queue_render_object(RenderObject child) {
        child.transform.set_parent(this->transform);
        //TODO
    }

    RenderLayer::RenderLayer(void) {
        //TODO
    }

    void RenderLayer::destroy(void) {
        //TODO
        delete this;
    }

}
