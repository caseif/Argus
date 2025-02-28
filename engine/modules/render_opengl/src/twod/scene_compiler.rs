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

use std::collections::BTreeMap;
use argus_render::constants::*;
use argus_render::util::process_objects_2d;
use crate::aglet::*;
use crate::state::*;
use crate::twod::{deinit_object_2d, process_object};
use crate::util::buffer::GlBuffer;

fn get_bucket_key(processed_obj: &ProcessedObject) -> RenderBucketKey {
    RenderBucketKey::new(
        processed_obj.material_res.get_prototype().uid.clone(),
        &processed_obj.atlas_stride,
        processed_obj.z_index,
        processed_obj.light_opacity
    )
}

fn create_obj_ubo(bucket: &mut RenderBucket) {
    bucket.obj_ubo = Some(
        GlBuffer::new(GL_UNIFORM_BUFFER, SHADER_UBO_OBJ_LEN as usize, GL_STATIC_DRAW, true, false)
    );
    let ubo = &bucket.obj_ubo.as_ref().unwrap();

    // we assume that these values will never change

    let stride = [bucket.atlas_stride.x, bucket.atlas_stride.y];
    ubo.write_val(&stride, SHADER_UNIFORM_OBJ_UV_STRIDE_OFF as usize);

    let light_opacity = bucket.light_opacity;
    ubo.write_val(&light_opacity, SHADER_UNIFORM_OBJ_LIGHT_OPACITY_OFF as usize);
}

fn handle_new_obj(
    render_buckets: &mut BTreeMap<RenderBucketKey, RenderBucket>,
    processed_obj: &mut ProcessedObject
) {
    let key = get_bucket_key(processed_obj);
    let bucket = render_buckets.entry(key)
        .or_insert_with(|| {
            let mut bucket = RenderBucket::create(
                processed_obj.material_res.clone(),
                processed_obj.atlas_stride,
                processed_obj.z_index,
                processed_obj.light_opacity
            );
            create_obj_ubo(&mut bucket);
            bucket
        });

    processed_obj.newly_created = false;

    bucket.objects.push(processed_obj.obj_handle);
    bucket.needs_rebuild = true;
}

fn handle_stale_obj(
    render_buckets: &mut BTreeMap<RenderBucketKey, RenderBucket>,
    processed_obj: &mut ProcessedObject
) {
    deinit_object_2d(processed_obj);

    // we need to remove it from its containing bucket and flag the bucket for a rebuild
    let bucket = render_buckets.get_mut(&get_bucket_key(processed_obj)).unwrap();
    bucket.objects
        .remove(bucket.objects.iter().position(|h| h == &processed_obj.obj_handle).unwrap());
    bucket.needs_rebuild = true;
}

pub(crate) fn compile_scene_2d(renderer_state: &mut RendererState, scene_id: impl AsRef<str>) {
    process_objects_2d(
        scene_id.as_ref(),
        process_object,
        renderer_state,
    );

    let scene_state = renderer_state.scene_states_2d
        .entry(scene_id.as_ref().to_string())
        .or_insert_with(|| {
            Scene2dState {
                scene_id: scene_id.as_ref().to_string(),
                ubo: None,
                render_buckets: Default::default(),
                processed_objs: Default::default(),
            }
        });

    scene_state.processed_objs.retain(|_, obj| {
        if obj.newly_created {
            handle_new_obj(&mut scene_state.render_buckets, obj);
        } else if !obj.visited {
            // wasn't visited this iteration, must not  be present in the scene graph anymore
            handle_stale_obj(&mut scene_state.render_buckets, obj);
            return false;
        }

        obj.visited = false;
        return true;
    });
}
