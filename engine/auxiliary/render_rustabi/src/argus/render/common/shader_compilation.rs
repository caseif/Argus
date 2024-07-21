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
use std::ptr;
use std::ptr::null_mut;

use crate::argus::render::{Shader, ShaderReflectionInfo};

use crate::render_cabi::*;

pub fn compile_glsl_to_spirv(
    glsl_sources: &Vec<Shader>,
    client: glslang_client_t,
    client_version: glslang_target_client_version_t,
    spirv_version: glslang_target_language_version_t,
) -> (Vec<Shader>, ShaderReflectionInfo) {
    unsafe {
        let src_handles: Vec<argus_shader_const_t> =
            glsl_sources.iter().map(|src| src.get_handle()).collect();

        let mut compiled_shaders: Vec<argus_shader_t> = Vec::with_capacity(src_handles.len());
        let mut refl_info: argus_shader_refl_info_t = null_mut();

        argus_compile_glsl_to_spirv(
            src_handles.as_ptr(),
            src_handles.len(),
            client,
            client_version,
            spirv_version,
            compiled_shaders.as_mut_ptr(),
            ptr::addr_of_mut!(refl_info),
        );

        (
            compiled_shaders
                .into_iter()
                .map(|handle| Shader::of(handle))
                .collect(),
            ShaderReflectionInfo::of(refl_info),
        )
    }
}
