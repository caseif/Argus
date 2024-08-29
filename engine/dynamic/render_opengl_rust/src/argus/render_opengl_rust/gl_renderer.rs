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
use crate::argus::render_opengl_rust::bucket_proc::fill_buckets_2d;
use crate::argus::render_opengl_rust::compositing::*;
use crate::argus::render_opengl_rust::shaders::get_material_program;
use crate::argus::render_opengl_rust::state::*;
use crate::argus::render_opengl_rust::textures::get_or_load_texture;
use crate::argus::render_opengl_rust::twod::compile_scene_2d;
use crate::argus::render_opengl_rust::util::buffer::GlBuffer;
use core_rustabi::argus::core::{get_screen_space_scale_mode, ScreenSpaceScaleMode};
use lowlevel_rustabi::argus::lowlevel::Vector2u;
use render_rustabi::argus::render::*;
use resman_rustabi::argus::resman::Resource;
use std::ffi::CStr;
use std::ptr;
use std::time::{Duration, SystemTime, UNIX_EPOCH};
use wm_rustabi::argus::wm::*;
use crate::argus::render_opengl_rust::util::gl_util::gl_debug_callback;

pub(crate) struct GlRenderer {
    window: Window,
    state: RendererState,
}

impl GlRenderer {
    pub(crate) fn new(window: &Window) -> Self {
        let mut renderer = Self {
            window: window.clone(),
            state: Default::default(),
        };
        renderer.init();
        renderer
    }

    fn init(&mut self) {
        #[cfg(debug_assertions)]
        let context_flags = GlContextFlags::ProfileCore;
        #[cfg(not(debug_assertions))]
        let context_flags = GlContextFlags::ProfileCore | GlContextFlags::DebugContext;

        self.state.gl_context =
            Some(gl_create_context(&mut self.window, 3, 3, context_flags).unwrap());
        gl_make_context_current(&mut self.window, self.state.gl_context.as_ref().unwrap())
            .expect("Failed to make GL context current");

        if let Err(e) = agletLoad(gl_load_proc) {
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

        //TODO
        println!(
            "Obtained OpenGL {}.{} context ({})",
            gl_major, gl_minor, gl_version_str
        );

        gl_swap_interval(0);

        if aglet_have_gl_khr_debug() {
            //TODO
            glDebugMessageCallback(gl_debug_callback, ptr::null());
        }

        self.create_global_ubo();

        setup_framebuffer(&mut self.state);
    }

    pub(crate) fn render(&mut self, delta: Duration) {
        gl_make_context_current(&mut self.window, self.state.gl_context.as_ref().unwrap())
            .expect("Failed to make GL context current");

        let resolution = self.window.get_resolution();
        
        if resolution.value.x == 0 {
            return;
        }

        if !self.state.are_viewports_initialized {
            self.update_view_matrix(&resolution.value);
            self.state.are_viewports_initialized = true;
        }

        let vsync = self.window.is_vsync_enabled();
        if vsync.dirty {
            gl_swap_interval(vsync.value.then_some(1).unwrap_or(0));
        }

        self.update_global_ubo();

        self.rebuild_scene();

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
        } else if aglet_have_gl_amd_draw_buffers_blend() {
            glBlendEquationIndexedAMD(1, GL_MAX);
        }

        glDisable(GL_CULL_FACE);

        let canvas = Canvas::of(self.window.get_canvas());

        let viewports = &mut canvas.get_viewports_2d();
        viewports.sort_by_key(|vp| vp.as_generic().get_z_index());

        for viewport in &*viewports {
            let scene = viewport.get_camera().get_scene();
            draw_scene_2d_to_framebuffer(&mut self.state, &scene, viewport, &resolution);
        }

        // set up state for drawing framebuffers to screen

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        for viewport in &*viewports {
            let viewport_state = self.state.get_viewport_2d_state(viewport);

            draw_framebuffer_to_screen(
                viewport_state,
                self.state.frame_program.as_ref().unwrap(),
                self.state.frame_vao.unwrap(),
                &resolution,
            );
        }

        gl_swap_buffers(&mut canvas.get_window());
    }

    pub(crate) fn notify_window_resize(&mut self, resolution: &Vector2u) {
        self.update_view_matrix(resolution);
    }

    pub(crate) fn update_view_matrix(&mut self, resolution: &Vector2u) {
        let canvas = Canvas::of(self.window.get_canvas());

        for viewport in &canvas.get_viewports_2d() {
            let viewport_state = self.state.get_or_create_viewport_2d_state(viewport);
            let camera_transform = viewport.get_camera().peek_transform();
            recompute_2d_viewport_view_matrix(
                &viewport_state.viewport.get_viewport(),
                &camera_transform.inverse(),
                resolution,
                &mut viewport_state.view_matrix,
            );
            viewport_state.view_matrix_dirty = true;
        }
    }

    fn rebuild_scene(&mut self) {
        let canvas = Canvas::of(self.window.get_canvas());

        for viewport in &canvas.get_viewports_2d() {
            let viewport_state = self.state.get_or_create_viewport_2d_state(viewport);
            let camera_transform = viewport.get_camera().get_transform();

            if camera_transform.dirty {
                recompute_2d_viewport_view_matrix(
                    &viewport_state.viewport.get_viewport(),
                    &camera_transform.value.inverse(),
                    &self.window.peek_resolution(),
                    &mut viewport_state.view_matrix,
                );
            }
        }

        for scene in canvas.get_viewports_2d().iter().map(|vp| vp.get_camera().get_scene()) {
            compile_scene_2d(&mut self.state, &scene);

            fill_buckets_2d(&mut self.state, &scene);

            let mats: Vec<Resource> = self
                .state
                .get_scene_2d_state(&scene)
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
        self.state
            .gl_context
            .as_mut()
            .inspect(|ctx| gl_destroy_context(ctx));
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

    let (hor_scale, ver_scale) = match get_screen_space_scale_mode() {
        ScreenSpaceScaleMode::NormalizeMinDim => {
            if res_hor > res_ver {
                (res_hor_f / res_ver_f, 1.0)
            } else {
                (1.0, res_ver_f / res_hor_f)
            }
        }
        ScreenSpaceScaleMode::NormalizeMaxDim => {
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

    let cur_translation = &transform.translation;

    let anchor_mat_1 = Matrix4x4::from_row_major([
        1.0,
        0.0,
        0.0,
        -center_x + cur_translation.x,
        0.0,
        1.0,
        0.0,
        -center_y + cur_translation.y,
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
        center_x - cur_translation.x,
        0.0,
        1.0,
        0.0,
        center_y - cur_translation.y,
        0.0,
        0.0,
        1.0,
        0.0,
        0.0,
        0.0,
        0.0,
        1.0,
    ]);

    let view_mat = transform.get_translation_matrix()
        .multiply_matrix(anchor_mat_2)
        .multiply_matrix(transform.get_rotation_matrix())
        .multiply_matrix(transform.get_scale_matrix())
        .multiply_matrix(anchor_mat_1);

    *dest = compute_proj_matrix(resolution.x, resolution.y).multiply_matrix(view_mat);
}
