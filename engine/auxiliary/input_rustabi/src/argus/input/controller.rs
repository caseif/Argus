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
use std::{ffi, ptr};
use std::ffi::{c_void, CStr, CString};
use num_enum::{IntoPrimitive, UnsafeFromPrimitive};
use lowlevel_rustabi::argus::lowlevel::FfiWrapper;
use crate::argus::input::{GamepadAxis, GamepadButton, HidDeviceId, KeyboardScancode, MouseAxis, MouseButton};
use crate::input_cabi::*;

#[repr(u32)]
#[derive(Clone, Debug, IntoPrimitive, UnsafeFromPrimitive)]
pub enum DeadzoneShape {
    Ellipse = DEADZONE_SHAPE_ELLIPSE,
    Quad = DEADZONE_SHAPE_QUAD,
    Cross = DEADZONE_SHAPE_CROSS,
    MaxValue = DEADZONE_SHAPE_MAXVALUE,
}

pub struct Controller {
    handle: argus_controller_t,
}

impl FfiWrapper for Controller {
    fn of(handle: *mut c_void) -> Self {
        Self { handle }
    }
}

impl Controller {
    pub fn get_name(&self) -> String {
        unsafe {
            CStr::from_ptr(argus_controller_get_name(self.handle)).to_string_lossy().to_string()
        }
    }

    pub fn has_gamepad(&self) -> bool {
        unsafe { argus_controller_has_gamepad(self.handle) }
    }

    pub fn attach_gamepad(&mut self, id: HidDeviceId) {
        unsafe { argus_controller_attach_gamepad(self.handle, id); }
    }

    pub fn attach_first_available_gamepad(&mut self) -> bool {
        unsafe { argus_controller_attach_first_available_gamepad(self.handle) }
    }

    pub fn detach_gamepad(&mut self) {
        unsafe { argus_controller_detach_gamepad(self.handle); }
    }

    pub fn get_gamepad_name(&self) -> String {
        unsafe {
            CStr::from_ptr(argus_controller_get_gamepad_name(self.handle))
                .to_string_lossy().to_string()
        }
    }

    pub fn get_deadzone_radius(&self) -> f64 {
        unsafe { argus_controller_get_deadzone_radius(self.handle) }
    }

    pub fn set_deadzone_radius(&mut self, radius: f64) {
        unsafe { argus_controller_set_deadzone_radius(self.handle, radius); }
    }

    pub fn clear_deadzone_radius(&mut self) {
        unsafe { argus_controller_clear_deadzone_radius(self.handle); }
    }

    pub fn get_deadzone_shape(&self) -> DeadzoneShape {
        unsafe {
            DeadzoneShape::unchecked_transmute_from(
                argus_controller_get_deadzone_shape(self.handle)
            )
        }
    }

    pub fn set_deadzone_shape(&mut self, shape: DeadzoneShape) {
        unsafe { argus_controller_set_deadzone_shape(self.handle, shape.into()); }
    }

    pub fn clear_deadzone_shape(&mut self) {
        unsafe { argus_controller_clear_deadzone_shape(self.handle) }
    }

    pub fn get_axis_deadzone_radius(&self, axis: GamepadAxis) -> f64 {
        unsafe { argus_controller_get_axis_deadzone_radius(self.handle, axis.into()) }
    }

    pub fn set_axis_deadzone_radius(&mut self, axis: GamepadAxis, radius: f64) {
        unsafe { argus_controller_set_axis_deadzone_radius(self.handle, axis.into(), radius); }
    }

    pub fn clear_axis_deadzone_radius(&mut self, axis: GamepadAxis) {
        unsafe { argus_controller_clear_axis_deadzone_radius(self.handle, axis.into()); }
    }

    pub fn get_axis_deadzone_shape(&self, axis: GamepadAxis) -> DeadzoneShape {
        unsafe {
            DeadzoneShape::unchecked_transmute_from(
                argus_controller_get_axis_deadzone_shape(self.handle, axis.into())
            )
        }
    }

    pub fn set_axis_deadzone_shape(&mut self, axis: GamepadAxis, shape: DeadzoneShape) {
        unsafe { argus_controller_set_axis_deadzone_shape(self.handle, axis.into(), shape.into()); }
    }

    pub fn clear_axis_deadzone_shape(&mut self, axis: GamepadAxis) {
        unsafe { argus_controller_clear_axis_deadzone_shape(self.handle, axis.into()); }
    }

    pub fn unbind_action(&mut self, action: &str) {
        unsafe {
            let action_c = CString::new(action).unwrap();
            argus_controller_unbind_action(self.handle, action_c.as_ptr());
        }
    }

    pub fn get_keyboard_key_bindings(&self, key: KeyboardScancode) -> Vec<String> {
        unsafe {
            let count = argus_controller_get_keyboard_key_bindings_count(
                self.handle,
                key.clone().into(),
            );
            let mut bindings: Vec<*const ffi::c_char> = Vec::with_capacity(count);
            bindings.resize(count, ptr::null());
            argus_controller_get_keyboard_key_bindings(
                self.handle,
                key.into(),
                bindings.as_mut_ptr(),
                count,
            );
            bindings.into_iter()
                .map(|action| CStr::from_ptr(action).to_string_lossy().to_string())
                .collect()
        }
    }

