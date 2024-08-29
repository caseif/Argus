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
use std::collections::HashMap;
use std::ffi::c_void;
use std::ptr;
use argus_macros::ffi_repr;
use lowlevel_rustabi::argus::lowlevel::Handle;
use crate::argus::render::{Matrix4x4, RenderObject2d, Scene2d};
use crate::render_cabi::*;

type ProcessedRenderObject2d = argus_processed_render_object_2d_t;

type ProcessRenderObject2dFn = fn(
    obj: &mut RenderObject2d,
    transform: &Matrix4x4,
    extra: *mut c_void
) -> argus_processed_render_object_2d_t;
type UpdateRenderObject2dFn = fn(
    obj: &mut RenderObject2d,
    proc_obj: ProcessedRenderObject2d,
    transform: &Matrix4x4,
    is_transform_dirty: bool,
    extra: *mut c_void,
);

#[repr(C)]
#[ffi_repr(ArgusProcessedObjectMap)]
#[derive(Clone)]
pub struct FfiProcessedObjectMap {
    count: usize,
    capacity: usize,
    keys: *const Handle,
    values: *const ProcessedRenderObject2d,
}

pub struct ProcessedObjectMap<T> {
    pub ffi_map: FfiProcessedObjectMap,
    pub rust_map: HashMap<Handle, *mut T>,
}

struct ObjProcContext {
    process_new_fn: ProcessRenderObject2dFn,
    update_fn: UpdateRenderObject2dFn,
    extra: *mut c_void,
}

unsafe extern "C" fn process_object_proxy(
    obj: argus_render_object_2d_const_t,
    transform: argus_matrix_4x4_t,
    context_ptr: *mut c_void
) -> argus_processed_render_object_2d_t {
    let context: &ObjProcContext = &*context_ptr.cast();

    let mut obj = RenderObject2d::of(obj.cast_mut());
    let mat = transform.into();
    (context.process_new_fn)(
        &mut obj,
        &mat,
        context.extra,
    ).cast()
}

unsafe extern "C" fn update_object_proxy(
    obj: argus_render_object_2d_const_t,
    proc_obj: argus_processed_render_object_2d_t,
    transform: argus_matrix_4x4_t,
    is_transform_dirty: bool,
    context_ptr: *mut c_void,
) {
    let context: &ObjProcContext = &*context_ptr.cast();

    let mut obj = RenderObject2d::of(obj.cast_mut());
    let tfm = transform.into();
    (context.update_fn)(
        &mut obj,
        proc_obj,
        &tfm,
        is_transform_dirty,
        context.extra,
    );
}

pub fn process_objects_2d(
    scene: &Scene2d,
    obj_map: &mut FfiProcessedObjectMap,
    process_new_fn: ProcessRenderObject2dFn,
    update_fn: UpdateRenderObject2dFn,
    extra: *mut c_void,
) {
    unsafe {
        let mut context = ObjProcContext {
            process_new_fn,
            update_fn,
            extra,
        };

        argus_process_objects_2d(
            scene.get_ffi_handle(),
            <&mut ArgusProcessedObjectMap>::from(obj_map),
            Some(process_object_proxy),
            Some(update_object_proxy),
            ptr::addr_of_mut!(context).cast(),
        );
    }
}

impl<'a, T> ProcessedObjectMap<T> {
    pub fn get(&self, handle: &Handle) -> &'a T {
        unsafe { &**self.rust_map.get(handle).unwrap() }
    }

    pub fn get_mut(&mut self, handle: &Handle) -> &'a mut T {
        unsafe { &mut **self.rust_map.get_mut(handle).unwrap() }
    }

    pub fn rebuild(&mut self) {
        unsafe {
            self.rust_map.clear();
            if self.ffi_map.count > self.rust_map.capacity() {
                self.rust_map.reserve(self.ffi_map.count - self.rust_map.capacity())
            }

            for i in 0..self.ffi_map.count {
                let k = self.ffi_map.keys.add(i);
                let v = self.ffi_map.values.add(i);

                self.rust_map.insert(*k, *v.cast());
            }
        }
    }

    pub fn free(&mut self) {
        unsafe { argus_processed_object_map_free(ptr::addr_of_mut!(self.ffi_map).cast()); }
    }
}

impl<T> Default for ProcessedObjectMap<T> {
    fn default() -> Self {
        ProcessedObjectMap {
            ffi_map: FfiProcessedObjectMap {
                count: 0,
                capacity: 0,
                keys: ptr::null(),
                values: ptr::null(),
            },
            rust_map: HashMap::new(),
        }
    }
}

impl<'a, T> From<&'a ProcessedObjectMap<T>> for &'a HashMap<Handle, *mut T> {
    fn from(value: &'a ProcessedObjectMap<T>) -> Self {
        &value.rust_map
    }
}

impl<'a, T> From<&'a mut ProcessedObjectMap<T>> for &'a mut HashMap<Handle, *mut T> {
    fn from(value: &'a mut ProcessedObjectMap<T>) -> Self {
        &mut value.rust_map
    }
}
