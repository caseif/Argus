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
use std::ffi::{c_char, c_void};
use lowlevel_rustabi::util::{cstr_to_string, string_vec_to_cstr_arr};

use crate::argus::resman::{
    unwrap_resource_prototype, Resource, ResourceError, ResourceManager, ResourcePrototype,
};
use crate::resman_cabi::*;

pub struct WrappedResourceLoader {
    handle: argus_resource_loader_t,
}

impl WrappedResourceLoader {
    pub(crate) fn of(handle: argus_resource_loader_t) -> Self {
        Self { handle }
    }

    pub fn load_dependencies(
        &mut self,
        manager: &mut ResourceManager,
        dependencies: Vec<String>,
    ) -> Result<HashMap<String, Resource>, ResourceError> {
        unsafe {
            let deps_c = string_vec_to_cstr_arr(&dependencies);
            let res = argus_resource_loader_load_dependencies(
                self.handle,
                manager.get_handle(),
                deps_c.as_ptr(),
                dependencies.len(),
            );

            if res.is_ok {
                let v = res.ve.value;

                let mut map = HashMap::<String, Resource>::new();
                for i in 0usize..argus_loaded_dependency_set_get_count(v) {
                    map.insert(
                        cstr_to_string(argus_loaded_dependency_set_get_name_at(v, i)),
                        Resource::of(argus_loaded_dependency_set_get_resource_at(v, i)),
                    );
                }

                argus_loaded_dependency_set_destruct(v);

                Ok(map)
            } else {
                let e = res.ve.error;

                let real_err: ResourceError = e.into();

                argus_resource_error_destruct(e);

                Err(real_err)
            }
        }
    }
}

pub trait ResourceLoader {
    fn load_resource(
        &mut self,
        handle: WrappedResourceLoader,
        manager: ResourceManager,
        prototype: ResourcePrototype,
        read_callback: Box<dyn Fn(&mut [u8], usize) -> usize>,
        size: usize,
    ) -> Result<*mut u8, ResourceError>;

    fn copy_resource(
        &mut self,
        handle: WrappedResourceLoader,
        manager: ResourceManager,
        prototype: ResourcePrototype,
        src_data: *const u8,
    ) -> Result<*mut u8, ResourceError>;

    fn unload_resource(&mut self, handle: WrappedResourceLoader, ptr: *mut u8);
}

unsafe extern "C" fn load_resource_proxy(
    loader: argus_resource_loader_t,
    manager: argus_resource_manager_t,
    prototype: argus_resource_prototype_t,
    read_callback: argus_resource_read_callback_t,
    size: usize,
    user_data: *mut c_void,
    engine_data: *mut c_void,
) -> VoidPtrOrResourceError {
    let read_callback_unwrapped = read_callback.expect("Read callback was nullptr").clone();

    let ptr: *mut Box<dyn ResourceLoader> = std::mem::transmute(user_data);
    let real_loader: &mut Box<dyn ResourceLoader> = &mut *ptr;

    let res = real_loader.load_resource(
        WrappedResourceLoader::of(loader),
        ResourceManager::of(manager),
        unwrap_resource_prototype(prototype),
        Box::new(move |dst, len|
            read_callback_unwrapped(dst.as_mut_ptr().cast(), len, engine_data)),
        size,
    );
    VoidPtrOrResourceError {
        is_ok: res.is_ok(),
        ve: match res {
            Ok(v) => VoidPtrOrResourceError__bindgen_ty_1 { value: v.cast() },
            Err(e) => VoidPtrOrResourceError__bindgen_ty_1 { error: e.into() },
        },
    }
}

unsafe extern "C" fn copy_resource_proxy(
    loader: argus_resource_loader_t,
    manager: argus_resource_manager_t,
    prototype: argus_resource_prototype_t,
    src: *const c_void,
    _src_len: usize,
    user_data: *mut c_void,
) -> VoidPtrOrResourceError {
    let real_loader: &mut Box<dyn ResourceLoader> = std::mem::transmute(user_data);
    let res = real_loader.copy_resource(
        WrappedResourceLoader::of(loader),
        ResourceManager::of(manager),
        unwrap_resource_prototype(prototype),
        src.cast(),
    );
    // re-wrap the loader
    Box::into_raw(Box::new(real_loader));
    VoidPtrOrResourceError {
        is_ok: res.is_ok(),
        ve: match res {
            Ok(v) => VoidPtrOrResourceError__bindgen_ty_1 { value: v.cast() },
            Err(e) => VoidPtrOrResourceError__bindgen_ty_1 { error: e.into() },
        },
    }
}

unsafe extern "C" fn unload_resource_proxy(
    loader: argus_resource_loader_t,
    ptr: *mut c_void,
    len: usize,
    user_data: *mut c_void,
) {
    let real_loader: &mut Box<dyn ResourceLoader> = std::mem::transmute(user_data);
    real_loader.unload_resource(
        WrappedResourceLoader::of(loader),
        ptr.cast(),
    );
}

pub(crate) unsafe fn wrap_loader(
    media_types: *const *const c_char,
    media_types_count: usize,
    resource_len: usize,
    loader: Box<dyn ResourceLoader>,
) -> argus_resource_loader_t {
    let loader_boxed = Box::new(loader);
    let loader_ptr: *mut Box<dyn ResourceLoader> = Box::into_raw(loader_boxed);

    argus_resource_loader_new(
        media_types,
        media_types_count,
        resource_len,
        Some(load_resource_proxy),
        Some(copy_resource_proxy),
        Some(unload_resource_proxy),
        loader_ptr.cast(),
    )
}
