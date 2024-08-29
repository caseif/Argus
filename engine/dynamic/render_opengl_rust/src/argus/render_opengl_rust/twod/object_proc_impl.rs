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
use crate::argus::render_opengl_rust::shaders::get_material_program;
use crate::argus::render_opengl_rust::state::{ProcessedObject, RendererState};
use crate::argus::render_opengl_rust::util::defines::*;
use lowlevel_rustabi::argus::lowlevel::Vector4f;
use render_rustabi::argus::render::*;
use render_rustabi::render_cabi::argus_processed_render_object_2d_t;
use resman_rustabi::argus::resman::ResourceManager;
use std::ffi::c_void;
use std::{ptr, slice};

fn count_vertices(obj: &RenderObject2d) -> usize {
    obj.get_primitives()
        .into_iter()
        .map(|p| p.vertices.len())
        .sum()
}

pub(crate) fn create_processed_object_2d(
    object: &mut RenderObject2d,
    transform: &Matrix4x4,
    state_ptr: *mut c_void,
) -> argus_processed_render_object_2d_t {
    let state: &mut RendererState = unsafe { &mut *state_ptr.cast() };

    let vertex_count = count_vertices(object);

    let mat_res = ResourceManager::get_instance()
        .get_resource(object.get_material())
        .expect("Resource for RenderObject2D material must be available");

    let program = get_material_program(&mut state.linked_programs, &mat_res);

    let attr_position_loc = program.reflection.inputs.get(SHADER_ATTRIB_POSITION);
    let attr_normal_loc = program.reflection.inputs.get(SHADER_ATTRIB_NORMAL);
    let attr_color_loc = program.reflection.inputs.get(SHADER_ATTRIB_COLOR);
    let attr_texcoord_loc = program.reflection.inputs.get(SHADER_ATTRIB_TEXCOORD);

    let vertex_len = (attr_position_loc
        .map(|_| SHADER_ATTRIB_POSITION_LEN)
        .unwrap_or(0)
        + attr_normal_loc
            .map(|_| SHADER_ATTRIB_NORMAL_LEN)
            .unwrap_or(0)
        + attr_color_loc.map(|_| SHADER_ATTRIB_COLOR_LEN).unwrap_or(0)
        + attr_texcoord_loc
            .map(|_| SHADER_ATTRIB_TEXCOORD_LEN)
            .unwrap_or(0)) as GLuint;

    let buffer_word_count = vertex_count * vertex_len as usize;
    let buffer_size = buffer_word_count * size_of::<GLfloat>();

    let mut vertex_buffer = 0;
    let is_buffer_persistent =
        aglet_have_gl_arb_direct_state_access() && aglet_have_gl_arb_buffer_storage();

    let mapped_buffer_ptr: *mut GLfloat = if aglet_have_gl_arb_direct_state_access() {
        glCreateBuffers(1, &mut vertex_buffer);
        if aglet_have_gl_arb_buffer_storage() {
            glNamedBufferStorage(
                vertex_buffer,
                buffer_size as GLsizeiptr,
                ptr::null(),
                GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT,
            );
            glMapNamedBufferRange(
                vertex_buffer,
                0,
                buffer_size as GLsizeiptr,
                GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT,
            )
        } else {
            glNamedBufferData(
                vertex_buffer,
                buffer_size as GLsizeiptr,
                ptr::null(),
                GL_DYNAMIC_DRAW,
            );
            glMapNamedBuffer(vertex_buffer, GL_WRITE_ONLY)
        }
    } else {
        glGenBuffers(1, &mut vertex_buffer);
        glBindBuffer(GL_COPY_READ_BUFFER, vertex_buffer);
        glBufferData(
            GL_COPY_READ_BUFFER,
            buffer_size as GLsizeiptr,
            ptr::null(),
            GL_DYNAMIC_DRAW,
        );
        glMapBuffer(GL_COPY_READ_BUFFER, GL_WRITE_ONLY)
    }
    .cast();

    let mapped_buffer = unsafe { slice::from_raw_parts_mut(mapped_buffer_ptr, buffer_word_count) };

    let mut cur_vertex_index: usize = 0;
    for prim in object.get_primitives() {
        for vertex in prim.vertices {
            let major_off: usize = cur_vertex_index * vertex_len as usize;
            let mut minor_off: usize = 0;

            if let Some(_) = attr_position_loc {
                let pos_vec = Vector4f {
                    x: vertex.position.x,
                    y: vertex.position.y,
                    z: 0.0,
                    w: 1.0,
                };
                let transformed_pos = transform.multiply_vector(pos_vec);
                mapped_buffer[major_off + minor_off + 0] = transformed_pos.x;
                mapped_buffer[major_off + minor_off + 1] = transformed_pos.y;
                minor_off += 2;
            }
            if let Some(_) = attr_normal_loc {
                mapped_buffer[major_off + minor_off + 0] = vertex.normal.x;
                mapped_buffer[major_off + minor_off + 1] = vertex.normal.y;
                minor_off += 2;
            }
            if let Some(_) = attr_color_loc {
                mapped_buffer[major_off + minor_off + 0] = vertex.color.x;
                mapped_buffer[major_off + minor_off + 1] = vertex.color.y;
                mapped_buffer[major_off + minor_off + 2] = vertex.color.z;
                mapped_buffer[major_off + minor_off + 3] = vertex.color.w;
                minor_off += 4;
            }
            if let Some(_) = attr_texcoord_loc {
                mapped_buffer[major_off + minor_off + 0] = vertex.tex_coord.x;
                mapped_buffer[major_off + minor_off + 1] = vertex.tex_coord.y;
                minor_off += 2;
            }

            cur_vertex_index += 1;
        }
    }

    if !aglet_have_gl_arb_direct_state_access() {
        glUnmapBuffer(GL_COPY_READ_BUFFER);
        glBindBuffer(GL_COPY_READ_BUFFER, 0);
    }

    let mut processed_obj = ProcessedObject::new(
        object.get_handle(),
        mat_res,
        object.get_atlas_stride(),
        object.get_z_index(),
        object.get_light_opacity(),
        vertex_buffer,
        buffer_size,
        count_vertices(object),
        is_buffer_persistent.then_some(mapped_buffer_ptr.cast()),
    );

    processed_obj.anim_frame = object.get_active_frame().value;

    processed_obj.visited = true;
    processed_obj.newly_created = true;

    Box::into_raw(Box::new(processed_obj)).cast()
}

