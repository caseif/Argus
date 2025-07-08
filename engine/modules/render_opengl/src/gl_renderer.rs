/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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
use crate::aglet::*;
use crate::bucket_proc::fill_buckets_2d;
use crate::compositing::*;
use crate::shaders::get_material_program;
use crate::state::*;
use crate::textures::get_or_load_texture;
use crate::twod::compile_scene_2d;
use crate::util::buffer::GlBuffer;
use crate::util::gl_util::gl_debug_callback;
use crate::LOGGER;
use argus_core::ScreenSpaceScaleMode;
use argus_logging::info;
use argus_render::common::{AttachedViewport, Matrix4x4, RenderCanvas, Transform2d, Viewport};
use argus_render::constants::*;
use argus_render::twod::get_render_context_2d;
use argus_resman::Resource;
use argus_util::math::{Vector2f, Vector2u};
use argus_wm::*;
use std::ffi::CStr;
use std::ptr;
use std::time::{Duration, SystemTime, UNIX_EPOCH};

pub(crate) struct GlRenderer {
    state: RendererState,
}

impl GlRenderer {
    pub(crate) fn new(window: &mut Window) -> Self {
        let mut renderer = Self {
            state: Default::default(),
        };
        renderer.init(window);
        renderer
    }

    fn init(&mut self, window: &mut Window) {
        #[cfg(debug_assertions)]
        let context_flags = GlContextFlags::ProfileCore;
        #[cfg(not(debug_assertions))]
        let context_flags = GlContextFlags::ProfileCore | GlContextFlags::DebugContext;

        let gl_mgr = WindowManager::instance().get_gl_manager()
            .expect("Failed to get OpenGL manager");

        self.state.gl_context =
            Some(gl_mgr.create_context(window, 3, 3, context_flags).unwrap());
        self.state.gl_context.as_ref().unwrap().make_current(window)
            .expect("Failed to make GL context current");

        if let Err(e) = agletLoad(GlManager::load_proc_ffi) {
            panic!(
                "Failed to load OpenGL bindings (Aglet returned error {:?})",
                e
            );
        }

        let mut gl_major: i32 = 0;
        let mut gl_minor: i32 = 0;
        let gl_version_str = unsafe {
            CStr::from_ptr(glGetString(GL_VERSION).cast())
                .to_str()
                .unwrap()
                .to_string()
        };
        glGetIntegerv(GL_MAJOR_VERSION, &mut gl_major);
        glGetIntegerv(GL_MINOR_VERSION, &mut gl_minor);
        if !aglet_have_gl_version_3_3() {
            panic!(
                "Argus requires support for OpenGL 3.3 or higher (got {}.{})",
                gl_major, gl_minor
            );
        }

        info!(
            LOGGER,
            "Obtained OpenGL {}.{} context ({})",
            gl_major, gl_minor, gl_version_str
        );

        gl_mgr.set_swap_interval(0);

        if aglet_have_gl_khr_debug() {
            glDebugMessageCallback(gl_debug_callback, ptr::null());
        }

        self.create_global_ubo();

        setup_framebuffer(&mut self.state);
    }

