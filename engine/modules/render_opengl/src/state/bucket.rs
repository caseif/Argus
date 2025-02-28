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

use crate::util::buffer::GlBuffer;
use crate::util::gl_util::{GlArrayHandle, GlBufferHandle};

use std::cmp::Ordering;
use std::hash::Hash;
use argus_resman::{Resource, ResourceIdentifier};
use argus_util::math::{Vector2f, Vector2i};
use argus_util::pool::Handle;

#[derive(Clone, PartialEq, Eq, Hash)]
pub(crate) struct RenderBucketKey {
    pub(crate) material_uid: ResourceIdentifier,
    pub(crate) atlas_stride: Vector2i,
    pub(crate) z_index: u32,
    pub(crate) light_opacity: i32,
}

impl RenderBucketKey {
    pub(crate) fn new(
        material_uid: ResourceIdentifier,
        atlas_stride: &Vector2f,
        z_index: u32,
        light_opacity: f32
    ) -> Self {
        Self {
            material_uid,
            atlas_stride: Vector2i {
                x: (atlas_stride.x * 1_000_000.0) as i32,
                y: (atlas_stride.y * 1_000_000.0) as i32
            },
            z_index,
            light_opacity: (light_opacity * 1_000_000.0) as i32,
        }
    }
}

pub(crate) struct RenderBucket {
    pub(crate) material_uid: String,
    pub(crate) material_res: Resource,
    pub(crate) atlas_stride: Vector2f,
    pub(crate) z_index: u32,
    pub(crate) light_opacity: f32,
    pub(crate) objects: Vec<Handle>,
    pub(crate) vertex_buffer: Option<GlBufferHandle>,
    pub(crate) anim_frame_buffer: Option<GlBufferHandle>,
    pub(crate) anim_frame_buffer_staging: Vec<u8>,
    pub(crate) vertex_array: Option<GlArrayHandle>,
    pub(crate) vertex_count: usize,
    pub(crate) obj_ubo: Option<GlBuffer>,
    pub(crate) needs_rebuild: bool,
}

impl RenderBucket {
    pub(crate) fn create(
        material_res: Resource,
        atlas_stride: Vector2f,
        z_index: u32,
        light_opacity: f32,
    ) -> Self {
        let material_uid = material_res.get_prototype().uid.to_string();

        Self {
            material_uid,
            material_res,
            atlas_stride,
            z_index,
            light_opacity,
            objects: Default::default(),
            vertex_buffer: Default::default(),
            anim_frame_buffer: Default::default(),
            anim_frame_buffer_staging: Vec::new(),
            vertex_array: Default::default(),
            vertex_count: 0,
            obj_ubo: Default::default(),
            needs_rebuild: true,
        }
    }
}

impl PartialOrd<Self> for RenderBucketKey {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for RenderBucketKey {
    fn cmp(&self, other: &Self) -> Ordering {
        self.z_index.cmp(&other.z_index)
            .then(self.light_opacity.cmp(&other.light_opacity))
            .then(self.atlas_stride.cmp(&other.atlas_stride))
            .then(self.material_uid.cmp(&other.material_uid))
    }
}
