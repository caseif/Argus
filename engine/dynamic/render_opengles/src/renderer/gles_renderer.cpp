/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/time.hpp"
#include "internal/lowlevel/logging.hpp"

#include "argus/core/event.hpp"
#include "internal/core/engine_config.hpp"

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_event.hpp"

#include "argus/wm/window.hpp"
#include "internal/wm/pimpl/window.hpp"

#include "argus/render/common/canvas.hpp"
#include "argus/render/common/scene.hpp"
#include "argus/render/common/transform.hpp"
#include "internal/render/defines.hpp"
#include "internal/render/pimpl/common/canvas.hpp"
#include "internal/render/pimpl/common/transform_2d.hpp"
#include "internal/render/util/object_processor.hpp"

#include "internal/render_opengles/defines.hpp"
#include "internal/render_opengles/gl_util.hpp"
#include "internal/render_opengles/glfw_include.hpp"
#include "internal/render_opengles/types.hpp"
#include "internal/render_opengles/renderer/bucket_proc.hpp"
#include "internal/render_opengles/renderer/compositing.hpp"
#include "internal/render_opengles/renderer/gles_renderer.hpp"
#include "internal/render_opengles/renderer/shader_mgmt.hpp"
#include "internal/render_opengles/renderer/texture_mgmt.hpp"
#include "internal/render_opengles/renderer/2d/scene_compiler.hpp"
#include "internal/render_opengles/state/render_bucket.hpp"
#include "internal/render_opengles/state/renderer_state.hpp"
#include "internal/render_opengles/state/scene_state.hpp"

#include "aglet/aglet.h"

