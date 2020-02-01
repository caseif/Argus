/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/lowlevel/logging.hpp"
#include "argus/math.hpp"
#include "argus/threading.hpp"

// module core
#include "argus/core.hpp"
#include "internal/core/core_util.hpp"

// module renderer
#include "argus/renderer/render_layer.hpp"
#include "argus/renderer/renderer.hpp"
#include "argus/renderer/window.hpp"
#include "internal/renderer/glext.hpp"
#include "internal/renderer/types.hpp"
#include "internal/renderer/pimpl/render_layer.hpp"
#include "internal/renderer/pimpl/renderer.hpp"
#include "internal/renderer/pimpl/window.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <atomic>
#include <functional>
#include <vector>

#include <cstdio>

namespace argus {

    using namespace glext;

    extern bool g_renderer_initialized;

    static void _activate_gl_context(window_handle_t window) {
        glfwMakeContextCurrent(window);
        if (glfwGetCurrentContext() != window) {
             _ARGUS_FATAL("Failed to make GL context current\n");
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

    Renderer::Renderer(Window &window): pimpl(new pimpl_Renderer(window)) {
        _ARGUS_ASSERT(g_renderer_initialized, "Cannot create renderer before module is initialized.\n");

        pimpl->window = window;
        pimpl->dirty_resolution = false;
    }

    // we do the init in a separate method so the GL context is always created from the render thread
    void Renderer::init(void) {
        _activate_gl_context(pimpl->window.pimpl->handle);

        const GLubyte *ver_str = glGetString(GL_VERSION);
        _ARGUS_DEBUG("Obtained context with version %s\n", ver_str);

        //TODO: load GL library on Windows?

        glDebugMessageCallback(_gl_debug_callback, nullptr);

        glDepthFunc(GL_ALWAYS);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    Renderer::~Renderer(void) = default;

    void Renderer::destroy(void) {
        delete pimpl;

        return;
    }

    RenderLayer &Renderer::create_render_layer(const int priority) {
        RenderLayer *layer = new RenderLayer(*this, priority);
        pimpl->render_layers.insert(pimpl->render_layers.cend(), layer);
        std::sort(pimpl->render_layers.begin(), pimpl->render_layers.end(), [](auto a, auto b) {
            return a->pimpl->priority < b->pimpl->priority;
        });
        return *layer;
    }

    void Renderer::remove_render_layer(RenderLayer &render_layer) {
        _ARGUS_ASSERT(&render_layer.pimpl->parent_renderer == this,
        "remove_render_layer called on RenderLayer with different parent");

        remove_from_vector(pimpl->render_layers, &render_layer);
        delete &render_layer;

        return;
    }

    void Renderer::render(const TimeDelta delta) {
        _activate_gl_context(pimpl->window.pimpl->handle);

        if (pimpl->dirty_resolution) {
            Vector2u res = pimpl->window.pimpl->properties.resolution;
            glViewport(0, 0, res.x, res.y);
            pimpl->dirty_resolution = false;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (RenderLayer *layer : pimpl->render_layers) {
            layer->render();
        }

        glfwSwapBuffers(pimpl->window.pimpl->handle);
    }

}