    pub(crate) fn render(&mut self, window: &mut Window, _delta: Duration) {
        self.state.gl_context.as_ref().unwrap().make_current(window)
            .expect("Failed to make GL context current");

        let resolution = window.get_resolution();

        if resolution.value.x == 0 {
            return;
        }

        if !self.state.are_viewports_initialized {
            self.update_view_matrix(window, &resolution.value);
            self.state.are_viewports_initialized = true;
        }

        let vsync = window.is_vsync_enabled();
        if vsync.dirty {
            WindowManager::instance().get_gl_manager().unwrap()
                .set_swap_interval(if vsync.value { 1 } else { 0 });
        }

        self.update_global_ubo();

        self.rebuild_scene(window);

        // set up state for drawing scene to framebuffers
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // set blend equation for light opacity map
        if aglet_have_gl_version_4_0() {
            glBlendEquationi(1, GL_MAX);
        } else if aglet_have_gl_arb_draw_buffers_blend() {
            glBlendEquationiARB(1, GL_MAX);
        }

        glDisable(GL_CULL_FACE);

        let canvas = window.get_canvas_mut()
            .expect("Window does not have associated canvas")
            .as_any_mut()
            .downcast_mut::<RenderCanvas>()
            .expect("Canvas object from window was unexpected type!");

        let mut viewports = canvas.get_viewports_2d();
        viewports.sort_by_key(|vp| vp.get_z_index());
        let viewports = viewports;

        // initial render pass
        for viewport in &viewports {
            let scene_id = viewport.get_scene_id();
            draw_scene_2d_to_framebuffer(&mut self.state, scene_id, viewport, &resolution);
        }

        // Lighting pass
        //
        // The actual lightmaps are generated while drawing the scene but the
        // compositing onto the draw framebuffer is deferred until now to allow
        // the lightmap to be applied to viewports "below" the current one.
        //
        // The basic algorithm (slightly rewritten for readability) is:
        //   For each viewport A:
        //     Apply lightmap A to viewport A
        //     For each viewport B in front of A (back to front)
        //       If viewport B has lighting enabled
        //         Apply lightmap B to viewport A
        //
        // In effect, the lightmap of a given viewport A will be applied to all
        // viewports behind A.
        for target_idx in 0..viewports.len() {
            let target_viewport = viewports[target_idx];

            for src_idx in target_idx..viewports.len() {
                let source_viewport = viewports[src_idx];
                let source_scene_id = source_viewport.get_scene_id();
                let source_scene = get_render_context_2d().get_scene(source_scene_id).unwrap();
                if !source_scene.is_lighting_enabled() {
                    continue;
                }
                draw_lightmap_to_framebuffer(
                    &self.state,
                    source_viewport.get_id(),
                    target_viewport.get_id(),
                    target_viewport.get_viewport(),
                    &resolution.value,
                );
            }
        }

        // set up state for drawing framebuffers to screen

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        for viewport in &viewports {
            let viewport_state = self.state.get_viewport_2d_state(viewport.get_id());

            draw_framebuffer_to_screen(
                viewport_state,
                viewport.get_viewport(),
                self.state.frame_program.as_ref().unwrap(),
                self.state.frame_vao.unwrap(),
                &resolution,
            );
        }

        WindowManager::instance().get_gl_manager().unwrap().swap_buffers(window)
            .expect("Failed to swap buffers");
    }

    pub(crate) fn notify_window_resize(
        &mut self,
        window: &mut Window,
        resolution: &Vector2u
    ) {
        self.update_view_matrix(window, resolution);
    }

    pub(crate) fn update_view_matrix(
        &mut self,
        window: &mut Window,
        resolution: &Vector2u
    ) {
        let canvas = window.get_canvas_mut()
            .expect("Window does not have associated canvas")
            .as_any_mut()
            .downcast_mut::<RenderCanvas>()
            .expect("Canvas object from window was unexpected type!");

        for viewport in &canvas.get_viewports_2d() {
            let camera_transform = {
                let scene = get_render_context_2d().get_scene(viewport.get_scene_id()).unwrap();
                scene.get_camera(viewport.get_camera_id()).unwrap()
                    .peek_transform()
            };
            let viewport_state = self.state.get_or_create_viewport_2d_state(viewport.get_id());
            recompute_2d_viewport_view_matrix(
                viewport.get_viewport(),
                &camera_transform.inverse(),
                resolution,
                &mut viewport_state.view_matrix,
            );
            viewport_state.view_matrix_dirty = true;
        }
    }

    fn rebuild_scene(&mut self, window: &Window) {
        let canvas = window.get_canvas()
            .expect("Window does not have associated canvas")
            .as_any()
            .downcast_ref::<RenderCanvas>()
            .expect("Canvas object from window was unexpected type!");

        for viewport in &canvas.get_viewports_2d() {
            let camera_transform = {
                let mut scene = get_render_context_2d()
                    .get_scene_mut(viewport.get_scene_id())
                    .unwrap();
                scene.get_camera_mut(viewport.get_camera_id()).unwrap().get_transform()
            };
            let viewport_state = self.state.get_or_create_viewport_2d_state(viewport.get_id());

            if camera_transform.dirty {
                recompute_2d_viewport_view_matrix(
                    viewport.get_viewport(),
                    &camera_transform.value.inverse(),
                    &window.peek_resolution(),
                    &mut viewport_state.view_matrix,
                );
            }
        }

        let scene_ids = canvas.get_viewports_2d().iter()
            .map(|vp| vp.get_scene_id())
            .collect::<Vec<_>>();
        for scene_id in scene_ids {
            compile_scene_2d(&mut self.state, scene_id);

            fill_buckets_2d(&mut self.state, scene_id);

            let mats: Vec<Resource> = self
                .state
                .get_scene_2d_state(scene_id)
                .render_buckets
                .values()
                .map(|rb| &rb.material_res)
                .cloned()
                .collect();
            for mat in mats {
                _ = get_material_program(&mut self.state.linked_programs, &mat);

                _ = get_or_load_texture(&mut self.state, &mat);
            }
        }
    }

