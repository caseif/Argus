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
use std::ffi::c_void;
use std::ptr;
use argus_macros::ffi_repr;
use crate::lowlevel_cabi;
use crate::lowlevel_cabi::*;

#[repr(C)]
#[ffi_repr(ArgusHandle)]
#[derive(Clone, Copy, Debug, Eq, PartialEq, Hash)]
pub struct Handle {
    index: u32,
    uid: u32,
}

pub struct HandleTable {
    ffi_handle: argus_handle_table_t,
}

impl HandleTable {
    fn of(handle: argus_handle_table_t) -> Self {
        Self { ffi_handle: handle }
    }

    pub fn new() -> Self {
        unsafe { Self::of(argus_handle_table_new()) }
    }

    pub fn delete(&mut self) {
        unsafe { argus_handle_table_delete(self.ffi_handle) }
    }

    pub fn create_handle<T>(&mut self, ptr: &mut T) -> Handle {
        unsafe {
            argus_handle_table_create_handle(self.ffi_handle, ptr::from_mut(ptr).cast()).into()
        }
    }

    pub fn copy_handle(&mut self, handle: Handle) -> Handle {
        unsafe { argus_handle_table_copy_handle(self.ffi_handle, handle.into()).into() }
    }

    pub fn update_handle<T>(&mut self, handle: Handle, ptr: &mut T) -> bool {
        unsafe {
            argus_handle_table_update_handle(
                self.ffi_handle,
                handle.into(),
                ptr::from_mut(ptr).cast()
            )
        }
    }

    pub fn release_handle(&mut self, handle: Handle) {
        unsafe { argus_handle_table_release_handle(self.ffi_handle, handle.into()) }
    }

    pub fn deref<T>(&self, handle: Handle) -> &mut T {
        unsafe {
            argus_handle_table_deref(self.ffi_handle, handle.into()).cast::<T>()
                .as_mut().expect("Failed to dereference Handle")
        }
    }
}
