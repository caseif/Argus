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

use std::ffi;
use lowlevel_rustabi::argus::lowlevel::{Handle, Vector2f, Vector2u};
use resman_rustabi::argus::resman::Resource;
use crate::argus::render_opengl_rust::util::gl_util::GlBufferHandle;

pub(crate) struct ProcessedObject {
    pub(crate) obj_handle: Handle,

    pub(crate) material_res: Resource,
    pub(crate) atlas_stride: Vector2f,
    pub(crate) z_index: u32,
    pub(crate) light_opacity: f32,

    pub(crate) anim_frame: Vector2u,

    pub(crate) staging_buffer: GlBufferHandle,
    pub(crate) staging_buffer_size: usize,
    pub(crate) vertex_count: usize,
    pub(crate) mapped_buffer: Option<*mut ffi::c_void>,
    pub(crate) newly_created: bool,
    pub(crate) visited: bool,
    pub(crate) updated: bool,
    pub(crate) anim_frame_updated: bool,
}

impl ProcessedObject {
    pub(crate) fn new(obj_handle: Handle, material_res: Resource, atlas_stride: Vector2f,
    z_index: u32, light_opacity: f32, staging_buffer: GlBufferHandle, staging_buffer_size: usize,
    vertex_count: usize, mapped_buffer: Option<*mut ffi::c_void>) -> Self {
        Self {
            obj_handle,
            material_res,
            atlas_stride,
            z_index,
            light_opacity,
            anim_frame: Default::default(),
            staging_buffer,
            staging_buffer_size,
            vertex_count,
            mapped_buffer,
            newly_created: true,
            visited: false,
            updated: false,
            anim_frame_updated: false,
        }
    }
}