    fn create_global_ubo(&mut self) {
        self.state.global_ubo = Some(GlBuffer::new(
            GL_UNIFORM_BUFFER,
            SHADER_UBO_GLOBAL_LEN as usize,
            GL_DYNAMIC_DRAW,
            true,
            false,
        ));
    }

    fn update_global_ubo(&mut self) {
        let millis = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .expect("Time went backwards!")
            .as_millis() as u64;

        self.state
            .global_ubo
            .as_mut()
            .expect("Global UBO must be initialized before it can be updated")
            .write_val(&millis, SHADER_UNIFORM_GLOBAL_TIME_OFF as usize);
    }
}

impl Drop for GlRenderer {
    fn drop(&mut self) {
    }
}

fn compute_proj_matrix(res_hor: u32, res_ver: u32) -> Matrix4x4 {
    // screen space is [0, 1] on both axes with the origin in the top-left
    let l = 0;
    let r = 1;
    let b = 1;
    let t = 0;

    let res_hor_f = res_hor as f32;
    let res_ver_f = res_ver as f32;

    //let mode = EngineManager::instance().get_config().get_section::<>().screen_scale_mode;
    //TODO
    let mode = ScreenSpaceScaleMode::NormalizeMinDimension;
    let (hor_scale, ver_scale) = match mode {
        ScreenSpaceScaleMode::NormalizeMinDimension => {
            if res_hor > res_ver {
                (res_hor_f / res_ver_f, 1.0)
            } else {
                (1.0, res_ver_f / res_hor_f)
            }
        }
        ScreenSpaceScaleMode::NormalizeMaxDimension => {
            if res_hor > res_ver {
                (1.0, res_ver_f / res_hor_f)
            } else {
                (res_hor_f / res_ver_f, 1.0)
            }
        }
        ScreenSpaceScaleMode::NormalizeVertical => (res_hor_f / res_ver_f, 1.0),
        ScreenSpaceScaleMode::NormalizeHorizontal => (1.0, res_ver_f / res_hor_f),
        ScreenSpaceScaleMode::None => (1.0, 1.0),
    };

    Matrix4x4::from_row_major([
        2.0 / ((r - l) as f32 * hor_scale),
        0.0,
        0.0,
        -(r + l) as f32 / ((r - l) as f32 * hor_scale),
        0.0,
        2.0 / ((t - b) as f32 * ver_scale),
        0.0,
        -(t + b) as f32 / ((t - b) as f32 * ver_scale),
        0.0,
        0.0,
        1.0,
        0.0,
        0.0,
        0.0,
        0.0,
        1.0,
    ])
}

fn recompute_2d_viewport_view_matrix(
    viewport: &Viewport,
    transform: &Transform2d,
    resolution: &Vector2u,
    dest: &mut Matrix4x4,
) {
    let center_x = (viewport.left + viewport.right) / 2.0;
    let center_y = (viewport.top + viewport.bottom) / 2.0;

    let mut adj_transform = transform.clone();
    adj_transform.translation.x += (viewport.right - viewport.left) / 2.0;
    adj_transform.translation.y += (viewport.bottom - viewport.top) / 2.0;

    let anchor_mat_1 = Matrix4x4::from_row_major([
        1.0,
        0.0,
        0.0,
        -center_x + adj_transform.translation.x,
        0.0,
        1.0,
        0.0,
        -center_y + adj_transform.translation.y,
        0.0,
        0.0,
        1.0,
        0.0,
        0.0,
        0.0,
        0.0,
        1.0,
    ]);
    let anchor_mat_2 = Matrix4x4::from_row_major([
        1.0,
        0.0,
        0.0,
        center_x - adj_transform.translation.x,
        0.0,
        1.0,
        0.0,
        center_y - adj_transform.translation.y,
        0.0,
        0.0,
        1.0,
        0.0,
        0.0,
        0.0,
        0.0,
        1.0,
    ]);

    let view_mat =
        adj_transform.get_translation_matrix() *
        anchor_mat_2 *
            adj_transform.get_rotation_matrix() *
            adj_transform.get_scale_matrix() *
        anchor_mat_1;

    *dest = compute_proj_matrix(resolution.x, resolution.y) * view_mat;
}
