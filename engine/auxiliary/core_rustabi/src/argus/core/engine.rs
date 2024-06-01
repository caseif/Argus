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

use std::convert::TryFrom;

use num_enum::{IntoPrimitive, TryFromPrimitive};

use crate::core_cabi;
use crate::core_cabi::*;

pub type NullaryCallback = extern "C" fn();
pub type DeltaCallback = extern "C" fn(u64);

#[derive(Eq, Ord, PartialEq, PartialOrd, IntoPrimitive, TryFromPrimitive)]
#[repr(u32)]
pub enum Ordering {
    First = ORDERING_FIRST as u32,
    Early = ORDERING_EARLY as u32,
    Standard = ORDERING_STANDARD as u32,
    Late = ORDERING_LATE as u32,
    Last = ORDERING_LAST as u32,
}

pub fn initialize_engine() {
    unsafe {
        argus_initialize_engine();
    }
}

pub fn start_engine(callback: DeltaCallback) -> ! {
    unsafe {
        argus_start_engine(Some(callback));
    }
}

pub fn get_current_lifecycle_stage() -> LifecycleStage {
    unsafe {
        return LifecycleStage::try_from(argus_get_current_lifecycle_stage()).unwrap();
    }
}

pub fn register_update_callback(update_callback: DeltaCallback, ordering: Ordering) -> Index {
    unsafe {
        return argus_register_render_callback(Some(update_callback), ordering as bindings::Ordering);
    }
}

pub fn unregister_update_callback(id: Index) {
    unsafe {
        argus_unregister_update_callback(id);
    }
}

pub fn register_render_callback(render_callback: DeltaCallback, ordering: Ordering) -> Index {
    unsafe {
        return argus_register_render_callback(Some(render_callback), ordering as bindings::Ordering);
    }
}

pub fn unregister_render_callback(id: Index) {
    unsafe {
        argus_unregister_render_callback(id);
    }
}

pub fn run_on_game_thread(callback: NullaryCallback) {
    unsafe {
        return argus_run_on_game_thread(Some(callback));
    }
}

pub fn is_current_thread_update_thread() -> bool {
    unsafe {
        return argus_is_current_thread_update_thread();
    }
}
