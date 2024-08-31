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
use std::ffi::{c_void, CString};
use num_enum::UnsafeFromPrimitive;
use lowlevel_rustabi::argus::lowlevel::FfiWrapper;
use crate::argus::input::{Controller, DeadzoneShape, GamepadAxis};
use crate::input_cabi::*;

pub struct InputManager {
    handle: argus_input_manager_t,
}

impl FfiWrapper for InputManager {
    fn of(handle: *mut c_void) -> Self {
        Self { handle }
    }
}

impl InputManager {
    pub fn get_instance() -> Self {
        Self { handle: unsafe { argus_input_manager_get_instance() } }
    }

    pub fn get_controller(&mut self, name: &str) -> Controller {
        let name_c = CString::new(name).unwrap();
        unsafe { Controller::of(argus_input_manager_get_controller(self.handle, name_c.as_ptr())) }
    }

    pub fn add_controller(&mut self, name: &str) -> Controller {
        let name_c = CString::new(name).unwrap();
        unsafe { Controller::of(argus_input_manager_add_controller(self.handle, name_c.as_ptr())) }
    }

    pub fn remove_controller(&mut self, name: &str) {
        let name_c = CString::new(name).unwrap();
        unsafe { argus_input_manager_remove_controller(self.handle, name_c.as_ptr()); }
    }

    pub fn get_global_deadzone_radius(&self) -> f64 {
        unsafe { argus_input_manager_get_global_deadzone_radius(self.handle) }
    }

    pub fn set_global_deadzone_radius(&mut self, radius: f64) {
        unsafe { argus_input_manager_set_global_deadzone_radius(self.handle, radius); }
    }

    pub fn get_global_deadzone_shape(&self) -> DeadzoneShape {
        unsafe {
            DeadzoneShape::unchecked_transmute_from(
                argus_input_manager_get_global_deadzone_shape(self.handle)
            )
        }
    }

    pub fn set_global_deadzone_shape(&mut self, shape: DeadzoneShape) {
        unsafe { argus_input_manager_set_global_deadzone_shape(self.handle, shape.into()); }
    }

    pub fn get_global_axis_deadzone_radius(&self, axis: GamepadAxis) -> f64 {
        unsafe { argus_input_manager_get_global_axis_deadzone_radius(self.handle, axis.into()) }
    }

    pub fn set_global_axis_deadzone_radius(&mut self, axis: GamepadAxis, radius: f64) {
        unsafe {
            argus_input_manager_set_global_axis_deadzone_radius(self.handle, axis.into(), radius);
        }
    }

    pub fn clear_global_axis_deadzone_radius(&mut self, axis: GamepadAxis) {
        unsafe { argus_input_manager_clear_global_axis_deadzone_radius(self.handle, axis.into()); }
    }

    pub fn get_global_axis_deadzone_shape(&self, axis: GamepadAxis) -> DeadzoneShape {
        unsafe {
            DeadzoneShape::unchecked_transmute_from(
                argus_input_manager_get_global_axis_deadzone_shape(self.handle, axis.into())
            )
        }
    }

    pub fn set_global_axis_deadzone_shape(&mut self, axis: GamepadAxis, shape: DeadzoneShape) {
        unsafe {
            argus_input_manager_set_global_axis_deadzone_shape(
                self.handle,
                axis.into(),
                shape.into(),
            );
        }
    }

    pub fn clear_global_axis_deadzone_shape(&mut self, axis: GamepadAxis) {
        unsafe { argus_input_manager_clear_global_axis_deadzone_shape(self.handle, axis.into()); }
    }
}
