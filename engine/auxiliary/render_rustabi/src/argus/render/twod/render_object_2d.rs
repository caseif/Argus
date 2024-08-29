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

use std::{mem, slice};

use lowlevel_rustabi::argus::lowlevel::{Handle, ValueAndDirtyFlag, Vector2f, Vector2u};
use lowlevel_rustabi::util::cstr_to_str;

use crate::argus::render::{RenderGroup2d, RenderPrimitive2d, Scene2d};
use crate::render_cabi::*;

pub struct RenderObject2d {
    handle: argus_render_object_2d_t,
}

impl RenderObject2d {
    pub(crate) fn of(handle: argus_render_object_2d_t) -> Self {
        Self { handle }
    }

    pub fn get_handle(&self) -> Handle {
        unsafe { argus_render_object_2d_get_handle(self.handle).into() }
    }

    pub fn get_scene(&self) -> Scene2d {
        unsafe { Scene2d::of(mem::transmute(argus_render_object_2d_get_scene(self.handle))) }
    }

    pub fn get_parent(&self) -> RenderGroup2d {
        unsafe { RenderGroup2d::of(mem::transmute(argus_render_object_2d_get_parent(self.handle))) }
    }

    pub fn get_material(&self) -> &str {
        unsafe { cstr_to_str(argus_render_object_2d_get_material(self.handle)) }
    }

    pub fn get_primitives(&self) -> Vec<RenderPrimitive2d> {
        unsafe {
            let count = argus_render_object_2d_get_primitives_count(self.handle);
            let mut buf: Vec<ArgusRenderPrimitive2d> = Vec::with_capacity(count);
            buf.resize(count, mem::zeroed());
            argus_render_object_2d_get_primitives(self.handle, buf.as_mut_ptr(), count);
            buf.into_iter()
                .map(|prim| RenderPrimitive2d {
                    vertices: slice::from_raw_parts(prim.vertices.cast(), prim.vertex_count)
                        .to_vec(),
                })
                .collect()
        }
    }

    pub fn get_anchor_point(&self) -> Vector2f {
        unsafe { argus_render_object_2d_get_anchor_point(self.handle).into() }
    }

    pub fn get_atlas_stride(&self) -> Vector2f {
        unsafe { argus_render_object_2d_get_atlas_stride(self.handle).into() }
    }

    pub fn get_z_index(&self) -> u32 {
        unsafe { argus_render_object_2d_get_z_index(self.handle) }
    }

    pub fn get_light_opacity(&self) -> f32 {
        unsafe { argus_render_object_2d_get_light_opacity(self.handle) }
    }

    pub fn set_light_opacity(&mut self, opacity: f32) {
        unsafe { argus_render_object_2d_set_light_opacity(self.handle, opacity); }
    }

    pub fn get_active_frame(&mut self) -> ValueAndDirtyFlag<Vector2u> {
        unsafe {
            let mut dirty = false;
            let val = argus_render_object_2d_get_active_frame(self.handle, &mut dirty);
            ValueAndDirtyFlag {
                value: val.into(),
                dirty,
            }
        }
    }

    pub fn set_active_frame(&mut self, frame: Vector2u) {
        unsafe { argus_render_object_2d_set_active_frame(self.handle, frame.into()); }
    }

    pub fn peek_transform(&self) -> ArgusTransform2d {
        unsafe { argus_render_object_2d_peek_transform(self.handle).into() }
    }

    pub fn get_transform(&mut self) -> ArgusTransform2d {
        unsafe { argus_render_object_2d_get_transform(self.handle).into() }
    }

    pub fn set_transform(&mut self, transform: ArgusTransform2d) {
        unsafe { argus_render_object_2d_set_transform(self.handle, transform.into()); }
    }

    pub fn copy(&self, parent: RenderGroup2d) -> RenderObject2d {
        unsafe {
            RenderObject2d::of(argus_render_object_2d_copy(self.handle, parent.get_ffi_handle()))
        }
    }
}
