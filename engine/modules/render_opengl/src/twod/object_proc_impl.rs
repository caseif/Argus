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
use crate::shaders::{get_material_program, LinkedProgram};
use crate::state::{ProcessedObject, RendererState, Scene2dState};
use crate::util::defines::*;
use argus_render::constants::*;
use argus_render::twod::{get_render_context_2d, RenderObject2d};
use argus_util::math::{Matrix4x4, Vector2f, Vector4f};
use argus_util::pool::Handle;
use argus_util::versioned::Version;
use crate::util::buffer::GlBuffer;
use crate::util::support::{GlExt, GlSupport};

fn count_vertices(obj: &RenderObject2d) -> usize {
    obj.get_primitives()
        .iter()
        .map(|p| p.vertices.len())
        .sum()
}

pub(crate) fn process_object(
    scene_id: &str,
    object_handle: Handle,
    transform: &Matrix4x4,
    is_transform_dirty: bool,
    state: &mut RendererState,
) {
    //TODO: stopgap until render graph buffering is properly implemented
    let Some(mut object) = get_render_context_2d().get_object_mut(object_handle) else { return; };

    let existing_obj = {
        let scene_state = state.scene_states_2d.entry(scene_id.to_string()).or_insert_with(|| {
            Scene2dState {
                scene_id: scene_id.to_string(),
                ambient_light_level_version: Version::MAX,
                ambient_light_color_version: Version::MAX,
                ubo: None,
                render_buckets: Default::default(),
                processed_objs: Default::default(),
            }
        });
        scene_state.processed_objs.get_mut(&object_handle)
    };

    if let Some(proc_obj) = existing_obj {
        // program should be linked by now
        let program = &state.linked_programs[&object.get_material().get_prototype().uid];
        update_processed_object_2d(&mut object, proc_obj, transform, is_transform_dirty, program);
    } else {
        let new_proc_obj = create_processed_object_2d(state, &mut object, transform);
        state.scene_states_2d.get_mut(scene_id).unwrap().processed_objs.insert(
            object_handle,
            new_proc_obj,
        );
    }
}

fn create_processed_object_2d(
    state: &mut RendererState,
    object: &mut RenderObject2d,
    transform: &Matrix4x4,
) -> ProcessedObject {
    let vertex_count = count_vertices(object);

    let mat_res = object.get_material().upgrade()
        .expect("Resource for RenderObject2D material was unloaded!");

    let program = get_material_program(&mut state.linked_programs, &mat_res);

    let attr_position_loc = program.reflection.inputs.get(SHADER_ATTRIB_POSITION);
    let attr_normal_loc = program.reflection.inputs.get(SHADER_ATTRIB_NORMAL);
    let attr_color_loc = program.reflection.inputs.get(SHADER_ATTRIB_COLOR);
    let attr_texcoord_loc = program.reflection.inputs.get(SHADER_ATTRIB_TEXCOORD);

    let words_per_vertex = (attr_position_loc
        .map(|_| SHADER_ATTRIB_POSITION_LEN)
        .unwrap_or(0)
        + attr_normal_loc
            .map(|_| SHADER_ATTRIB_NORMAL_LEN)
            .unwrap_or(0)
        + attr_color_loc.map(|_| SHADER_ATTRIB_COLOR_LEN).unwrap_or(0)
        + attr_texcoord_loc
            .map(|_| SHADER_ATTRIB_TEXCOORD_LEN)
            .unwrap_or(0)) as GLuint;

    let buffer_word_count = vertex_count * words_per_vertex as usize;
    let buffer_size = buffer_word_count * size_of::<GLfloat>();

    let vertex_buffer = GlBuffer::new(GL_COPY_READ_BUFFER, buffer_size, GL_DYNAMIC_DRAW, true);

    {
        let _vert_buf_map_guard = vertex_buffer.map_write();

        let mut cur_vertex_index: usize = 0;
        for prim in object.get_primitives() {
            #[allow(unused_assignments)]
            for vertex in &prim.vertices {
                let mut cursor = cur_vertex_index * words_per_vertex as usize * size_of::<GLfloat>();

                if attr_position_loc.is_some() {
                    let pos_vec = Vector4f {
                        x: vertex.position.x,
                        y: vertex.position.y,
                        z: 0.0,
                        w: 1.0,
                    };
                    let transformed_pos = {
                        let pos = transform * pos_vec;
                        Vector2f::new(pos.x, pos.y)
                    };
                    vertex_buffer.write_val(
                        transformed_pos,
                        cursor,
                    );
                    cursor += size_of_val(&transformed_pos);
                }
                if attr_normal_loc.is_some() {
                    vertex_buffer.write_val(vertex.normal, cursor);
                    cursor += size_of_val(&vertex.normal);
                }
                if attr_color_loc.is_some() {
                    vertex_buffer.write_val(vertex.color, cursor);
                    cursor += size_of_val(&vertex.color);
                }
                if attr_texcoord_loc.is_some() {
                    vertex_buffer.write_val(vertex.tex_coord, cursor);
                    cursor += size_of_val(&vertex.tex_coord);
                }

                cur_vertex_index += 1;
            }
        }
    }

    if !GlSupport::have(GlExt::DirectStateAccess) {
        glBindBuffer(GL_COPY_READ_BUFFER, 0);
    }

    let mut processed_obj = ProcessedObject::new(
        object.get_handle().unwrap(),
        mat_res,
        object.get_atlas_stride(),
        object.get_z_index(),
        **object.get_light_opacity(),
        vertex_buffer,
        buffer_size,
        count_vertices(object),
    );

    processed_obj.anim_frame.copy_if_stale(object.get_active_anim_frame());

    processed_obj.visited = true;
    processed_obj.newly_created = true;

    processed_obj
}

fn update_processed_object_2d(
    object: &RenderObject2d,
    proc_obj: &mut ProcessedObject,
    transform: &Matrix4x4,
    is_transform_dirty: bool,
    program: &LinkedProgram,
) {
    // if a parent group or the object itself has had its transform updated
    proc_obj.updated = is_transform_dirty;

    if proc_obj.anim_frame.copy_if_stale(object.get_active_anim_frame()) {
        proc_obj.anim_frame_updated = true;
    }

    proc_obj.active = object.is_active();

    if !is_transform_dirty || !proc_obj.active {
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

    let _buffer_map_guard = proc_obj.staging_buffer.map_write();

    let mut cur_vertex_index: usize = 0;
    for prim in object.get_primitives() {
        #[allow(unused_assignments, clippy::identity_op)]
        for vertex in &prim.vertices {
            let mut cursor = cur_vertex_index * vertex_len as usize * size_of::<GLfloat>();

            let pos_vec = Vector4f {
                x: vertex.position.x,
                y: vertex.position.y,
                z: 0.0,
                w: 1.0,
            };
            let transformed_pos = {
                let pos = transform * pos_vec;
                Vector2f::new(pos.x, pos.y)
            };
            proc_obj.staging_buffer.write_val(transformed_pos, cursor);
            cursor += size_of_val(&transformed_pos);

            cur_vertex_index += 1;
        }
    }

    proc_obj.visited = true;
    proc_obj.updated = true;
}

pub(crate) fn deinit_object_2d(_obj: &mut ProcessedObject) {
    // buffer is implicitly deleted on drop
}
