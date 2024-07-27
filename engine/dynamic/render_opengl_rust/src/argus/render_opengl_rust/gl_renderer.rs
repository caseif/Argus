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
use std::collections::HashMap;
use std::rc::Rc;
use std::time::Duration;

use crate::argus::render_opengl_rust::shaders::LinkedProgram;
use crate::argus::render_opengl_rust::state::Scene2dState;
use crate::argus::render_opengl_rust::util::gl_util::*;
use lowlevel_rustabi::argus::lowlevel::Vector2u;
use render_rustabi::argus::render::Scene2d;
use resman_rustabi::argus::resman::Resource;
use wm_rustabi::argus::wm::{GlContext, Window};
use crate::argus::render_opengl_rust::util::buffer::GlBuffer;

pub(crate) struct GlRenderer {
    window: Window,

    gl_context: Option<GlContext>,
    intrinsic_resources: Vec<Resource>,
    scene_states_2d: HashMap<Scene2d, Scene2dState>,
    //viewport_states_2d: HashMap<*const AttachedViewport2D, ViewportState>,
    are_viewports_initialized: bool,
    prepared_textures: HashMap<String, Rc<GlTextureHandle>>,
    material_textures: HashMap<String, String>,
    compiled_shaders: HashMap<String, GlShaderHandle>,
    linked_programs: HashMap<String, LinkedProgram>,
    std_program: Option<LinkedProgram>,
    shadowmap_program: Option<LinkedProgram>,
    lighting_program: Option<LinkedProgram>,
    lightmap_composite_program: Option<LinkedProgram>,
    postfx_programs: HashMap<String, LinkedProgram>,
    frame_vbo: Option<GlBufferHandle>,
    frame_vao: Option<GlArrayHandle>,
    frame_program: Option<LinkedProgram>,
    global_ubo: Option<GlBuffer>,
}

impl GlRenderer {
    pub(crate) fn new(window: Window) -> Self {
        Self {
            window,
            gl_context: None,
            intrinsic_resources: Vec::new(),
            scene_states_2d: HashMap::new(),
            //viewport_states_2d: HashMap::new(),
            are_viewports_initialized: false,
            prepared_textures: HashMap::new(),
            material_textures: HashMap::new(),
            compiled_shaders: HashMap::new(),
            linked_programs: HashMap::new(),
            std_program: None,
            shadowmap_program: None,
            lighting_program: None,
            lightmap_composite_program: None,
            postfx_programs: HashMap::new(),
            frame_vbo: None,
            frame_vao: None,
            frame_program: None,
            global_ubo: None,
        }
    }

    pub(crate) fn render(&mut self, delta: Duration) {
        //TODO
    }

    pub(crate) fn notify_window_resize(&mut self, new_size: Vector2u) {
        //TODO
    }
}
