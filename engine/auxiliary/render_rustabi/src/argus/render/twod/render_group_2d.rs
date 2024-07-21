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

use lowlevel_rustabi::argus::lowlevel::{Handle, Vector2f};
use lowlevel_rustabi::util::str_to_cstring;

use crate::argus::render::{RenderPrimitive2d, Scene2d, Transform2d};
use crate::render_cabi::*;

pub struct RenderGroup2d {
    handle: argus_render_group_2d_t,
}

impl RenderGroup2d {
    pub(crate) fn of(handle: argus_render_group_2d_t) -> Self {
        Self { handle }
    }

    pub(crate) fn get_ffi_handle(&self) -> argus_render_group_2d_t {
        self.handle
    }

    pub fn get_handle(&self) -> Handle {
        unsafe { argus_render_group_2d_get_handle(self.handle).into() }
    }

    pub fn get_scene(&self) -> Scene2d {
        unsafe { Scene2d::of(argus_render_group_2d_get_scene(self.handle)) }
    }

    pub fn get_parent(&self) -> Option<RenderGroup2d> {
        unsafe {
            argus_render_group_2d_get_parent(self.handle).as_mut()
                .map(|handle| RenderGroup2d::of(handle))
        }
    }

    pub fn add_group(&mut self, transform: Transform2d) -> Handle {
        unsafe { argus_render_group_2d_add_group(self.handle, transform.into()).into() }
    }

    pub fn add_object(
        &mut self,
        material: &str,
        primitives: &Vec<RenderPrimitive2d>,
        anchor_point: Vector2f,
        atlas_stride: Vector2f,
        z_index: u32,
        light_opacity: f32,
        transform: Transform2d
    ) -> Handle {
        unsafe {
            let prims_count = primitives.len();
            let prims: Vec<ArgusRenderPrimitive2d> = primitives.into_iter()
                .map(|prim| prim.into())
                .collect();

            argus_render_group_2d_add_object(
                self.handle,
                str_to_cstring(material).as_ptr(),
                prims.as_ptr(),
                prims_count,
                anchor_point.into(),
                atlas_stride.into(),
                z_index,
                light_opacity,
                transform.into()
            ).into()
        }
    }

    pub fn remove_group(&mut self, group_handle: Handle) {
        unsafe { argus_render_group_2d_remove_group(self.handle, group_handle.into()) }
    }

    pub fn remove_object(&mut self, obj_handle: Handle) {
        unsafe { argus_render_group_2d_remove_object(self.handle, obj_handle.into()) }
    }

    pub fn peek_transform(&self) -> Transform2d {
        unsafe { argus_render_group_2d_peek_transform(self.handle).into() }
    }

    pub fn get_transform(&mut self) -> Transform2d {
        unsafe { argus_render_group_2d_get_transform(self.handle).into() }
    }

    pub fn set_transform(&mut self, transform: Transform2d) {
        unsafe { argus_render_group_2d_set_transform(self.handle, transform.into()) }
    }

    pub fn copy(&mut self) -> RenderGroup2d {
        unsafe { RenderGroup2d::of(argus_render_group_2d_copy(self.handle)) }
    }
}
