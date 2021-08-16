/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/memory.hpp"
#include "argus/lowlevel/threading.hpp"
#include "argus/lowlevel/time.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "internal/core/core_util.hpp"

// module resman
#include "argus/resman.hpp"

// module wm
#include "argus/wm/window.hpp"
#include "internal/wm/pimpl/window.hpp"

// module render
#include "argus/render/common/material.hpp"
#include "argus/render/common/render_layer.hpp"
#include "argus/render/common/renderer.hpp"
#include "argus/render/common/shader.hpp"
#include "argus/render/common/texture_data.hpp"
#include "argus/render/common/transform.hpp"
#include "internal/render/defines.hpp"
#include "internal/render/renderer_impl.hpp"
#include "internal/render/pimpl/common/material.hpp"
#include "internal/render/pimpl/common/renderer.hpp"
#include "internal/render/pimpl/common/shader.hpp"
#include "internal/render/pimpl/common/texture_data.hpp"
#include "internal/render/pimpl/common/transform_2d.hpp"

// module render_opengl
#include "internal/render_opengl/defines.hpp"
#include "internal/render_opengl/gl_util.hpp"
#include "internal/render_opengl/glfw_include.hpp"
#include "internal/render_opengl/types.hpp"
#include "internal/render_opengl/renderer/compositing.hpp"
#include "internal/render_opengl/renderer/gl_renderer_base.hpp"
#include "internal/render_opengl/renderer/gl_renderer_2d.hpp"
#include "internal/render_opengl/renderer/shader_mgmt.hpp"
#include "internal/render_opengl/renderer/texture_mgmt.hpp"
#include "internal/render_opengl/state/layer_state.hpp"
#include "internal/render_opengl/state/processed_render_object.hpp"
#include "internal/render_opengl/state/render_bucket.hpp"
#include "internal/render_opengl/state/renderer_state.hpp"

#include "aglet/aglet.h"

#include <algorithm>
#include <atomic>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <cstdbool>
#include <cstdio>
#include <cstring>

namespace argus {
    // forward declarations
    class RenderLayer2D;

    GLRenderer::GLRenderer(void): RendererImpl() {
    }

    static void _rebuild_scene(RendererState &state) {
        for (auto *layer : state.renderer.pimpl->render_layers) {
            LayerState &layer_state = state.get_layer_state(*layer, true);

            auto &layer_transform = layer->get_transform();
            if (layer_transform.pimpl->dirty) {
                multiply_matrices(g_view_matrix, layer_transform.as_matrix(), layer_state.view_matrix);
                layer_transform.pimpl->dirty = false;
            }

            render_layer_2d(reinterpret_cast<RenderLayer2D&>(*layer), state,
                    reinterpret_cast<Layer2DState&>(layer_state));

            for (auto bucket_it : layer_state.render_buckets) {
                auto &mat = bucket_it.second->material_res;

                build_shaders(state, mat);

                prepare_texture(state, mat);
            }
        }
    }

    static void _deinit_material(RendererState &state, const std::string &material) {
        _ARGUS_DEBUG("De-initializing material %s\n", material.c_str());
        for (auto *layer_state : state.all_layer_states) {
            auto &buckets = layer_state->render_buckets;
            auto bucket_it = buckets.find(material);
            if (bucket_it != buckets.end()) {
                try_delete_buffer(bucket_it->second->vertex_array);
                try_delete_buffer(bucket_it->second->vertex_buffer);
                bucket_it->second->~RenderBucket();
                buckets.erase(bucket_it);
            }
        }

        auto &programs = state.linked_programs;
        auto program_it = programs.find(material);
        if (program_it != programs.end()) {
            deinit_program(program_it->second.handle);
        }

        programs.erase(program_it);
    }

    static void _handle_resource_event(const ArgusEvent &base_event, void *renderer_state) {
        auto &event = static_cast<const ResourceEvent&>(base_event);
        if (event.subtype != ResourceEventType::Unload) {
            return;
        }

        auto &state = *static_cast<RendererState*>(renderer_state);

        std::string mt = event.prototype.media_type;
        if (mt == RESOURCE_TYPE_TEXTURE_PNG) {
            deinit_texture(state, event.prototype.uid);
        } else if (mt == RESOURCE_TYPE_SHADER_GLSL_VERT || mt == RESOURCE_TYPE_SHADER_GLSL_FRAG) {
            deinit_shader(state, event.prototype.uid);
        } else if (mt == RESOURCE_TYPE_MATERIAL) {
            _deinit_material(state, event.prototype.uid);
        }
    }

    void GLRenderer::init(Renderer &renderer) {
        activate_gl_context(renderer.pimpl->window.pimpl->handle);

        agletLoad(reinterpret_cast<AgletLoadProc>(glfwGetProcAddress));

        int gl_major;
        int gl_minor;
        const unsigned char *gl_version_str = glGetString(GL_VERSION);
        glGetIntegerv(GL_MAJOR_VERSION, &gl_major);
        glGetIntegerv(GL_MINOR_VERSION, &gl_minor);
        if (!AGLET_GL_VERSION_3_3) {
            _ARGUS_FATAL("Argus requires support for OpenGL 3.3 or higher (got %d.%d)\n", gl_major, gl_minor);
        }

        _ARGUS_INFO("Obtained OpenGL %d.%d context (%s)\n", gl_major, gl_minor, gl_version_str);

        auto &state = renderer_states.insert({ &renderer, RendererState(renderer) }).first->second;

        resource_event_handler = register_event_handler(ArgusEventType::Resource, _handle_resource_event,
                TargetThread::Render, &state);

        if (AGLET_GL_KHR_debug) {
            glDebugMessageCallback(gl_debug_callback, nullptr);
        }

        setup_framebuffer(get_renderer_state(renderer));
    }

    void GLRenderer::deinit(Renderer &renderer) {
        unregister_event_handler(resource_event_handler);

        get_renderer_state(renderer).~RendererState();
    }

    void GLRenderer::render(Renderer &renderer, const TimeDelta delta) {
        UNUSED(delta);
        auto &state = get_renderer_state(renderer);

        activate_gl_context(renderer.pimpl->window.pimpl->handle);

        _rebuild_scene(state);

        // set up state for drawing scene to framebuffers
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDisable(GL_CULL_FACE);

        for (auto *layer : renderer.pimpl->render_layers) {
            auto &layer_state = state.get_layer_state(*layer);
            draw_layer_to_framebuffer(layer_state);
        }

        // set up state for drawing framebuffers to screen

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        for (auto *layer : renderer.pimpl->render_layers) {
            auto &layer_state = state.get_layer_state(*layer);

            draw_framebuffer_to_screen(layer_state);
        }

        glfwSwapBuffers(renderer.pimpl->window.pimpl->handle);
    }

    RendererState &GLRenderer::get_renderer_state(Renderer &renderer) {
        auto it = renderer_states.find(&renderer);
        _ARGUS_ASSERT(it != renderer_states.cend(), "Cannot find renderer state");
        return it->second;
    }
}
