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
use lowlevel_rustabi::util::*;
use std::ffi::c_char;

use crate::argus::resman::{
    wrap_loader, Resource, ResourceError, ResourceErrorReason, ResourceLoader,
    WrappedResourceLoader,
};
use crate::resman_cabi::*;

unsafe fn _unwrap_result(res: ResourceOrResourceError) -> Result<Resource, ResourceError> {
    if res.is_ok {
        Ok(Resource::of(res.ve.value))
    } else {
        let error = ResourceError {
            reason: ResourceErrorReason::try_from(argus_resource_error_get_reason(res.ve.error))
                .expect("Invalid ResourceErrorReason ordinal"),
            uid: cstr_to_string(argus_resource_error_get_uid(res.ve.error)),
            info: cstr_to_string(argus_resource_error_get_info(res.ve.error)),
        };
        argus_resource_error_destruct(res.ve.error);
        Err(error)
    }
}

pub struct ResourceManager {
    handle: argus_resource_manager_t,
}

impl ResourceManager {
    pub fn of(handle: argus_resource_t) -> Self {
        Self { handle }
    }

    pub fn get_instance() -> Self {
        unsafe {
            Self {
                handle: argus_resource_manager_get_instance(),
            }
        }
    }

    pub(crate) fn get_handle(&mut self) -> argus_resource_manager_t {
        self.handle
    }

    pub fn discover_resources(&self) {
        unsafe {
            argus_resource_manager_discover_resources(self.handle);
        }
    }

    pub fn add_memory_package(&mut self, buf: *const u8, len: usize) {
        unsafe { argus_resource_manager_add_memory_package(self.handle, buf, len) }
    }

    pub fn register_loader(
        &mut self,
        media_types: Vec<String>,
        loader: Box<dyn ResourceLoader>,
    ) -> WrappedResourceLoader {
        unsafe {
            let mt_count = media_types.len();
            let mt_cstr_arr: *const *const c_char = string_vec_to_cstr_arr(media_types).into();

            let wrapped_loader = wrap_loader(mt_cstr_arr.into(), mt_count, loader);
            argus_resource_manager_register_loader(
                self.handle,
                wrapped_loader,
            );

            WrappedResourceLoader::of(wrapped_loader)
        }
    }

    pub fn get_resource(&mut self, uid: &str) -> Result<Resource, ResourceError> {
        unsafe {
            _unwrap_result(argus_resource_manager_get_resource(
                self.handle,
                uid.as_ptr().cast(),
            ))
        }
    }

    pub fn get_resource_weak(&mut self, uid: &str) -> Result<Resource, ResourceError> {
        unsafe {
            _unwrap_result(argus_resource_manager_get_resource_weak(
                self.handle,
                uid.as_ptr().cast(),
            ))
        }
    }

    pub fn try_get_resource(&mut self, uid: &str) -> Result<Resource, ResourceError> {
        unsafe {
            _unwrap_result(argus_resource_manager_try_get_resource(
                self.handle,
                uid.as_ptr().cast(),
            ))
        }
    }

    //TODO: get_resource_async

    pub fn create_resource(
        &mut self,
        uid: &str,
        media_type: &str,
        data: &[u8],
    ) -> Result<Resource, ResourceError> {
        unsafe {
            _unwrap_result(argus_resource_manager_create_resource(
                self.handle,
                uid.as_ptr().cast(),
                media_type.as_ptr().cast(),
                data.as_ptr().cast(),
                data.len(),
            ))
        }
    }
}
