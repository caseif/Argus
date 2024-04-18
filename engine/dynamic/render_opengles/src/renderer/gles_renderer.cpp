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

#include "argus/lowlevel/enum_ops.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/time.hpp"

#include "argus/core/engine_config.hpp"
#include "argus/core/event.hpp"
#include "argus/core/screen_space.hpp"

#include "argus/resman/resource_event.hpp"

#include "argus/wm/api_util.hpp"
#include "argus/wm/window.hpp"

#include "argus/render/common/canvas.hpp"
#include "argus/render/common/scene.hpp"
#include "argus/render/common/transform.hpp"
#include "argus/render/defines.hpp"

#include "internal/render_opengles/gl_util.hpp"
#include "internal/render_opengles/renderer/2d/scene_compiler.hpp"
#include "internal/render_opengles/renderer/bucket_proc.hpp"
#include "internal/render_opengles/renderer/compositing.hpp"
#include "internal/render_opengles/renderer/gles_renderer.hpp"
#include "internal/render_opengles/renderer/shader_mgmt.hpp"
#include "internal/render_opengles/renderer/texture_mgmt.hpp"
#include "internal/render_opengles/state/render_bucket.hpp"
#include "internal/render_opengles/state/renderer_state.hpp"
#include "internal/render_opengles/state/scene_state.hpp"
#include "internal/render_opengles/state/viewport_state.hpp"

#include "aglet/aglet.h"

#include <algorithm>
#include <set>
#include <string>

namespace argus {
    // forward declarations
    class Scene2D;

    static Matrix4 _compute_proj_matrix(unsigned int res_hor, unsigned int res_ver) {
        // screen space is [0, 1] on both axes with the origin in the top-left
        auto l = 0;
        auto r = 1;
        auto b = 1;
        auto t = 0;

        float hor_scale = 1;
        float ver_scale = 1;

        auto res_hor_f = float(res_hor);
        auto res_ver_f = float(res_ver);

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

        return Matrix4::from_row_major({
                2 / (float(r - l) * hor_scale), 0,                              0,
                                                                                    -float(r + l) /
                                                                                    (float(r - l) * hor_scale),
                0,                              2 / (float(t - b) * ver_scale), 0, -float(t + b) /
                                                                                    (float(t - b) * ver_scale),
                0,                              0,                              1, 0,
                0,                              0,                              0, 1
        });
    }

    static Matrix4 _compute_proj_matrix(const Vector2u &resolution) {
        return _compute_proj_matrix(resolution.x, resolution.y);
    }

    [[maybe_unused]] static Matrix4 _compute_proj_matrix(const Vector2u &&resolution) {
        return _compute_proj_matrix(resolution.x, resolution.y);
    }

    static void _recompute_2d_viewport_view_matrix(const Viewport &viewport, const Transform2D &transform,
            const Vector2u &resolution, Matrix4 &dest) {
        UNUSED(transform);

        auto center_x = (viewport.left + viewport.right) / 2.0f;
        auto center_y = (viewport.top + viewport.bottom) / 2.0f;

        auto cur_translation = transform.get_translation();

        Matrix4 anchor_mat_1 = Matrix4::from_row_major({
                1, 0, 0, -center_x + cur_translation.x,
                0, 1, 0, -center_y + cur_translation.y,
                0, 0, 1, 0,
                0, 0, 0, 1
        });
        Matrix4 anchor_mat_2 = Matrix4::from_row_major({
                1, 0, 0, center_x - cur_translation.x,
                0, 1, 0, center_y - cur_translation.y,
                0, 0, 1, 0,
                0, 0, 0, 1,
        });
        auto view_mat = transform.get_translation_matrix()
                * anchor_mat_2
                * transform.get_rotation_matrix()
                * transform.get_scale_matrix()
                * anchor_mat_1;

        dest = _compute_proj_matrix(resolution) * view_mat;
    }

    static std::set<Scene *> _get_associated_scenes_for_canvas(Canvas &canvas) {
        std::set<Scene *> scenes;
        for (auto viewport : canvas.get_viewports_2d()) {
            scenes.insert(&viewport.get().get_camera().get_scene());
        }
        return scenes;
    }

    static void _update_view_matrix(const Window &window, RendererState &state, const Vector2u &resolution) {
        auto &canvas = window.get_canvas();

        for (auto viewport : canvas.get_viewports_2d()) {
            auto &viewport_state
                    = reinterpret_cast<Viewport2DState &>(state.get_viewport_state(viewport, true));
            auto camera_transform = viewport.get().get_camera().peek_transform();
            _recompute_2d_viewport_view_matrix(viewport_state.viewport->get_viewport(), camera_transform.inverse(),
                    resolution,
                    viewport_state.view_matrix);
            viewport_state.view_matrix_dirty = true;
        }
    }

