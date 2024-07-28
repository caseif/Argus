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
use wm_rustabi::argus::wm::GlContext;
use render_rustabi::argus::render::{AttachedViewport2d, Scene2d};
use resman_rustabi::argus::resman::Resource;
use crate::argus::render_opengl_rust::shaders::*;
use crate::argus::render_opengl_rust::state::*;
use crate::argus::render_opengl_rust::util::buffer::GlBuffer;
use crate::argus::render_opengl_rust::util::gl_util::*;

#[derive(Default)]
pub(crate) struct RendererState {
    pub gl_context: Option<GlContext>,
    pub intrinsic_resources: Vec<Resource>,
    pub scene_states_2d: HashMap<Scene2d, Scene2dState>,
    pub viewport_states_2d: HashMap<AttachedViewport2d, ViewportState>,
    pub are_viewports_initialized: bool,
    pub prepared_textures: HashMap<String, Rc<GlTextureHandle>>,
    pub material_textures: HashMap<String, (String, Rc<GlTextureHandle>)>,
    pub compiled_shaders: HashMap<String, GlShaderHandle>,
    pub linked_programs: HashMap<String, LinkedProgram>,
    pub std_program: Option<LinkedProgram>,
    pub shadowmap_program: Option<LinkedProgram>,
    pub lighting_program: Option<LinkedProgram>,
    pub lightmap_composite_program: Option<LinkedProgram>,
    pub postfx_programs: HashMap<String, LinkedProgram>,
    pub frame_vbo: Option<GlBufferHandle>,
    pub frame_vao: Option<GlArrayHandle>,
    pub frame_program: Option<LinkedProgram>,
    pub global_ubo: Option<GlBuffer>,
}