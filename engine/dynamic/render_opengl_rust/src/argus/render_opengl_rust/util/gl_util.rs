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

use std::ffi::{c_void, CStr};
use std::ptr;
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
pub(crate) unsafe extern "C" fn gl_debug_callback(
    _source: GLenum,
    _ty: GLenum,
    _id: GLuint,
    severity: GLenum,
    _length: GLsizei,
    message: *const GLchar,
    _user_param: *const c_void,
) {
    #[cfg(debug_assertions)]
    if severity == GL_DEBUG_SEVERITY_NOTIFICATION || severity == GL_DEBUG_SEVERITY_LOW {
        return;
    }

    let (level, is_error) = match severity {
        GL_DEBUG_SEVERITY_HIGH => ("SEVERE", true),
        GL_DEBUG_SEVERITY_MEDIUM => ("WARN", true),
        GL_DEBUG_SEVERITY_LOW => ("INFO", false),
        GL_DEBUG_SEVERITY_NOTIFICATION => ("TRACE", false),
        _ => ("UNKNOWN", true), // shouldn't happen
    };

    if is_error {
        //TODO
        eprintln!("[GL][{}] {}", level, CStr::from_ptr(message).to_string_lossy().to_string());
    } else {
        //TODO
        println!("[GL][{}] {}", level, CStr::from_ptr(message).to_string_lossy().to_string());
    }
}

pub(crate) fn set_attrib_pointer(
    array_obj: GlArrayHandle,
    buffer_obj: GlBufferHandle,
    binding_index: GlBindingIndex,
    vertex_len: GLuint,
    attr_len: GLuint,
    attr_index: GLuint,
    attr_offset: &mut GLuint,
) {
    assert!(attr_len <= i32::MAX as u32);

    if aglet_have_gl_arb_direct_state_access() {
        glEnableVertexArrayAttrib(array_obj, attr_index);
        glVertexArrayAttribFormat(
            array_obj,
            attr_index,
            attr_len as GLint,
            GL_FLOAT,
            GL_FALSE as GLboolean,
            *attr_offset
        );
        glVertexArrayAttribBinding(array_obj, attr_index, binding_index);
    } else {
        let stride = vertex_len * size_of::<GLfloat>() as u32;
        assert!(stride <= i32::MAX as u32);

        glBindBuffer(GL_ARRAY_BUFFER, buffer_obj);
        glEnableVertexAttribArray(attr_index);
        glVertexAttribPointer(
            attr_index,
            attr_len as GLint,
            GL_FLOAT,
            GL_FALSE as GLboolean,
            stride as GLsizei,
            ptr::from_ref(attr_offset).cast()
        );
    }

    *attr_offset += attr_len * size_of::<GLfloat>() as u32;
}

pub(crate) fn try_delete_buffer(buffer: GlBufferHandle) {
    if buffer == 0 {
        return;
    }

    glDeleteBuffers(1, &buffer);
}

pub(crate) fn try_delete_vertex_array(array: GlArrayHandle) {
    if array == 0 {
        return;
    }

    glDeleteVertexArrays(1, &array);
}

pub(crate) fn bind_texture(unit: GLuint, texture: GlTextureHandle) {
    if aglet_have_gl_arb_direct_state_access() {
        glBindTextureUnit(unit, texture);
    } else {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, texture);
    }
}

//TODO: get_gl_logger

pub(crate) fn restore_gl_blend_params() {
    if aglet_have_gl_version_4_0() {
        return;
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
}
