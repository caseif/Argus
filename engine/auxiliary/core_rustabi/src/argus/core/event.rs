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

use std::ffi::c_void;
use std::mem;
use crate::argus::core::Index;
use crate::argus::core::Ordering;
use crate::core_cabi;
use crate::util::*;

pub use crate::core_cabi::argus_event_t;
pub use crate::core_cabi::argus_event_const_t;

#[repr(u32)]
pub enum TargetThread {
    Update = core_cabi::TARGET_THREAD_UPDATE,
    Render = core_cabi::TARGET_THREAD_RENDER,
}

pub trait ArgusEvent {
    fn of(handle: argus_event_t) -> Self where Self : Sized;

    fn get_type_id(&self) -> String;

    fn get_handle(&self) -> argus_event_t;
}

type EventHandler<E, D> = dyn Fn(&E, *mut D);

extern "C" fn event_handler_trampoline(handle: argus_event_const_t, ctx: *mut c_void) {
    unsafe {
        let closure_ref = &**(ctx as *const Box<dyn Fn(argus_event_const_t)>);
        closure_ref(handle);
    }
}

extern "C" fn clean_up_event_handler(_: Index, ctx: *mut c_void) {
    // drop the closure
    let _: Box<Box<dyn FnMut(argus_event_const_t)>> = unsafe { Box::from_raw(mem::transmute(ctx)) };
}

pub fn register_event_handler<E: ArgusEvent, D>(type_id: &str, handler: &EventHandler<E, D>,
    target_thread: TargetThread, data: *mut D, ordering: Ordering) -> Index {
    unsafe {
        let closure = |handle: argus_event_const_t| {
            handler(&E::of(handle as argus_event_t), data);
        };

        let ctx: Box<Box<dyn Fn(argus_event_const_t)>> = Box::new(Box::new(closure));

        return core_cabi::argus_register_event_handler(
            str_to_cstring(type_id).as_ptr(),
            Some(event_handler_trampoline),
            target_thread as core_cabi::TargetThread,
            Box::into_raw(ctx) as *mut c_void,
            ordering as core_cabi::Ordering,
            Some(clean_up_event_handler)
        );
    }
}

pub fn unregister_event_handler(index: Index) {
    unsafe {
        core_cabi::argus_unregister_event_handler(index);
    }
}

pub fn dispatch_event(event: &dyn ArgusEvent) {
    unsafe {
        core_cabi::argus_dispatch_event(event.get_handle());
    }
}