pub(crate) fn update_processed_object_2d(
    object: &mut RenderObject2d,
    proc_obj_ptr: argus_processed_render_object_2d_t,
    transform: &Matrix4x4,
    is_transform_dirty: bool,
    state_ptr: *mut c_void,
) {
    let state: &mut RendererState = unsafe { &mut *state_ptr.cast() };

    // program should be linked by now
    let program = &state.linked_programs[object.get_material()];

    let proc_obj: &mut ProcessedObject = unsafe { &mut *proc_obj_ptr.cast() };

    // if a parent group or the object itself has had its transform updated
    proc_obj.updated = is_transform_dirty;

    let cur_frame = object.get_active_frame();
    if cur_frame.dirty {
        proc_obj.anim_frame = cur_frame.value;
        proc_obj.anim_frame_updated = true;
    }

    if !is_transform_dirty {
        // nothing to do
        proc_obj.visited = true;
        return;
    }

    let attr_position_loc = program.reflection.inputs.get(SHADER_ATTRIB_POSITION);
    let attr_normal_loc = program.reflection.inputs.get(SHADER_ATTRIB_NORMAL);
    let attr_color_loc = program.reflection.inputs.get(SHADER_ATTRIB_COLOR);
    let attr_texcoord_loc = program.reflection.inputs.get(SHADER_ATTRIB_TEXCOORD);

    let vertex_len = (attr_position_loc
        .map(|_| SHADER_ATTRIB_POSITION_LEN)
        .unwrap_or(0)
        + attr_normal_loc
            .map(|_| SHADER_ATTRIB_NORMAL_LEN)
            .unwrap_or(0)
        + attr_color_loc.map(|_| SHADER_ATTRIB_COLOR_LEN).unwrap_or(0)
        + attr_texcoord_loc
            .map(|_| SHADER_ATTRIB_TEXCOORD_LEN)
            .unwrap_or(0)) as GLuint;

    let vertex_count = count_vertices(object);
    let buffer_word_count = vertex_count * vertex_len as usize;

    let mapped_buffer_ptr: *mut GLfloat = proc_obj
        .mapped_buffer
        .unwrap_or_else(|| {
            if aglet_have_gl_arb_direct_state_access() {
                glMapNamedBuffer(proc_obj.staging_buffer, GL_WRITE_ONLY)
            } else {
                glBindBuffer(GL_COPY_READ_BUFFER, proc_obj.staging_buffer);
                glMapBuffer(GL_COPY_READ_BUFFER, GL_WRITE_ONLY)
            }
        })
        .cast();

    let mapped_buffer = unsafe { slice::from_raw_parts_mut(mapped_buffer_ptr, buffer_word_count) };

    let mut cur_vertex_index: usize = 0;
    for prim in object.get_primitives() {
        for vertex in prim.vertices {
            let major_off: usize = cur_vertex_index * vertex_len as usize;
            let mut minor_off: usize = 0;

            let pos_vec = Vector4f {
                x: vertex.position.x,
                y: vertex.position.y,
                z: 0.0,
                w: 1.0,
            };
            let transformed_pos = transform.multiply_vector(pos_vec);
            mapped_buffer[major_off + minor_off + 0] = transformed_pos.x;
            mapped_buffer[major_off + minor_off + 1] = transformed_pos.y;
            minor_off += 2;

            cur_vertex_index += 1;
        }
    }

    if proc_obj.mapped_buffer.is_none() {
        if aglet_have_gl_arb_direct_state_access() {
            glUnmapNamedBuffer(proc_obj.staging_buffer);
        } else {
            glUnmapBuffer(GL_COPY_READ_BUFFER);
            glBindBuffer(GL_COPY_READ_BUFFER, 0);
        }
    }

    proc_obj.visited = true;
    proc_obj.updated = true;
}

pub(crate) fn deinit_object_2d(obj: &mut ProcessedObject) {
    if obj.mapped_buffer.is_some() {
        if aglet_have_gl_arb_direct_state_access() {
            glUnmapNamedBuffer(obj.staging_buffer);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, obj.staging_buffer);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }
    glDeleteBuffers(1, &obj.staging_buffer);
}
