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
use std::mem;
use num_enum::{IntoPrimitive, TryFromPrimitive};
use lowlevel_rustabi::argus::lowlevel::Vector2d;
use crate::input_cabi::*;

#[repr(u32)]
#[derive(Clone, Debug, IntoPrimitive, TryFromPrimitive)]
pub enum MouseButton {
    Primary = MOUSE_BUTTON_PRIMARY,
    Secondary = MOUSE_BUTTON_SECONDARY,
    Middle = MOUSE_BUTTON_MIDDLE,
    Back = MOUSE_BUTTON_BACK,
    Forward = MOUSE_BUTTON_FORWARD,
}

#[repr(u32)]
#[derive(Clone, Debug, IntoPrimitive, TryFromPrimitive)]
pub enum MouseAxis {
    Horizontal = MOUSE_AXIS_HORIZONTAL,
    Vertical = MOUSE_AXIS_VERTICAL,
}

pub fn mouse_delta() -> Vector2d {
    unsafe { mem::transmute(argus_mouse_delta()) }
}

pub fn mouse_pos() -> Vector2d {
    unsafe { mem::transmute(argus_mouse_pos()) }
}

pub fn get_mouse_axis(axis: MouseAxis) -> f64 {
    unsafe { argus_get_mouse_axis(axis.into()) }
}

pub fn get_mouse_axis_delta(axis: MouseAxis) -> f64 {
    unsafe { argus_get_mouse_axis_delta(axis.into()) }
}

pub fn is_mouse_button_pressed(button: MouseButton) -> bool {
    unsafe { argus_is_mouse_button_pressed(button.into()) }
}
