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

    std::vector<Window*> g_windows;
    size_t g_window_count = 0;

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
            _ARGUS_FATAL("Failed to initialize SDL video\n");
        }

        _init_opengl();

        g_renderer_initialized = true;
    }

    Renderer::Renderer(Window &window):
            window(window) {
        _ARGUS_ASSERT(g_renderer_initialized, "Cannot create renderer before module is initialized.");

        gl_context = SDL_GL_CreateContext(static_cast<SDL_Window*>(window.handle));
    }

    Renderer::~Renderer(void) = default;

    void Renderer::destroy(void) {
        SDL_GL_DeleteContext(gl_context);

        delete this;
        return;
    }

    RenderLayer &Renderer::create_render_layer(int priority) {
        RenderLayer *layer = new RenderLayer(this);
        render_layers.insert(render_layers.cend(), layer);
        return *layer;
    }

    void Renderer::remove_render_layer(RenderLayer &render_layer) {
        remove_from_vector(render_layers, &render_layer);
        delete &render_layer;
        return;
    }

    void Renderer::activate_gl_context(void) const {
        SDL_GL_MakeCurrent(static_cast<SDL_Window*>(window.handle), gl_context);
    }

}