#include <atomic>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace argus {
    // forward declarations
    class Scene2D;

    static Matrix4 _compute_view_matrix(unsigned int res_hor, unsigned int res_ver) {
        // screen space is [0, 1] on both axes with the origin in the top-left
        auto l = 0;
        auto r = 1;
        auto b = 1;
        auto t = 0;

        float hor_scale = 1;
        float ver_scale = 1;

        auto res_hor_f = static_cast<float>(res_hor);
        auto res_ver_f = static_cast<float>(res_ver);

        switch (get_engine_config().screen_space_scale_mode) {
            case ScreenSpaceScaleMode::NormalizeMinDimension:
                if (res_hor > res_ver) {
                    hor_scale = res_hor_f / res_ver_f;
                } else {
                    ver_scale = res_ver_f / res_hor_f;
                }
                break;
            case ScreenSpaceScaleMode::NormalizeMaxDimension:
                if (res_hor > res_ver) {
                    ver_scale = res_ver_f / res_hor_f;
                } else {
                    hor_scale = res_hor_f / res_ver_f;
                }
                break;
            case ScreenSpaceScaleMode::NormalizeVertical:
                hor_scale = res_hor_f / res_ver_f;
                break;
            case ScreenSpaceScaleMode::NormalizeHorizontal:
                ver_scale = res_ver_f / res_hor_f;
                break;
            case ScreenSpaceScaleMode::None:
                break;
        }

        return {
            {2 / ((r - l) * hor_scale), 0, 0, -(r + l) / ((r - l) * hor_scale)},
            {0, 2 / ((t - b) * ver_scale), 0, -(t + b) / ((t - b) * ver_scale)},
            {0, 0, 1, 0},
            {0, 0, 0, 1}
        };
    }

    static Matrix4 _compute_view_matrix(const Vector2u &resolution) {
        return _compute_view_matrix(resolution.x, resolution.y);
    }

    static Matrix4 _compute_view_matrix(const Vector2u &&resolution) {
        return _compute_view_matrix(resolution.x, resolution.y);
    }

    static void _update_view_matrix(const Window &window, RendererState &state, const Vector2u &resolution) {
        auto &canvas = window.get_canvas();
        for (auto *scene : canvas.pimpl->scenes) {
            SceneState &scene_state = state.get_scene_state(*scene, true);
            auto &scene_transform = scene->get_transform();

            multiply_matrices(scene_transform.as_matrix(), _compute_view_matrix(resolution),
                    scene_state.view_matrix);
        }
    }

    static void _rebuild_scene(const Window &window, RendererState &state) {
        auto &canvas = window.get_canvas();
        for (auto *scene : canvas.pimpl->scenes) {
            SceneState &scene_state = state.get_scene_state(*scene, true);

            auto &scene_transform = scene->get_transform();
            if (scene_transform.pimpl->dirty) {
                multiply_matrices(scene_transform.as_matrix(), _compute_view_matrix(window.get_resolution()),
                        scene_state.view_matrix);
                scene_transform.pimpl->dirty = false;
            }

            compile_scene_2d(reinterpret_cast<Scene2D&>(*scene), reinterpret_cast<Scene2DState&>(scene_state));

            fill_buckets(reinterpret_cast<Scene2DState&>(scene_state));

            for (auto bucket_it : scene_state.render_buckets) {
                auto &mat = bucket_it.second->material_res;

                build_shaders(state, mat);

                prepare_texture(state, mat);
            }
        }
    }

    static void _deinit_material(RendererState &state, const std::string &material) {
        _ARGUS_DEBUG("De-initializing material %s", material.c_str());
        for (auto *scene_state : state.all_scene_states) {
            auto &buckets = scene_state->render_buckets;
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

    static void _handle_resource_event(const ResourceEvent &event, void *renderer_state) {
        if (event.subtype != ResourceEventType::Unload) {
            return;
        }

        auto &state = *static_cast<RendererState*>(renderer_state);

        std::string mt = event.prototype.media_type;
        if (mt == RESOURCE_TYPE_TEXTURE_PNG) {
            remove_texture(state, event.prototype.uid);
        } else if (mt == RESOURCE_TYPE_SHADER_GLSL_VERT || mt == RESOURCE_TYPE_SHADER_GLSL_FRAG) {
            remove_shader(state, event.prototype.uid);
        } else if (mt == RESOURCE_TYPE_MATERIAL) {
            _deinit_material(state, event.prototype.uid);
        }
    }

    GLESRenderer::GLESRenderer(const Window &window):
            window(window),
            state(*this) {
        activate_gl_context(window.pimpl->handle);

        int rc = 0;
        if ((rc = agletLoad(reinterpret_cast<AgletLoadProc>(glfwGetProcAddress))) != 0) {
            _ARGUS_FATAL("Failed to load OpenGL ES bindings (Aglet returned code %d)", rc);
        }

        _ARGUS_DEBUG("Successfully loaded OpenGL ES bindings");

        int gl_major;
        int gl_minor;
        const unsigned char *gl_version_str = glGetString(GL_VERSION);
        glGetIntegerv(GL_MAJOR_VERSION, &gl_major);
        glGetIntegerv(GL_MINOR_VERSION, &gl_minor);
        if (!AGLET_GL_ES_VERSION_3_0) {
            _ARGUS_FATAL("Argus requires support for OpenGL ES 3.0 or higher (got %d.%d)", gl_major, gl_minor);
        }

        _ARGUS_INFO("Obtained OpenGL ES %d.%d context (%s)", gl_major, gl_minor, gl_version_str);

        resource_event_handler = register_event_handler<ResourceEvent>(_handle_resource_event,
                TargetThread::Render, &state);

        if (AGLET_GL_KHR_debug) {
            glDebugMessageCallback(gl_debug_callback, nullptr);
        }

        setup_framebuffer(state);
    }

    GLESRenderer::~GLESRenderer(void) {
        unregister_event_handler(resource_event_handler);
    }

    void GLESRenderer::render(const TimeDelta delta) {
        UNUSED(delta);

        activate_gl_context(window.pimpl->handle);

        auto vsync = window.pimpl->properties.vsync.read();
        if (vsync.dirty) {
            glfwSwapInterval(vsync ? 1 : 0);
        }

        _rebuild_scene(window, state);

        // set up state for drawing scene to framebuffers
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDisable(GL_CULL_FACE);

        auto &canvas = window.get_canvas();

        auto resolution = window.pimpl->cur_resolution.read();

        for (auto *scene : canvas.pimpl->scenes) {
            auto &scene_state = state.get_scene_state(*scene);
            draw_scene_to_framebuffer(scene_state, resolution);
        }

        // set up state for drawing framebuffers to screen

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        for (auto *scene : canvas.pimpl->scenes) {
            auto &scene_state = state.get_scene_state(*scene);

            draw_framebuffer_to_screen(scene_state, resolution);
        }

        glfwSwapBuffers(canvas.pimpl->window.pimpl->handle);
    }

    void GLESRenderer::notify_window_resize(const Vector2u &resolution) {
        _update_view_matrix(this->window, this->state, resolution);
    }
}
