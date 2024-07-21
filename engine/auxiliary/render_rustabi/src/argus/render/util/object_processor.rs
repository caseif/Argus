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

type ProcessRenderObject2dFn = fn(obj: RenderObject2d, transform: Matrix4x4) -> *mut u8;
type UpdateRenderObject2dFn = fn(
    obj: RenderObject2d,
    proc_obj: ProcessedRenderObject2d,
    transform: Matrix4x4,
    is_transform_dirty: bool,
);

#[repr(C)]
#[ffi_repr(ArgusProcessedObjectMap)]
struct FfiProcessedObjectMap {
    count: usize,
    capacity: usize,
    keys: *const Handle,
    values: *const ProcessedRenderObject2d,
}

pub struct ProcessedObjectMap {
    ffi_map: FfiProcessedObjectMap,
    rust_map: HashMap<Handle, ProcessedRenderObject2d>,
}

struct ObjProcFns {
    process_new_fn: ProcessRenderObject2dFn,
    update_fn: UpdateRenderObject2dFn,
}

unsafe extern "C" fn process_object_proxy(
    obj: argus_render_object_2d_const_t,
    transform: argus_matrix_4x4_t,
    extra: *mut c_void
) -> argus_processed_render_object_2d_t {
    let fns: &ObjProcFns = *extra.cast();

    (fns.process_new_fn)(
        RenderObject2d::of(obj.cast_mut()),
        transform.into()
    ).cast()
}

unsafe extern "C" fn update_object_proxy(
    obj: argus_render_object_2d_const_t,
    proc_obj: argus_processed_render_object_2d_t,
    transform: argus_matrix_4x4_t,
    is_transform_dirty: bool,
    extra: *mut c_void
) {
    let fns: &ObjProcFns = *extra.cast();

    (fns.update_fn)(
        RenderObject2d::of(obj.cast_mut()),
        proc_obj.into(),
        transform.into(),
        is_transform_dirty
    );
}

pub fn process_objects_2d(
    scene: Scene2d,
    obj_map: &mut ProcessedObjectMap,
    process_new_fn: ProcessRenderObject2dFn,
    update_fn: UpdateRenderObject2dFn,
) {
    unsafe {
        let mut fns = ObjProcFns {
            process_new_fn,
            update_fn,
        };

        argus_process_objects_2d(
            scene.get_ffi_handle(),
            <&mut ArgusProcessedObjectMap>::from(&mut obj_map.ffi_map),
            Some(process_object_proxy),
            Some(update_object_proxy),
            ptr::addr_of_mut!(fns).cast(),
        );
    }
}

impl ProcessedObjectMap {
    fn rebuild(&mut self) {
        unsafe {
            self.rust_map.clear();
            if self.ffi_map.count > self.rust_map.capacity() {
                self.rust_map.reserve(self.ffi_map.count - self.rust_map.capacity())
            }

            for i in 0..self.ffi_map.count {
                let k = self.ffi_map.keys.add(i);
                let v = self.ffi_map.values.add(i);

                self.rust_map.insert(*k, *v);
            }
        }
    }

    pub fn new() -> Self {
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

    pub fn free(&mut self) {
        unsafe { argus_processed_object_map_free(ptr::addr_of_mut!(self.ffi_map).cast()); }
    }
}

impl<'a> Into<&'a HashMap<Handle, ProcessedRenderObject2d>> for &'a ProcessedObjectMap {
    fn into(self) -> &'a HashMap<Handle, ProcessedRenderObject2d> {
        &self.rust_map
    }
}
