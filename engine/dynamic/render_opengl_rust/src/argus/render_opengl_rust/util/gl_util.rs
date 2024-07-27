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

use std::ffi::c_void;

use crate::aglet::*;

// all types here serve purely to provide semantic information to declarations

pub(crate) type GlBufferHandle = GLuint;
pub(crate) type GlArrayHandle = GLuint;
pub(crate) type GlBindingIndex = GLuint;
pub(crate) type GlTextureHandle = GLuint;
pub(crate) type GlShaderHandle = GLuint;
pub(crate) type GlProgramHandle = GLuint;
pub(crate) type GlAttributeLocation = GLint;
pub(crate) type GlUniformLocation = GLint;

#[no_mangle]
unsafe extern "C" fn gl_debug_callback(
    source: GLenum,
    ty: GLenum,
    id: GLuint,
    severity: GLenum,
    length: GLsizei,
    message: *const GLchar,
    user_param: *const c_void,
) {
    //TODO
}

pub(crate) fn set_attrib_pointer(
    array_obj: GlArrayHandle,
    buffer_obj: GlBufferHandle,
    binding_index: GlBindingIndex,
    vertex_len: GLuint,
    attr_len: GLuint,
    attr_index: GLuint,
    attr_offset: *const GLuint,
) {
    //TODO
}

pub(crate) fn try_delete_buffer(buffer: GlBufferHandle) {
    //TODO
}

pub(crate) fn bind_texture(unit: GLuint, texture: GlTextureHandle) {
    //TODO
}

//TODO: get_gl_logger

pub(crate) fn restore_gl_blend_params() {
    //TODO
}
