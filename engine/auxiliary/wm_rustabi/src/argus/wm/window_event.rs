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
use std::time::Duration;
use core_rustabi::argus::core::ArgusEvent;
use core_rustabi::core_cabi::argus_event_t;
use num_enum::{IntoPrimitive, UnsafeFromPrimitive};

use lowlevel_rustabi::argus::lowlevel::{Vector2i, Vector2u};

use crate::argus::wm::Window;
use crate::wm_cabi::*;

#[derive(Eq, Ord, PartialEq, PartialOrd, IntoPrimitive, UnsafeFromPrimitive)]
#[repr(u32)]
pub enum WindowEventType {
    Create = ARGUS_WINDOW_EVENT_TYPE_CREATE,
    Update = ARGUS_WINDOW_EVENT_TYPE_UPDATE,
    RequestClose = ARGUS_WINDOW_EVENT_TYPE_REQUEST_CLOSE,
    Minimize = ARGUS_WINDOW_EVENT_TYPE_MINIMIZE,
    Restore = ARGUS_WINDOW_EVENT_TYPE_RESTORE,
    Focus = ARGUS_WINDOW_EVENT_TYPE_FOCUS,
    Unfocus = ARGUS_WINDOW_EVENT_TYPE_UNFOCUS,
    Resize = ARGUS_WINDOW_EVENT_TYPE_RESIZE,
    Move = ARGUS_WINDOW_EVENT_TYPE_MOVE,
}

pub struct WindowEvent {
    handle: argus_window_event_t,
}

impl WindowEvent {
    pub fn get_subtype(&self) -> WindowEventType {
        unsafe {
            WindowEventType::unchecked_transmute_from(argus_window_event_get_subtype(self.handle))
        }
    }

    pub fn get_window(&self) -> Window {
        unsafe { Window::of(argus_window_event_get_window(self.handle)) }
    }

    pub fn get_resolution(&self) -> Vector2u {
        unsafe { argus_window_event_get_resolution(self.handle).into() }
    }

    pub fn get_position(&self) -> Vector2i {
        unsafe { argus_window_event_get_position(self.handle).into() }
    }

    pub fn get_delta(&self) -> Duration {
        unsafe { Duration::from_micros(argus_window_event_get_delta_us(self.handle)) }
    }
}

impl ArgusEvent for WindowEvent {
    fn get_type_id() -> &'static str {
        "window"
    }

    fn of(handle: argus_event_t) -> Self
    where Self: Sized
    {
        Self { handle }
    }

    fn get_handle(&self) -> argus_event_t {
        self.handle
    }
}