    pub fn get_keyboard_action_bindings(&self, action: &str) -> Vec<KeyboardScancode> {
        let action_c = CString::new(action).unwrap();
        unsafe {
            let count = argus_controller_get_keyboard_action_bindings_count(
                self.handle,
                action_c.as_ptr(),
            );
            let mut bindings: Vec<KeyboardScancode> = Vec::with_capacity(count);
            bindings.resize(count, KeyboardScancode::Unknown);
            argus_controller_get_keyboard_action_bindings(
                self.handle,
                action_c.as_ptr(),
                bindings.as_mut_ptr().cast(),
                count,
            );
            bindings
        }
    }

    pub fn bind_keyboard_key(&mut self, key: KeyboardScancode, action: &str) {
        let action_c = CString::new(action).unwrap();
        unsafe { argus_controller_bind_keyboard_key(self.handle, key.into(), action_c.as_ptr()); }
    }

    pub fn unbind_keyboard_key(&mut self, key: KeyboardScancode) {
        unsafe { argus_controller_unbind_keyboard_key(self.handle, key.into()); }
    }

    pub fn unbind_keyboard_key_action(&mut self, key: KeyboardScancode, action: &str) {
        let action_c = CString::new(action).unwrap();
        unsafe {
            argus_controller_unbind_keyboard_key_action(self.handle, key.into(), action_c.as_ptr());
        }
    }

    pub fn bind_mouse_button(&mut self, button: MouseButton, action: &str) {
        let action_c = CString::new(action).unwrap();
        unsafe {
            argus_controller_bind_mouse_button(self.handle, button.into(), action_c.as_ptr());
        }
    }

    pub fn unbind_mouse_button(&mut self, button: MouseButton) {
        unsafe { argus_controller_unbind_mouse_button(self.handle, button.into()); }
    }

    pub fn unbind_mouse_button_action(&mut self, button: MouseButton, action: &str) {
        let action_c = CString::new(action).unwrap();
        unsafe {
            argus_controller_unbind_mouse_button_action(
                self.handle,
                button.into(),
                action_c.as_ptr(),
            );
        }
    }

    pub fn bind_mouse_axis(&mut self, axis: MouseAxis, action: &str) {
        let action_c = CString::new(action).unwrap();
        unsafe { argus_controller_bind_mouse_axis(self.handle, axis.into(), action_c.as_ptr()); }
    }

    pub fn unbind_mouse_axis(&mut self, axis: MouseAxis) {
        unsafe { argus_controller_unbind_mouse_axis(self.handle, axis.into()); }
    }

    pub fn unbind_mouse_axis_action(&mut self, axis: MouseAxis, action: &str) {
        let action_c = CString::new(action).unwrap();
        unsafe {
            argus_controller_unbind_mouse_axis_action(self.handle, axis.into(), action_c.as_ptr());
        }
    }

    pub fn bind_gamepad_button(&mut self, button: GamepadButton, action: &str) {
        let action_c = CString::new(action).unwrap();
        unsafe {
            argus_controller_bind_gamepad_button(self.handle, button.into(), action_c.as_ptr());
        }
    }

    pub fn unbind_gamepad_button(&mut self, button: GamepadButton) {
        unsafe { argus_controller_unbind_gamepad_button(self.handle, button.into()); }
    }

    pub fn unbind_gamepad_button_action(&mut self, button: GamepadButton, action: &str) {
        let action_c = CString::new(action).unwrap();
        unsafe {
            argus_controller_unbind_gamepad_button_action(
                self.handle,
                button.into(),
                action_c.as_ptr(),
            );
        }
    }

    pub fn bind_gamepad_axis(&mut self, axis: GamepadAxis, action: &str) {
        let action_c = CString::new(action).unwrap();
        unsafe { argus_controller_bind_gamepad_axis(self.handle, axis.into(), action_c.as_ptr()); }
    }

    pub fn unbind_gamepad_axis(&mut self, axis: GamepadAxis) {
        unsafe { argus_controller_unbind_gamepad_axis(self.handle, axis.into()); }
    }

    pub fn unbind_gamepad_axis_action(&mut self, axis: GamepadAxis, action: &str) {
        let action_c = CString::new(action).unwrap();
        unsafe {
            argus_controller_unbind_gamepad_axis_action(
                self.handle,
                axis.into(),
                action_c.as_ptr()
            );
        }
    }

    pub fn is_gamepad_button_pressed(&self, button: GamepadButton) -> bool {
        unsafe { argus_controller_is_gamepad_button_pressed(self.handle, button.into()) }
    }

    pub fn get_gamepad_axis(&self, axis: GamepadAxis) -> f64 {
        unsafe { argus_controller_get_gamepad_axis(self.handle, axis.into()) }
    }

    pub fn get_gamepad_axis_delta(&self, axis: GamepadAxis) -> f64 {
        unsafe { argus_controller_get_gamepad_axis_delta(self.handle, axis.into()) }
    }

    pub fn is_action_pressed(&self, action: &str) -> bool {
        let action_c = CString::new(action).unwrap();
        unsafe { argus_controller_is_action_pressed(self.handle, action_c.as_ptr()) }
    }

    pub fn get_action_axis(&self, action: &str) -> f64 {
        let action_c = CString::new(action).unwrap();
        unsafe { argus_controller_get_action_axis(self.handle, action_c.as_ptr()) }
    }

    pub fn get_action_axis_delta(&self, action: &str) -> f64 {
        let action_c = CString::new(action).unwrap();
        unsafe { argus_controller_get_action_axis_delta(self.handle, action_c.as_ptr()) }
    }
}
