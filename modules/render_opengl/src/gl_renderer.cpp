/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/threading.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core.hpp"
#include "internal/core/core_util.hpp"

// module render
#include "argus/render/render_layer.hpp"
#include "argus/render/renderer.hpp"
#include "argus/render/texture_data.hpp"
#include "argus/render/transform.hpp"
#include "argus/render/window.hpp"
#include "internal/render/pimpl/render_layer.hpp"
#include "internal/render/pimpl/renderer.hpp"
#include "internal/render/pimpl/texture_data.hpp"
#include "internal/render/pimpl/window.hpp"
#include "internal/render/renderer_impl.hpp"

// module render_opengl
#include "internal/render_opengl/buffered_texture.hpp"
#include "internal/render_opengl/gl_renderer.hpp"
#include "internal/render_opengl/glext.hpp"
#include "internal/render_opengl/glfw_include.hpp"

#include <algorithm>
#include <atomic>
#include <stdexcept>
#include <vector>

#include <cstdio>

namespace argus {

    using namespace glext;

    std::map<const TextureData*, BufferedTexture> g_buffered_textures;

    static void _activate_gl_context(GLFWwindow *window) {
        if (glfwGetCurrentContext() == window) {
            // already current
            return;
        }

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
        char const *level;
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

    GLRenderer::GLRenderer(void): RendererImpl() {
    }

    void GLRenderer::init_context_hints(void) {
#ifdef USE_GLES
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#else
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    }

    // we do the init in a separate method so the GL context is always created from the render thread
    void GLRenderer::init(Renderer &renderer) {
        _activate_gl_context(renderer.pimpl->window.pimpl->handle);

        init_opengl_extensions();

        const GLubyte *ver_str = glGetString(GL_VERSION);
        _ARGUS_DEBUG("Obtained context with version %s\n", ver_str);

        //TODO: actually do something
        if (glDebugMessageCallback != nullptr) {
            glDebugMessageCallback(_gl_debug_callback, nullptr);
        }

        glDepthFunc(GL_ALWAYS);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    BufferedTexture &_as_buffered_texture(Renderer &renderer, const TextureData &texture) {
        auto existing = g_buffered_textures.find(&texture);
        if (existing != g_buffered_textures.end()) {
            return existing->second;
        }

        auto buffered = BufferedTexture(texture.width, texture.height);

        glGenBuffers(1, &buffered.gl_handle);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffered.gl_handle);

        if (!glIsBuffer(buffered.gl_handle)) {
            _ARGUS_FATAL("Failed to gen pixel buffer during texture preparation\n");
        }

        size_t row_size = texture.width * 32 / 8;
        glBufferData(GL_PIXEL_UNPACK_BUFFER, texture.height * row_size, nullptr, GL_STREAM_COPY);

        size_t offset = 0;
        for (size_t y = 0; y < texture.height; y++) {
            glBufferSubData(GL_PIXEL_UNPACK_BUFFER, offset, row_size, texture.pimpl->image_data[y]);
            offset += row_size;
        }

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        auto inserted = g_buffered_textures.insert({ &texture, buffered });
        return inserted.first->second;
    }

    void GLRenderer::deinit_texture(Renderer &renderer, const TextureData &texture) {
        auto buffered = g_buffered_textures.find(&texture);
        if (buffered == g_buffered_textures.end()) {
            return;
        }

        glDeleteBuffers(1, &buffered->second.gl_handle);
    }

    void GLRenderer::render(Renderer &renderer, const TimeDelta delta) {
        _activate_gl_context(renderer.pimpl->window.pimpl->handle);

        if (renderer.pimpl->window.pimpl->dirty_resolution) {
            Vector2u res = renderer.pimpl->window.pimpl->properties.resolution;
            glViewport(0, 0, res.x, res.y);
            renderer.pimpl->window.pimpl->dirty_resolution = false;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (RenderLayer *layer : renderer.pimpl->render_layers) {
            //TODO
        }

        glfwSwapBuffers(renderer.pimpl->window.pimpl->handle);
    }
}
