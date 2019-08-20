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

    using namespace glext;

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
        //SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, SDL_TRUE);
        //SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
        #ifdef _ARGUS_DEBUG_MODE
        //SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG | SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
        #else
        //SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
        #endif

        init_opengl_extensions();
    }

    static void APIENTRY _gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
            const GLchar *message, void *userParam) {
        #ifndef _ARGUS_DEBUG_MODE
        if (severity == GL_DEBUG_SEVERITY_NOTIFICATION || severity == GL_DEBUG_SEVERITY_LOW) {
            return;
        }
        #endif
        char const* level;
        auto stream = stdout;
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:
                level = "SEVERE";
                stream = stderr;
                break;
            case GL_DEBUG_SEVERITY_MEDIUM:
                level = "WARN";
                stream = stderr;
                break;
            case GL_DEBUG_SEVERITY_LOW:
                level = "INFO";
                break;
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                level = "TRACE";
                break;
        }
        _GENERIC_PRINT(stream, level, "GL", "%s\n", message);
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
        const char* sdl_err = SDL_GetError();
        if (sdl_err[0] != '\0') {
            _ARGUS_FATAL("Failed to create GL contextt: \"%s\"\n", sdl_err);
        }

        activate_gl_context();

        int version_major;
        int version_minor;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &version_major);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &version_minor);
        _ARGUS_DEBUG("Obtained context with version %d.%d\n", version_major, version_minor);

        #ifdef _WIN32
        if (SDL_GL_LoadLibrary(nullptr) != 0) {
           _ARGUS_FATAL("Failed to load GL library\n");
        }
        load_gl_extensions_for_current_context();
        #endif

        glDebugMessageCallback(_gl_debug_callback, nullptr);
        
        glDepthFunc(GL_ALWAYS);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    Renderer::~Renderer(void) = default;

    void Renderer::destroy(void) {
        SDL_GL_DeleteContext(gl_context);

        return;
    }

    RenderLayer &Renderer::create_render_layer(const int priority) {
        RenderLayer *layer = new RenderLayer(this);
        render_layers.insert(render_layers.cend(), layer);
        return *layer;
    }

    void Renderer::remove_render_layer(RenderLayer &render_layer) {
        _ARGUS_ASSERT(render_layer.parent_renderer == this, "remove_render_layer called on RenderLayer with different parent");

        remove_from_vector(render_layers, &render_layer);
        delete &render_layer;

        return;
    }

    void Renderer::render(const TimeDelta delta) {
        if (window.invalid) {
            return;
        }

        activate_gl_context();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //TODO: account for priorities
        for (RenderLayer *layer : render_layers) {
            layer->render();
        }
    }

    void Renderer::activate_gl_context(void) const {
        SDL_GL_MakeCurrent(static_cast<SDL_Window*>(window.handle), gl_context);
    }

}
