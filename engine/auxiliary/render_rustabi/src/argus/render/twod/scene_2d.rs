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
use std::ffi::CStr;
use std::ptr;
use lowlevel_rustabi::argus::lowlevel::{Handle, ValueAndDirtyFlag, Vector2f, Vector3f};
use lowlevel_rustabi::util::str_to_cstring;
use crate::argus::render::*;
use crate::render_cabi::*;

#[derive(Clone)]
pub struct Scene2d {
    handle: argus_scene_2d_t,
}

impl Scene2d {
    pub(crate) fn of(handle: argus_scene_2d_t) -> Self {
        Self { handle }
    }

    pub(crate) fn get_ffi_handle(&self) -> argus_scene_2d_const_t {
        self.handle
    }

    pub fn create(id: &str) -> Self {
        unsafe {
            let id_c = str_to_cstring(id);
            Self::of(argus_scene_2d_create(id_c.as_ptr()))
        }
    }

    pub fn get_id(&self) -> String {
        unsafe { CStr::from_ptr(argus_scene_2d_get_id(self.handle)).to_string_lossy().to_string() }
    }
    
    pub fn is_lighting_enabled(&self) -> bool {
        unsafe { argus_scene_2d_is_lighting_enabled(self.handle) }
    }

    pub fn set_lighting_enabled(&mut self, enabled: bool) {
        unsafe { argus_scene_2d_set_lighting_enabled(self.handle, enabled); }
    }

    pub fn peek_ambient_light_level(&self) -> f32 {
        unsafe { argus_scene_2d_peek_ambient_light_level(self.handle) }
    }

    pub fn get_ambient_light_level(&mut self) -> ValueAndDirtyFlag<f32> {
        unsafe {
            let mut dirty = false;
            let level = argus_scene_2d_get_ambient_light_level(self.handle, &mut dirty);
            ValueAndDirtyFlag::of(level, dirty)
        }
    }

    pub fn set_ambient_light_level(&mut self, level: f32) {
        unsafe { argus_scene_2d_set_ambient_light_level(self.handle, level); }
    }

    pub fn peek_ambient_light_color(&self) -> Vector3f {
        unsafe { argus_scene_2d_peek_ambient_light_color(self.handle).into() }
    }

    pub fn get_ambient_light_color(&mut self) -> ValueAndDirtyFlag<Vector3f> {
        unsafe {
            let mut dirty = false;
            let color = argus_scene_2d_get_ambient_light_color(self.handle, &mut dirty).into();
            ValueAndDirtyFlag::of(color, dirty)
        }
    }

    pub fn set_ambient_light_color(&mut self, color: Vector3f) {
        unsafe { argus_scene_2d_set_ambient_light_color(self.handle, color.into()); }
    }

    pub fn get_lights(&mut self) -> Vec<Light2d> {
        unsafe {
            let count = argus_scene_2d_get_lights_count(self.handle);
            let mut lights: Vec<argus_light_2d_t> = Vec::with_capacity(count);
            lights.resize(count, ptr::null_mut());

            argus_scene_2d_get_lights(self.handle, lights.as_mut_ptr(), count);
            lights.into_iter().map(Light2d::of).collect()
        }
    }

    pub fn get_lights_for_render(&mut self) -> Vec<Light2d> {
        unsafe {
            let count = argus_scene_2d_get_lights_count_for_render(self.handle);
            let mut lights: Vec<argus_light_2d_const_t> = Vec::with_capacity(count);
            lights.resize(count, ptr::null_mut());

            argus_scene_2d_get_lights_for_render(self.handle, lights.as_mut_ptr(), count);
            lights.into_iter().map(|l| Light2d::of(l as argus_light_2d_t)).collect()
        }
    }

    pub fn add_light(
        &mut self,
        light_type: Light2dType,
        is_occludable: bool,
        color: Vector3f,
        params: Light2dParameters,
        initial_transform: Transform2d
    ) -> Handle {
        unsafe {
            argus_scene_2d_add_light(
                self.handle,
                light_type.into(),
                is_occludable,
                color.into(),
                params.into(),
                initial_transform.into()
            ).into()
        }
    }

    pub fn get_light(&mut self, light_handle: Handle) -> Option<Light2d> {
        unsafe {
            argus_scene_2d_get_light(self.handle, light_handle.into())
                .as_mut()
                .map(|ptr| Light2d::of(ptr))
        }
    }

    pub fn remove_light(&mut self, light_handle: Handle) {
        unsafe { argus_scene_2d_remove_light(self.handle, light_handle.into()); }
    }

    pub fn get_group(&mut self, group_handle: Handle) -> Option<RenderGroup2d> {
        unsafe {
            argus_scene_2d_get_group(self.handle, group_handle.into())
                .as_mut()
                .map(|ptr| RenderGroup2d::of(ptr))
        }
    }

    pub fn get_object(&mut self, obj_handle: Handle) -> Option<RenderObject2d> {
        unsafe {
            argus_scene_2d_get_object(self.handle, obj_handle.into())
                .as_mut()
                .map(|ptr| RenderObject2d::of(ptr))
        }
    }

    pub fn add_group(&mut self, transform: ArgusTransform2d) -> Handle {
        unsafe { argus_scene_2d_add_group(self.handle, transform.into()).into() }
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

            let material_c = str_to_cstring(material);
            
            argus_scene_2d_add_object(
                self.handle,
                material_c.as_ptr(),
                prims.as_ptr(),
                prims_count,
                anchor_point.into(),
                atlas_stride.into(),
                z_index,
                light_opacity,
                transform.into(),
            ).into()
        }
    }

    pub fn remove_group(&mut self, group_handle: Handle) {
        unsafe { argus_scene_2d_remove_group(self.handle, group_handle.into()); }
    }

    pub fn remove_object(&mut self, obj_handle: Handle) {
        unsafe { argus_scene_2d_remove_object(self.handle, obj_handle.into()); }
    }

    pub fn find_camera(&self, id: &str) -> Camera2d {
        unsafe {
            let id_c = str_to_cstring(id);
            Camera2d::of(argus_scene_2d_find_camera(self.handle, id_c.as_ptr()))
        }
    }

    pub fn create_camera(&mut self, id: &str) -> Camera2d {
        unsafe {
            let id_c = str_to_cstring(id);
            Camera2d::of(argus_scene_2d_create_camera(self.handle, id_c.as_ptr()))
        }
    }

    pub fn destroy_camera(&mut self, id: &str) {
        unsafe {
            let id_c = str_to_cstring(id);
            argus_scene_2d_destroy_camera(self.handle, id_c.as_ptr());
        }
    }

    pub fn lock_render_state(&mut self) {
        unsafe { argus_scene_2d_lock_render_state(self.handle); }
    }

    pub fn unlock_render_state(&mut self) {
        unsafe { argus_scene_2d_unlock_render_state(self.handle); }
    }
}

impl Into<Scene> for Scene2d {
    fn into(self) -> Scene {
        Scene::of(self.handle)
    }
}
