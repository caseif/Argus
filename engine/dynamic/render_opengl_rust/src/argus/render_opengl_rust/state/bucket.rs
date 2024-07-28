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

use crate::argus::render_opengl_rust::util::buffer::GlBuffer;
use crate::argus::render_opengl_rust::util::gl_util::{GlArrayHandle, GlBufferHandle};

use lowlevel_rustabi::argus::lowlevel::Vector2f;
use resman_rustabi::argus::resman::Resource;

use std::{ffi, ptr};
use crate::argus::render_opengl_rust::state::ProcessedObject;

pub(crate) struct RenderBucketKey {
    material_uid: String,
    atlas_stride: Vector2f,
    z_index: u32,
    light_opacity: f32,
}

pub(crate) struct RenderBucket {
    material_res: Resource,
    atlas_stride: Vector2f,
    z_index: u32,
    light_opacity: f32,
    objects: Vec<ProcessedObject>,
    vertex_buffer: Option<GlBufferHandle>,
    anim_frame_buffer: Option<GlBufferHandle>,
    anim_frame_buffer_staging: *mut ffi::c_void,
    vertex_array: Option<GlArrayHandle>,
    vertex_count: usize,
    obj_ubo: Option<GlBuffer>,
    needs_rebuild: bool,
}

impl RenderBucket {
    pub(crate) fn create(
        material_res: Resource,
        atlas_stride: Vector2f,
        z_index: u32,
        light_opacity: f32,
    ) -> Self {
        Self {
            material_res,
            atlas_stride,
            z_index,
            light_opacity,
            objects: Default::default(),
            vertex_buffer: Default::default(),
            anim_frame_buffer: Default::default(),
            anim_frame_buffer_staging: ptr::null_mut(),
            vertex_array: Default::default(),
            vertex_count: 0,
            obj_ubo: Default::default(),
            needs_rebuild: true,
        }
    }
}
