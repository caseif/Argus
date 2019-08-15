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

    static void _clean_up(void) {
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
        #ifdef USE_GLES
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        #else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        #endif

        load_opengl_extensions();
    }

    static void _gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
            const GLchar *message, void *userParam) {
        char const* severity_str;
        switch (severity) {
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                severity_str = "TRACE";
                break;
            case GL_DEBUG_SEVERITY_LOW:
                severity_str = "INFO";
                break;
            case GL_DEBUG_SEVERITY_MEDIUM:
                severity_str = "WARN";
                break;
            case GL_DEBUG_SEVERITY_HIGH:
                severity_str = "SEVERE";
                break;
        }
        std::cerr << "[GL][" << severity_str << "] " << message << std::endl;
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

        activate_gl_context();
        glClearColor(0, 0, 0, 1);
    }

    Renderer::~Renderer(void) = default;

    void Renderer::destroy(void) {
        SDL_GL_DeleteContext(gl_context);

        delete this;
        return;
    }

    RenderLayer &Renderer::create_render_layer(const int priority) {
        RenderLayer *layer = new RenderLayer(this);
        render_layers.insert(render_layers.cend(), layer);
        return *layer;
    }

    void Renderer::remove_render_layer(RenderLayer &render_layer) {
        remove_from_vector(render_layers, &render_layer);
        delete &render_layer;
        return;
    }

    void Renderer::render(const TimeDelta delta) {
        //TODO: account for priorities
        for (RenderLayer *layer : render_layers) {
            layer->render();
        }

        SDL_GL_SwapWindow(window.handle);
    }

    void Renderer::activate_gl_context(void) const {
        SDL_GL_MakeCurrent(static_cast<SDL_Window*>(window.handle), gl_context);
    }

}
