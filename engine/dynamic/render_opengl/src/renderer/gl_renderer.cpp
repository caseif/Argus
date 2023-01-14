/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include "internal/render_opengl/renderer/gl_renderer.hpp"

#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/time.hpp"

#include "argus/core/engine_config.hpp"
#include "argus/core/event.hpp"
#include "argus/core/screen_space.hpp"

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_event.hpp"

#include "argus/render/common/canvas.hpp"
#include "argus/render/common/scene.hpp"
#include "argus/render/common/transform.hpp"
#include "argus/render/defines.hpp"

#include "internal/render_opengl/gl_util.hpp"
#include "internal/render_opengl/renderer/2d/scene_compiler.hpp"
#include "internal/render_opengl/renderer/bucket_proc.hpp"
#include "internal/render_opengl/renderer/compositing.hpp"
#include "internal/render_opengl/renderer/shader_mgmt.hpp"
#include "internal/render_opengl/renderer/texture_mgmt.hpp"
#include "internal/render_opengl/state/render_bucket.hpp"
#include "internal/render_opengl/state/renderer_state.hpp"
#include "internal/render_opengl/state/scene_state.hpp"
#include "internal/render_opengl/state/viewport_state.hpp"

#include "GLFW/glfw3.h"
#include "aglet/aglet.h"
#include "argus/wm/window.hpp"

