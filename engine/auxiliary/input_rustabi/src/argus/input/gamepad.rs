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
use std::ffi::CStr;
use num_enum::{IntoPrimitive, UnsafeFromPrimitive};
use crate::input_cabi::*;

pub type HidDeviceId = ArgusHidDeviceId;

#[repr(i32)]
#[derive(Clone, Debug, IntoPrimitive, UnsafeFromPrimitive)]
pub enum GamepadButton {
    Unknown = GAMEPAD_BUTTON_UNKNOWN,
    A = GAMEPAD_BUTTON_A,
    B = GAMEPAD_BUTTON_B,
    X = GAMEPAD_BUTTON_X,
    Y = GAMEPAD_BUTTON_Y,
    DpadUp = GAMEPAD_BUTTON_DPAD_UP,
    DpadDown = GAMEPAD_BUTTON_DPAD_DOWN,
    DpadLeft = GAMEPAD_BUTTON_DPAD_LEFT,
    DpadRight = GAMEPAD_BUTTON_DPAD_RIGHT,
    LBumper = GAMEPAD_BUTTON_L_BUMPER,
    RBumper = GAMEPAD_BUTTON_R_BUMPER,
    LTrigger = GAMEPAD_BUTTON_L_TRIGGER,
    RTrigger = GAMEPAD_BUTTON_R_TRIGGER,
    LStick = GAMEPAD_BUTTON_L_STICK,
    RStick = GAMEPAD_BUTTON_R_STICK,
    L4 = GAMEPAD_BUTTON_L4,
    R4 = GAMEPAD_BUTTON_R4,
    L5 = GAMEPAD_BUTTON_L5,
    R5 = GAMEPAD_BUTTON_R5,
    Start = GAMEPAD_BUTTON_START,
    Back = GAMEPAD_BUTTON_BACK,
    Guide = GAMEPAD_BUTTON_GUIDE,
    Misc1 = GAMEPAD_BUTTON_MISC_1,
    MaxValue = GAMEPAD_BUTTON_MAX_VALUE,
}

#[repr(i32)]
#[derive(Clone, Debug, IntoPrimitive, UnsafeFromPrimitive)]
pub enum GamepadAxis {
    Unknown = GAMEPAD_AXIS_UNKNOWN,
    LeftX = GAMEPAD_AXIS_LEFT_X,
    LeftY = GAMEPAD_AXIS_LEFT_Y,
    RightX = GAMEPAD_AXIS_RIGHT_X,
    RightY = GAMEPAD_AXIS_RIGHT_Y,
    LTrigger = GAMEPAD_AXIS_L_TRIGGER,
    RTrigger = GAMEPAD_AXIS_R_TRIGGER,
    MaxValue = GAMEPAD_AXIS_MAX_VALUE,
}

pub fn get_connected_gamepad_count() -> u8 {
    unsafe { argus_get_connected_gamepad_count() }
}

pub fn get_unattached_gamepad_count() -> u8 {
    unsafe { argus_get_unattached_gamepad_count() }
}

pub fn get_gamepad_name(gamepad: HidDeviceId) -> String {
    unsafe { CStr::from_ptr(argus_get_gamepad_name(gamepad)).to_string_lossy().to_string() }
}

pub fn is_gamepad_button_pressed(gamepad: HidDeviceId, button: GamepadButton) -> bool {
    unsafe { argus_is_gamepad_button_pressed(gamepad, button.into()) }
}

pub fn get_gamepad_axis(gamepad: HidDeviceId, axis: GamepadAxis) -> f64 {
    unsafe { argus_get_gamepad_axis(gamepad, axis.into()) }
}

pub fn get_gamepad_axis_delta(gamepad: HidDeviceId, axis: GamepadAxis) -> f64 {
    unsafe { argus_get_gamepad_axis_delta(gamepad, axis.into()) }
}
