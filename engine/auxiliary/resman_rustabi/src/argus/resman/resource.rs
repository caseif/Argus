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
use num_enum::{IntoPrimitive, TryFromPrimitive};
use std::path::PathBuf;

use crate::resman_cabi::*;

pub struct Resource {
    handle: argus_resource_const_t,
}

pub struct ResourcePrototype {
    pub uid: String,
    pub media_type: String,
    pub fs_path: PathBuf,
}

pub(crate) unsafe fn unwrap_resource_prototype(
    proto: &argus_resource_prototype_t,
) -> ResourcePrototype {
    ResourcePrototype {
        uid: cstr_to_string(proto.uid),
        media_type: cstr_to_string(proto.media_type),
        fs_path: PathBuf::from(cstr_to_str(proto.fs_path)),
    }
}

impl Resource {
    #[allow(dead_code)]
    pub(crate) fn get_handle(&self) -> argus_resource_const_t {
        self.handle
    }

    pub fn of(handle: argus_resource_const_t) -> Self {
        Self { handle }
    }

    pub fn get_prototype(&self) -> ResourcePrototype {
        unsafe { unwrap_resource_prototype(&argus_resource_get_prototype(self.handle)) }
    }

    pub fn release(&self) -> () {
        unsafe { argus_resource_release(self.handle) }
    }

    pub fn get_data_ptr(&self) -> *const u8 {
        unsafe { argus_resource_get_data_ptr(self.handle).cast() }
    }

    pub fn get<'a, T>(&'a self) -> &'a T {
        unsafe {
            let t_ptr: *const T = argus_resource_get_data_ptr(self.handle).cast();
            t_ptr.as_ref().unwrap()
        }
    }
}

#[derive(Debug, Eq, Ord, PartialEq, PartialOrd, IntoPrimitive, TryFromPrimitive)]
#[repr(u32)]
pub enum ResourceErrorReason {
    Generic = RESOURCE_ERROR_REASON_GENERIC,
    NotFound = RESOURCE_ERROR_REASON_NOT_FOUND,
    NotLoaded = RESOURCE_ERROR_REASON_NOT_LOADED,
    AlreadyLoaded = RESOURCE_ERROR_REASON_ALREADY_LOADED,
    NoLoader = RESOURCE_ERROR_REASON_NO_LOADER,
    LoadFailed = RESOURCE_ERROR_REASON_LOAD_FAILED,
    MalformedContent = RESOURCE_ERROR_REASON_MALFORMED_CONTENT,
    InvalidContent = RESOURCE_ERROR_REASON_INVALID_CONTENT,
    UnsupportedContent = RESOURCE_ERROR_REASON_UNSUPPORTED_CONTENT,
    UnexpectedReferenceType = RESOURCE_ERROR_REASON_UNEXPECTED_REFERENCE_TYPE,
}

#[derive(Debug)]
pub struct ResourceError {
    pub reason: ResourceErrorReason,
    pub uid: String,
    pub info: String,
}

impl ResourceError {
    pub fn new(reason: ResourceErrorReason, uid: &str, info: &str) -> Self {
        Self {
            reason,
            uid: uid.to_string(),
            info: info.to_string(),
        }
    }
}

impl From<argus_resource_error_t> for ResourceError {
    fn from(value: argus_resource_error_t) -> Self {
        unsafe {
            Self {
                reason: ResourceErrorReason::try_from(argus_resource_error_get_reason(value))
                    .expect("Invalid ResourceErrorReason ordinal"),
                uid: cstr_to_string(argus_resource_error_get_uid(value)),
                info: cstr_to_string(argus_resource_error_get_info(value)),
            }
        }
    }
}

impl Into<argus_resource_error_t> for ResourceError {
    fn into(self) -> argus_resource_error_t {
        unsafe {
            let uid_c = string_to_cstring(&self.uid);
            let info_c = string_to_cstring(&self.info);

            argus_resource_error_new(
                self.reason.into(),
                uid_c.as_ptr(),
                info_c.as_ptr(),
            )
        }
    }
}