#include <atomic>
#include <map>
#include <set>
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

        switch (get_screen_space_scale_mode()) {
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

    [[maybe_unused]] static Matrix4 _compute_view_matrix(const Vector2u &&resolution) {
        return _compute_view_matrix(resolution.x, resolution.y);
    }

    static void _recompute_2d_viewport_view_matrix(const Viewport &viewport, const Transform2D &transform,
           const Vector2u &resolution, Matrix4 &dest) {
        UNUSED(transform);

        auto center_x = (viewport.left + viewport.right) / 2.0f;
        auto center_y = (viewport.top + viewport.bottom) / 2.0f;

        auto cur_translation = transform.get_translation();

        Matrix4 anchor_mat_1 = {
            {1, 0, 0, -center_x + cur_translation.x},
            {0, 1, 0, -center_y + cur_translation.y},
            {0, 0, 1, 0},
            {0, 0, 0, 1},
        };
        Matrix4 anchor_mat_2 = {
            {1, 0, 0, center_x - cur_translation.x},
            {0, 1, 0, center_y - cur_translation.y},
            {0, 0, 1, 0},
            {0, 0, 0, 1},
        };
        dest = Matrix4::identity();
        multiply_matrices(dest, anchor_mat_1);
        multiply_matrices(dest, transform.get_scale_matrix());
        multiply_matrices(dest, transform.get_rotation_matrix());
        multiply_matrices(dest, anchor_mat_2);
        multiply_matrices(dest, transform.get_translation_matrix());
        multiply_matrices(dest, _compute_view_matrix(resolution));
    }

    static std::set<Scene*> _get_associated_scenes_for_canvas(Canvas &canvas) {
        std::set<Scene*> scenes;
        for (auto viewport : canvas.get_viewports_2d()) {
            scenes.insert(&viewport.get().get_camera().get_scene());
        }
        return scenes;
    }

    static void _update_view_matrix(const Window &window, RendererState &state, const Vector2u &resolution) {
        auto &canvas = window.get_canvas();

        for (auto viewport : canvas.get_viewports_2d()) {
            auto &viewport_state
                = reinterpret_cast<Viewport2DState&>(state.get_viewport_state(viewport, true));
            auto camera_transform = viewport.get().get_camera().peek_transform();
            _recompute_2d_viewport_view_matrix(viewport_state.viewport->get_viewport(), camera_transform.inverse(), resolution,
                   viewport_state.view_matrix);
        }
    }

    static void _rebuild_scene(const Window &window, RendererState &state) {
        auto &canvas = window.get_canvas();

        for (auto viewport : canvas.get_viewports_2d()) {
            Viewport2DState &viewport_state
                = reinterpret_cast<Viewport2DState&>(state.get_viewport_state(viewport, true));
            auto camera_transform = viewport.get().get_camera().get_transform();

            if (camera_transform.dirty) {
                _recompute_2d_viewport_view_matrix(viewport_state.viewport->get_viewport(), camera_transform->inverse(),
                       window.peek_resolution(), viewport_state.view_matrix);
            }
        }

        for (auto *scene : _get_associated_scenes_for_canvas(canvas)) {
            SceneState &scene_state = state.get_scene_state(*scene, true);

            compile_scene_2d(reinterpret_cast<Scene2D&>(*scene), reinterpret_cast<Scene2DState&>(scene_state));

            fill_buckets(scene_state);

            for (auto bucket_it : scene_state.render_buckets) {
                auto &mat = bucket_it.second->material_res;

                build_shaders(state, mat);

                get_or_load_texture(state, mat);
            }
        }
    }

    static void _deinit_material(RendererState &state, const std::string &material) {
        Logger::default_logger().debug("De-initializing material %s", material.c_str());
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
            programs.erase(program_it);
        }

        if (auto texture_uid = state.material_textures.find(material);
            texture_uid != state.material_textures.end()) {
            release_texture(state, texture_uid->second);
        }
    }

    static void _handle_resource_event(const ResourceEvent &event, void *renderer_state) {
        if (event.subtype != ResourceEventType::Unload) {
            return;
        }

        auto &state = *static_cast<RendererState*>(renderer_state);

        std::string mt = event.prototype.media_type;
        if (mt == RESOURCE_TYPE_SHADER_GLSL_VERT || mt == RESOURCE_TYPE_SHADER_GLSL_FRAG) {
            remove_shader(state, event.prototype.uid);
        } else if (mt == RESOURCE_TYPE_MATERIAL) {
            _deinit_material(state, event.prototype.uid);
        }
    }

    GLRenderer::GLRenderer(Window &window):
            window(window),
            state(*this) {
        activate_gl_context(get_window_handle<GLFWwindow>(window));

        int rc = 0;
        if ((rc = agletLoad(reinterpret_cast<AgletLoadProc>(glfwGetProcAddress))) != 0) {
            Logger::default_logger().fatal("Failed to load OpenGL bindings (Aglet returned code %d)", rc);
        }

        int gl_major;
        int gl_minor;
        const unsigned char *gl_version_str = glGetString(GL_VERSION);
        glGetIntegerv(GL_MAJOR_VERSION, &gl_major);
        glGetIntegerv(GL_MINOR_VERSION, &gl_minor);
        if (!AGLET_GL_VERSION_3_3) {
            Logger::default_logger().fatal("Argus requires support for OpenGL 3.3 or higher (got %d.%d)", gl_major, gl_minor);
        }

        Logger::default_logger().info("Obtained OpenGL %d.%d context (%s)", gl_major, gl_minor, gl_version_str);

        resource_event_handler = register_event_handler<ResourceEvent>(_handle_resource_event,
                TargetThread::Render, &state);

        if (AGLET_GL_KHR_debug) {
            glDebugMessageCallback(gl_debug_callback, nullptr);
        }

        setup_framebuffer(state);
    }

    GLRenderer::~GLRenderer(void) {
        unregister_event_handler(resource_event_handler);
    }

    void GLRenderer::render(const TimeDelta delta) {
        UNUSED(delta);

        activate_gl_context(get_window_handle<GLFWwindow>(window));

        auto vsync = window.is_vsync_enabled();
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

        auto resolution = window.get_resolution();

        auto viewports = canvas.get_viewports_2d();

        for (auto &viewport : viewports) {
            auto &viewport_state = state.get_viewport_state(viewport);
            auto &scene = viewport.get().get_camera().get_scene();
            auto &scene_state = state.get_scene_state(scene);
            draw_scene_to_framebuffer(scene_state, viewport_state, resolution);
        }

        // set up state for drawing framebuffers to screen

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        for (auto &viewport : viewports) {
            auto &viewport_state = state.get_viewport_state(viewport);
            auto &scene = viewport.get().get_camera().get_scene();
            auto &scene_state = state.get_scene_state(scene);

            draw_framebuffer_to_screen(scene_state, viewport_state, resolution);
        }

        glfwSwapBuffers(get_window_handle<GLFWwindow>(canvas.get_window()));
    }

    void GLRenderer::notify_window_resize(const Vector2u &resolution) {
        _update_view_matrix(this->window, this->state, resolution);
    }
}
