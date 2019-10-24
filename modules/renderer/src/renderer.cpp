/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/logging.hpp"

// module core
#include "argus/core.hpp"
#include "internal/core_util.hpp"

// module resman
#include "argus/resource_manager.hpp"

// module renderer
#include "argus/renderer.hpp"
#include "internal/glext.hpp"
#include "internal/renderer_defines.hpp"
#include "internal/texture_loader.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_render.h>

namespace argus {

    using namespace glext;

    extern bool g_renderer_initialized;

    static void _activate_gl_context(window_handle_t window, graphics_context_t ctx) {
        int rc = SDL_GL_MakeCurrent(static_cast<SDL_Window*>(window), ctx);
        if (rc != 0) {
            _ARGUS_FATAL("SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
        }
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

    Renderer::Renderer(Window &window):
            window(window),
            initialized(false),
            destruction_pending(false),
            valid(true),
            dirty_resolution(false) {
        _ARGUS_ASSERT(g_renderer_initialized, "Cannot create renderer before module is initialized.");
        callback_id = register_render_callback(std::bind(&Renderer::render, this, std::placeholders::_1));
    }

    Renderer::Renderer(Renderer &rhs):
            window(rhs.window),
            render_layers(rhs.render_layers),
            initialized(rhs.initialized),
            callback_id(rhs.callback_id),
            destruction_pending(rhs.destruction_pending.load()),
            valid(valid),
            dirty_resolution(false) {
    }

    Renderer::Renderer(Renderer &&rhs):
            window(rhs.window),
            render_layers(std::move(rhs.render_layers)),
            initialized(std::move(initialized)),
            callback_id(rhs.callback_id),
            destruction_pending(rhs.destruction_pending.load()),
            valid(valid),
            dirty_resolution(false) {
    }

    // we do the init in a separate method so the GL context is always created from the render thread
    void Renderer::init(void) {
        gl_context = SDL_GL_CreateContext(static_cast<SDL_Window*>(window.handle));
        if (!gl_context) {
            _ARGUS_FATAL("Failed to create GL context: \"%s\"\n", SDL_GetError());
        }

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

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        initialized = true;
    }

    Renderer::~Renderer(void) = default;

    void Renderer::destroy(void) {
        SDL_GL_DeleteContext(gl_context);

        unregister_render_callback(callback_id);

        valid = false;

        return;
    }

    RenderLayer &Renderer::create_render_layer(const int priority) {
        RenderLayer *layer = new RenderLayer(*this, priority);
        render_layers.insert(render_layers.cend(), layer);
        std::sort(render_layers.begin(), render_layers.end(), [](auto a, auto b) {return a->priority < b->priority;});
        return *layer;
    }

    void Renderer::remove_render_layer(RenderLayer &render_layer) {
        _ARGUS_ASSERT(&render_layer.parent_renderer == this, "remove_render_layer called on RenderLayer with different parent");

        remove_from_vector(render_layers, &render_layer);
        delete &render_layer;

        return;
    }

    void Renderer::render(const TimeDelta delta) {
        // there's no contract that guarantees callbacks will be removed immediately, so we need to be able to track
        // when the renderer has been invalidated so we don't try to deinit it more than once
        if (!valid) {
            return;
        }

        // this may be invoked between the parent window being destroyed and the callback being fully unregistered
        if (destruction_pending) {
            destroy();
            return;
        }

        if (!initialized) {
            init();
        }

        _activate_gl_context(window.handle, gl_context);

        if (dirty_resolution) {
            Vector2u res = window.properties.resolution;
            glViewport(0, 0, res.x, res.y);
            dirty_resolution = false;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (RenderLayer *layer : render_layers) {
            layer->render();
        }

        SDL_GL_SwapWindow(static_cast<SDL_Window*>(window.handle));
    }

}
