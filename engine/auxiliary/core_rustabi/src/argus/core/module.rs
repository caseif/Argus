/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

use num_enum::TryFromPrimitive;

use crate::core_cabi;
use crate::core_cabi::*;
use crate::util::*;

#[derive(Eq, Ord, PartialEq, PartialOrd, TryFromPrimitive)]
#[repr(u32)]
pub enum LifecycleStage {
    Load = LIFECYCLE_STAGE_LOAD as u32,
    PreInit = LIFECYCLE_STAGE_PREINIT as u32,
    Init = LIFECYCLE_STAGE_INIT as u32,
    PostInit = LIFECYCLE_STAGE_POSTINIT as u32,
    Running = LIFECYCLE_STAGE_RUNNING as u32,
    PreDeinit = LIFECYCLE_STAGE_PREDEINIT as u32,
    Deinit = LIFECYCLE_STAGE_DEINIT as u32,
    PostDeinit = LIFECYCLE_STAGE_POSTDEINIT as u32,
}

pub type LifecycleUpdateCallback = extern "C" fn(bindings::LifecycleStage);

pub fn lifecycle_stage_to_str(stage: LifecycleStage) -> String {
    unsafe {
        return cstr_to_string(argus_lifecycle_stage_to_str(stage as u32));
    }
}

pub fn register_dynamic_module(id: &str, lifecycle_callback: LifecycleUpdateCallback) {
    unsafe {
        argus_register_dynamic_module(str_to_cstring(id).as_ptr(), Some(lifecycle_callback));
    }
}

pub fn enable_dynamic_module(module_id: &str) -> bool {
    unsafe {
        return argus_enable_dynamic_module(str_to_cstring(module_id).as_ptr());
    }
}

//TODO: argus_get_present_dynamic_modules
