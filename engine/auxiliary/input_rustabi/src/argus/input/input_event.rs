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
use std::ffi::{c_void, CStr};
use num_enum::{IntoPrimitive, TryFromPrimitive, UnsafeFromPrimitive};
use wm_rustabi::argus::wm::Window;
use lowlevel_rustabi::argus::lowlevel::FfiWrapper;
use crate::argus::input::HidDeviceId;
use crate::input_cabi::*;

#[repr(u32)]
#[derive(Clone, Debug, IntoPrimitive, UnsafeFromPrimitive)]
pub enum InputEventType {
    ButtonDown,
    ButtonUp,
    AxisChanged,
}

#[repr(u32)]
#[derive(Clone, Debug, IntoPrimitive, UnsafeFromPrimitive)]
pub enum InputDeviceEventType {
    GamepadConnected,
    GamepadDisconnected,
}

pub struct InputEvent {
    handle: argus_input_event_const_t,
}

impl FfiWrapper for InputEvent {
    fn of(handle: *mut c_void) -> Self {
        Self { handle }
    }
}

impl InputEvent {
    pub fn get_input_type(&self) -> InputEventType {
        unsafe {
            InputEventType::unchecked_transmute_from(argus_input_event_get_input_type(self.handle))
        }
    }

    pub fn get_window(&self) -> Window {
        Window::of(unsafe { argus_input_event_get_window(self.handle).cast_mut() })
    }

    pub fn get_controller_name(&self) -> String {
        unsafe {
            CStr::from_ptr(argus_input_event_get_controller_name(self.handle))
                .to_string_lossy().to_string()
        }
    }

    pub fn get_action(&self) -> String {
        unsafe {
            CStr::from_ptr(argus_input_event_get_action(self.handle))
                .to_string_lossy().to_string()
        }
    }

    pub fn get_axis_value(&self) -> f64 {
        unsafe { argus_input_event_get_axis_value(self.handle) }
    }

    pub fn get_axis_delta(&self) -> f64 {
        unsafe { argus_input_event_get_axis_delta(self.handle) }
    }
}

pub struct InputDeviceEvent {
    handle: argus_input_device_event_const_t,
}

impl FfiWrapper for InputDeviceEvent {
    fn of(handle: *mut c_void) -> Self {
        Self { handle }
    }
}

impl InputDeviceEvent {
    pub fn get_device_event(&self) -> InputDeviceEventType {
        unsafe {
            InputDeviceEventType::unchecked_transmute_from(
                argus_input_event_get_input_type(self.handle)
            )
        }
    }

    pub fn get_controller_name(&self) -> String {
        unsafe {
            CStr::from_ptr(argus_input_event_get_controller_name(self.handle))
                .to_string_lossy().to_string()
        }
    }

    pub fn get_device_id(&self) -> HidDeviceId {
        unsafe { argus_input_device_event_get_device_id(self.handle) }
    }
}
