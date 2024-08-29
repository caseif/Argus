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

use crate::argus::render_opengl_rust::shaders::deinit_program;
use crate::argus::render_opengl_rust::state::{RenderBucketKey, RendererState};
use crate::argus::render_opengl_rust::textures::release_texture;
use crate::argus::render_opengl_rust::util::gl_util::{try_delete_buffer, try_delete_vertex_array};

pub(crate) fn deinit_material(state: &mut RendererState, material: &str) {
    println!("De-initializing material {material}"); //TODO

    for scene_state in state.scene_states_2d.values_mut() {
        scene_state.render_buckets.retain(|key, bucket| {
            if key.material_uid == material {
                bucket.vertex_array.inspect(|arr| try_delete_vertex_array(*arr));
                bucket.vertex_buffer.inspect(|buf| try_delete_buffer(*buf));
                bucket.anim_frame_buffer.inspect(|buf| try_delete_buffer(*buf));

                // object ubo will be implicitly deleted in GL when bucket is dropped

                return false;
            }

            return true;
        });
    }

    if let Some(program) = state.linked_programs.remove(material) {
        deinit_program(program.handle);
    }

    let mat_tex = state.material_textures.get(material).cloned();
    if let Some((texture_uid, _)) = mat_tex {
        release_texture(state, texture_uid.as_str());
    }
}
