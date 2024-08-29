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
use crate::aglet::*;
use crate::argus::render_opengl_rust::state::{RendererState, Scene2dState};
use crate::argus::render_opengl_rust::util::defines::*;
use crate::argus::render_opengl_rust::util::gl_util::*;
use render_rustabi::argus::render::*;
use std::ptr;
use crate::argus::render_opengl_rust::shaders::LinkedProgram;

const BINDING_INDEX_VBO: u32 = 0;
const BINDING_INDEX_ANIM_FRAME_BUF: u32 = 1;

pub(crate) fn fill_buckets_2d(renderer_state: &mut RendererState, scene: &Scene2d) {
    _ = renderer_state.get_or_create_scene_2d_state(scene);
    let scene_state = renderer_state.scene_states_2d.get_mut(&scene.get_id()).unwrap();
    scene_state.render_buckets.retain(|_, bucket| {
        if bucket.objects.is_empty() {
            bucket
                .vertex_array
                .inspect(|arr| try_delete_vertex_array(*arr));
            bucket.vertex_buffer.inspect(|buf| try_delete_buffer(*buf));
            bucket
                .anim_frame_buffer
                .inspect(|buf| try_delete_buffer(*buf));

            // ubo will be implicitly deleted in GL when bucket is dropped

            return false;
        }

        return true;
    });

    for bucket in scene_state.render_buckets.values_mut() {
        let program = renderer_state.linked_programs
            .get(&bucket.material_res.get_prototype().uid)
            .expect("Bucket program should have been linked during object processing");

        let attr_position_loc = program.reflection.inputs.get(SHADER_ATTRIB_POSITION);
        let attr_normal_loc = program.reflection.inputs.get(SHADER_ATTRIB_NORMAL);
        let attr_color_loc = program.reflection.inputs.get(SHADER_ATTRIB_COLOR);
        let attr_texcoord_loc = program.reflection.inputs.get(SHADER_ATTRIB_TEXCOORD);
        let attr_anim_frame_loc = program.reflection.inputs.get(SHADER_ATTRIB_ANIM_FRAME);

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

        let mut anim_frame_buf_len = 0;
        if bucket.needs_rebuild {
            let mut buffer_len = 0;
            for obj_handle in &bucket.objects {
                let obj = scene_state.processed_objs.get(obj_handle);
                buffer_len += obj.staging_buffer_size;
                anim_frame_buf_len +=
                    obj.vertex_count * SHADER_ATTRIB_ANIM_FRAME_LEN * size_of::<GLfloat>();
            }

            bucket
                .vertex_array
                .inspect(|arr| glDeleteVertexArrays(1, arr));

            bucket.vertex_buffer.inspect(|buf| glDeleteBuffers(1, buf));

            bucket
                .anim_frame_buffer
                .inspect(|buf| glDeleteBuffers(1, buf));

            if aglet_have_gl_arb_direct_state_access() {
                let vert_arr = {
                    let mut handle = 0;
                    glCreateVertexArrays(1, &mut handle);
                    bucket.vertex_array = Some(handle);
                    handle
                };

                let vert_buf = {
                    let mut handle = 0;
                    glCreateBuffers(1, &mut handle);
                    bucket.vertex_buffer = Some(handle);
                    handle
                };
                glNamedBufferData(
                    vert_buf,
                    buffer_len as GLsizeiptr,
                    ptr::null(),
                    GL_DYNAMIC_COPY,
                );

                let stride = (vertex_len as usize * size_of::<GLfloat>()) as GLsizei;

                glVertexArrayVertexBuffer(vert_arr, BINDING_INDEX_VBO, vert_buf, 0, stride);

                let anim_buf = {
                    let mut handle = 0;
                    glCreateBuffers(1, &mut handle);
                    bucket.anim_frame_buffer = Some(handle);
                    handle
                };
                glNamedBufferData(
                    anim_buf,
                    anim_frame_buf_len as GLsizeiptr,
                    ptr::null(),
                    GL_DYNAMIC_DRAW,
                );

                glVertexArrayVertexBuffer(
                    vert_arr,
                    BINDING_INDEX_ANIM_FRAME_BUF,
                    anim_buf,
                    0,
                    (SHADER_ATTRIB_ANIM_FRAME_LEN * size_of::<GLfloat>()) as GLsizei,
                );
            } else {
                let vert_arr = {
                    let mut handle = 0;
                    glGenVertexArrays(1, &mut handle);
                    bucket.vertex_array = Some(handle);
                    handle
                };
                glBindVertexArray(vert_arr);

                let anim_buf = {
                    let mut handle = 0;
                    glGenBuffers(1, &mut handle);
                    bucket.anim_frame_buffer = Some(handle);
                    handle
                };
                glBindBuffer(GL_ARRAY_BUFFER, anim_buf);
                glBufferData(
                    GL_ARRAY_BUFFER,
                    anim_frame_buf_len as GLsizeiptr,
                    ptr::null(),
                    GL_DYNAMIC_DRAW,
                );

                let vert_buf = {
                    let mut handle = 0;
                    glGenBuffers(1, &mut handle);
                    bucket.vertex_buffer = Some(handle);
                    handle
                };
                glBindBuffer(GL_ARRAY_BUFFER, vert_buf);
                glBufferData(
                    GL_ARRAY_BUFFER,
                    buffer_len as GLsizeiptr,
                    ptr::null(),
                    GL_DYNAMIC_COPY,
                );
            }

            if anim_frame_buf_len > 0 {
                bucket
                    .anim_frame_buffer_staging
                    .resize(anim_frame_buf_len, 0);
            } else {
                #[allow(clippy::vec_resize_to_zero)]
                bucket.anim_frame_buffer_staging.resize(0, 0);
            }

            let mut attr_offset = 0;

            if let Some(loc) = attr_position_loc {
                set_attrib_pointer(
                    bucket.vertex_array.unwrap(),
                    bucket.vertex_buffer.unwrap(),
                    BINDING_INDEX_VBO,
                    vertex_len as GLuint,
                    SHADER_ATTRIB_POSITION_LEN as u32,
                    *loc,
                    &mut attr_offset,
                );
            }
            if let Some(loc) = attr_normal_loc {
                set_attrib_pointer(
                    bucket.vertex_array.unwrap(),
                    bucket.vertex_buffer.unwrap(),
                    BINDING_INDEX_VBO,
                    vertex_len,
                    SHADER_ATTRIB_NORMAL_LEN as u32,
                    *loc,
                    &mut attr_offset,
                );
            }
            if let Some(loc) = attr_color_loc {
                set_attrib_pointer(
                    bucket.vertex_array.unwrap(),
                    bucket.vertex_buffer.unwrap(),
                    BINDING_INDEX_VBO,
                    vertex_len,
                    SHADER_ATTRIB_COLOR_LEN as u32,
                    *loc,
                    &mut attr_offset,
                );
            }
            if let Some(loc) = attr_texcoord_loc {
                set_attrib_pointer(
                    bucket.vertex_array.unwrap(),
                    bucket.vertex_buffer.unwrap(),
                    BINDING_INDEX_VBO,
                    vertex_len,
                    SHADER_ATTRIB_TEXCOORD_LEN as u32,
                    *loc,
                    &mut attr_offset,
                );
            }
            if let Some(loc) = attr_anim_frame_loc {
                let mut offset = 0;
                set_attrib_pointer(
                    bucket.vertex_array.unwrap(),
                    bucket.anim_frame_buffer.unwrap(),
                    BINDING_INDEX_ANIM_FRAME_BUF,
                    SHADER_ATTRIB_ANIM_FRAME_LEN as u32,
                    SHADER_ATTRIB_ANIM_FRAME_LEN as u32,
                    *loc,
                    &mut offset,
                );
            }
        } else {
            anim_frame_buf_len =
                bucket.vertex_count * SHADER_ATTRIB_ANIM_FRAME_LEN * size_of::<GLfloat>();
        }

        let anim_frame_buf_len = anim_frame_buf_len; // drop mut

        bucket.vertex_count = 0;

        if !aglet_have_gl_arb_direct_state_access() {
            glBindBuffer(GL_ARRAY_BUFFER, bucket.vertex_buffer.unwrap());
        }

        let mut anim_buf_updated = false;

        let mut offset = 0;
        let mut anim_frame_off = 0;
        for obj_handle in &mut bucket.objects {
            let processed = scene_state.processed_objs.get_mut(obj_handle);

            if bucket.needs_rebuild || processed.updated {
                if aglet_have_gl_arb_direct_state_access() {
                    glCopyNamedBufferSubData(
                        processed.staging_buffer,
                        bucket.vertex_buffer.unwrap(),
                        0,
                        offset as GLintptr,
                        processed.staging_buffer_size as GLsizeiptr,
                    );
                } else {
                    glBindBuffer(GL_COPY_READ_BUFFER, processed.staging_buffer);
                    glCopyBufferSubData(
                        GL_COPY_READ_BUFFER,
                        GL_ARRAY_BUFFER,
                        0,
                        offset as GLintptr,
                        processed.staging_buffer_size as GLsizeiptr,
                    );
                    glBindBuffer(GL_COPY_READ_BUFFER, 0);
                }
            }

            if bucket.needs_rebuild || processed.anim_frame_updated {
                for _ in 0..processed.vertex_count {
                    unsafe {
                        *bucket
                            .anim_frame_buffer_staging
                            .as_mut_ptr()
                            .cast::<GLfloat>()
                            .offset(anim_frame_off) = processed.anim_frame.x as f32;
                        anim_frame_off += 1;
                        *bucket
                            .anim_frame_buffer_staging
                            .as_mut_ptr()
                            .cast::<GLfloat>()
                            .offset(anim_frame_off) = processed.anim_frame.y as f32;
                        anim_frame_off += 1;
                    }
                }
                processed.anim_frame_updated = false;
                anim_buf_updated = true;
            } else {
                anim_frame_off += (processed.vertex_count * SHADER_ATTRIB_ANIM_FRAME_LEN) as isize;
            }

            offset += processed.staging_buffer_size;

            bucket.vertex_count += processed.vertex_count;
        }

        if anim_buf_updated {
            if aglet_have_gl_arb_direct_state_access() {
                glNamedBufferSubData(
                    bucket.anim_frame_buffer.unwrap(),
                    0,
                    anim_frame_buf_len as GLsizeiptr,
                    bucket.anim_frame_buffer_staging.as_ptr().cast(),
                );
            } else {
                glBindBuffer(GL_ARRAY_BUFFER, bucket.anim_frame_buffer.unwrap());
                glBufferSubData(
                    GL_ARRAY_BUFFER,
                    0,
                    anim_frame_buf_len as GLsizeiptr,
                    bucket.anim_frame_buffer_staging.as_ptr().cast(),
                );
            }
        }

        if !aglet_have_gl_arb_direct_state_access() {
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glBindVertexArray(0);
        }

        bucket.needs_rebuild = false;
    }
}
