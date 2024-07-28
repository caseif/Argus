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
use lowlevel_rustabi::argus::lowlevel::{Vector2f, Vector2u};
use resman_rustabi::argus::resman::Resource;
use crate::argus::render_opengl_rust::util::gl_util::GlBufferHandle;

pub(crate) struct ProcessedObject {
    material_res: Resource,
    atlas_stride: Vector2f,
    z_index: u32,
    light_opacity: f32,

    anim_frame: Vector2u,

    staging_buffer: GlBufferHandle,
    staging_buffer_size: usize,
    vertex_count: usize,
    mapped_buffer: Option<*mut ffi::c_void>,
    newly_created: bool,
    visited: bool,
    updated: bool,
    anim_frame_updated: bool,
}

impl ProcessedObject {
    pub(crate) fn new(material_res: Resource, atlas_stride: Vector2f,
    z_index: u32, light_opacity: f32, staging_buffer: GlBufferHandle, staging_buffer_size: usize,
    vertex_count: usize, mapped_buffer: Option<*mut ffi::c_void>) -> Self {
        Self {
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