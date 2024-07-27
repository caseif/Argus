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

use render_rustabi::argus::render::ShaderReflectionInfo;
use resman_rustabi::argus::resman::Resource;
use crate::argus::render_opengl_rust::gl_renderer::GlRenderer;
use crate::argus::render_opengl_rust::util::gl_util::{GlProgramHandle, GlShaderHandle};

pub(crate) struct LinkedProgram {
    pub(crate) handle: GlProgramHandle,
    pub(crate) reflection: ShaderReflectionInfo,
    pub(crate) has_custom_frag: bool,
}

impl LinkedProgram {
    pub(crate) fn new(
        handle: GlProgramHandle,
        reflection: ShaderReflectionInfo,
        has_custom_frag: bool
    ) -> Self {
        Self { handle, reflection, has_custom_frag }
    }
}

/*pub(crate) fn link_program(shader_uids: &[&str]) -> LinkedProgram {
    //TODO
}

pub(crate) fn build_shaders(renderer: &mut GlRenderer, material_res: &Resource) -> LinkedProgram {
    //TODO
}

pub(crate) fn deinit_shader(shader: GlShaderHandle) {
    //TODO
}

pub(crate) fn remove_shader(renderer: &mut GlRenderer, shader_uid: &str) {
    //TODO
}

pub(crate) fn deinit_program(program: GlProgramHandle) {
    //TODO
}

pub(crate) fn get_std_program(renderer: &mut GlRenderer) -> LinkedProgram {
    //TODO
}

pub(crate) fn get_shadowmap_program(renderer: &mut GlRenderer) -> LinkedProgram {
    //TODO
}

pub(crate) fn get_lighting_program(renderer: &mut GlRenderer) -> LinkedProgram {
    //TODO
}

pub(crate) fn get_lightmap_composite_program(renderer: &mut GlRenderer) -> LinkedProgram {
    if (!state.lightmap_composite_program.has_value()) {
        state.lightmap_composite_program = link_program(
            { SHADER_LIGHTMAP_COMPOSITE_VERT, SHADER_LIGHTMAP_COMPOSITE_FRAG });
    }
    return renderer.lightmap_composite_program.value();
}

pub(crate) fn get_material_program(renderer: &mut GlRenderer, mat_res: &Resource) -> LinkedProgram {
    //TODO
}*/