    static void _rebuild_scene(const Window &window, RendererState &state) {
        auto &canvas = window.get_canvas();

        for (auto viewport : canvas.get_viewports_2d()) {
            Viewport2DState &viewport_state
                    = reinterpret_cast<Viewport2DState &>(state.get_viewport_state(viewport, true));
            auto camera_transform = viewport.get().get_camera().get_transform();

            if (camera_transform.dirty) {
                _recompute_2d_viewport_view_matrix(viewport_state.viewport->get_viewport(), camera_transform->inverse(),
                        window.peek_resolution(), viewport_state.view_matrix);
            }
        }

        for (auto *scene : _get_associated_scenes_for_canvas(canvas)) {
            SceneState &scene_state = state.get_scene_state(*scene, true);

            compile_scene_2d(reinterpret_cast<Scene2D &>(*scene), reinterpret_cast<Scene2DState &>(scene_state));

            fill_buckets(scene_state);

            for (const auto &[_, bucket] : scene_state.render_buckets) {
                auto &mat = bucket->material_res;

                build_shaders(state, mat);

                get_or_load_texture(state, mat);
            }
        }
    }

    static void _create_global_ubo(RendererState &state) {
        state.global_ubo = BufferInfo::create(GL_UNIFORM_BUFFER, SHADER_UBO_GLOBAL_LEN, GL_DYNAMIC_DRAW, false);
    }

    static void _update_global_ubo(RendererState &state) {
        float time = float(
                double(
                        std::chrono::time_point_cast<std::chrono::microseconds>(
                                now()).time_since_epoch().count()
                ) / 1000.0
        );

        state.global_ubo.write_val(time, SHADER_UNIFORM_GLOBAL_TIME_OFF);
    }

    GLESRenderer::GLESRenderer(Window &window) :
            window(window),
            state(*this) {
        using namespace argus::enum_ops;

        auto context_flags = GLContextFlags::ProfileES;
        #ifdef _ARGUS_DEBUG_MODE
        context_flags |= GLContextFlags::DebugContext;
        #endif

        state.gles_context = gl_create_context(window, 3, 0, context_flags);
        gl_make_context_current(window, state.gles_context);

        int rc;
        if ((rc = agletLoad(reinterpret_cast<AgletLoadProc>(gl_load_proc))) != 0) {
            Logger::default_logger().fatal("Failed to load OpenGL ES bindings (Aglet returned code %d)", rc);
        }

        Logger::default_logger().debug("Successfully loaded OpenGL ES bindings");

        int gl_major;
        int gl_minor;
        const unsigned char *gl_version_str = glGetString(GL_VERSION);
        glGetIntegerv(GL_MAJOR_VERSION, &gl_major);
        glGetIntegerv(GL_MINOR_VERSION, &gl_minor);
        if (!AGLET_GL_ES_VERSION_3_0) {
            Logger::default_logger().fatal("Argus requires support for OpenGL ES 3.0 or higher (got %d.%d)", gl_major,
                    gl_minor);
        }

        Logger::default_logger().info("Obtained OpenGL ES %d.%d context (%s)", gl_major, gl_minor, gl_version_str);

        gl_swap_interval(false);

        if (AGLET_GL_KHR_debug) {
            glDebugMessageCallback(gl_debug_callback, nullptr);
        }

        _create_global_ubo(state);

        setup_framebuffer(state);
    }

    GLESRenderer::~GLESRenderer(void) {
        gl_destroy_context(state.gles_context);
    }

    void GLESRenderer::render(const TimeDelta delta) {
        UNUSED(delta);

        gl_make_context_current(window, state.gles_context);

        if (!state.are_viewports_initialized) {
            _update_view_matrix(window, state, window.get_resolution().value);
            state.are_viewports_initialized = true;
        }

        auto vsync = window.is_vsync_enabled();
        if (vsync.dirty) {
            gl_swap_interval(vsync ? 1 : 0);
        }

        _update_global_ubo(state);

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
        std::sort(viewports.begin(), viewports.end(),
                [](auto a, auto b) { return a.get().get_z_index() < b.get().get_z_index(); });

        for (auto viewport : viewports) {
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

        gl_swap_buffers(window);
    }

    void GLESRenderer::notify_window_resize(const Vector2u &resolution) {
        _update_view_matrix(this->window, this->state, resolution);
    }
}
