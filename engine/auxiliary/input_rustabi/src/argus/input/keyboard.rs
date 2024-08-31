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
use num_enum::{IntoPrimitive, TryFromPrimitive};
use crate::input_cabi::*;

#[repr(u32)]
#[derive(Clone, Debug, IntoPrimitive, TryFromPrimitive)]
pub enum KeyboardScancode {
    Unknown = KB_SCANCODE_UNKNOWN,
    A = KB_SCANCODE_A,
    B = KB_SCANCODE_B,
    C = KB_SCANCODE_C,
    D = KB_SCANCODE_D,
    E = KB_SCANCODE_E,
    F = KB_SCANCODE_F,
    G = KB_SCANCODE_G,
    H = KB_SCANCODE_H,
    I = KB_SCANCODE_I,
    J = KB_SCANCODE_J,
    K = KB_SCANCODE_K,
    L = KB_SCANCODE_L,
    M = KB_SCANCODE_M,
    N = KB_SCANCODE_N,
    O = KB_SCANCODE_O,
    P = KB_SCANCODE_P,
    Q = KB_SCANCODE_Q,
    R = KB_SCANCODE_R,
    S = KB_SCANCODE_S,
    T = KB_SCANCODE_T,
    U = KB_SCANCODE_U,
    V = KB_SCANCODE_V,
    W = KB_SCANCODE_W,
    X = KB_SCANCODE_X,
    Y = KB_SCANCODE_Y,
    Z = KB_SCANCODE_Z,
    Number1 = KB_SCANCODE_NUMBER_1,
    Number2 = KB_SCANCODE_NUMBER_2,
    Number3 = KB_SCANCODE_NUMBER_3,
    Number4 = KB_SCANCODE_NUMBER_4,
    Number5 = KB_SCANCODE_NUMBER_5,
    Number6 = KB_SCANCODE_NUMBER_6,
    Number7 = KB_SCANCODE_NUMBER_7,
    Number8 = KB_SCANCODE_NUMBER_8,
    Number9 = KB_SCANCODE_NUMBER_9,
    Number0 = KB_SCANCODE_NUMBER_0,
    Enter = KB_SCANCODE_ENTER,
    Escape = KB_SCANCODE_ESCAPE,
    Backspace = KB_SCANCODE_BACKSPACE,
    Tab = KB_SCANCODE_TAB,
    Space = KB_SCANCODE_SPACE,
    Minus = KB_SCANCODE_MINUS,
    Equals = KB_SCANCODE_EQUALS,
    LeftBracket = KB_SCANCODE_LEFT_BRACKET,
    RightBracket = KB_SCANCODE_RIGHT_BRACKET,
    BackSlash = KB_SCANCODE_BACK_SLASH,
    Semicolon = KB_SCANCODE_SEMICOLON,
    Apostrophe = KB_SCANCODE_APOSTROPHE,
    Grave = KB_SCANCODE_GRAVE,
    Comma = KB_SCANCODE_COMMA,
    Period = KB_SCANCODE_PERIOD,
    ForwardSlash = KB_SCANCODE_FORWARD_SLASH,
    CapsLock = KB_SCANCODE_CAPS_LOCK,
    F1 = KB_SCANCODE_F1,
    F2 = KB_SCANCODE_F2,
    F3 = KB_SCANCODE_F3,
    F4 = KB_SCANCODE_F4,
    F6 = KB_SCANCODE_F6,
    F7 = KB_SCANCODE_F7,
    F8 = KB_SCANCODE_F8,
    F5 = KB_SCANCODE_F5,
    F9 = KB_SCANCODE_F9,
    F10 = KB_SCANCODE_F10,
    F11 = KB_SCANCODE_F11,
    F12 = KB_SCANCODE_F12,
    PrintScreen = KB_SCANCODE_PRINT_SCREEN,
    ScrollLock = KB_SCANCODE_SCROLL_LOCK,
    Pause = KB_SCANCODE_PAUSE,
    Insert = KB_SCANCODE_INSERT,
    Home = KB_SCANCODE_HOME,
    PageUp = KB_SCANCODE_PAGE_UP,
    Delete = KB_SCANCODE_DELETE,
    End = KB_SCANCODE_END,
    PageDown = KB_SCANCODE_PAGE_DOWN,
    ArrowRight = KB_SCANCODE_ARROW_RIGHT,
    ArrowLeft = KB_SCANCODE_ARROW_LEFT,
    ArrowDown = KB_SCANCODE_ARROW_DOWN,
    ArrowUp = KB_SCANCODE_ARROW_UP,
    NumpadNumLock = KB_SCANCODE_NUMPAD_NUM_LOCK,
    NumpadDivide = KB_SCANCODE_NUMPAD_DIVIDE,
    NumpadTimes = KB_SCANCODE_NUMPAD_TIMES,
    NumpadMinus = KB_SCANCODE_NUMPAD_MINUS,
    NumpadPlus = KB_SCANCODE_NUMPAD_PLUS,
    NumpadEnter = KB_SCANCODE_NUMPAD_ENTER,
    Numpad1 = KB_SCANCODE_NUMPAD_1,
    Numpad2 = KB_SCANCODE_NUMPAD_2,
    Numpad3 = KB_SCANCODE_NUMPAD_3,
    Numpad4 = KB_SCANCODE_NUMPAD_4,
    Numpad5 = KB_SCANCODE_NUMPAD_5,
    Numpad6 = KB_SCANCODE_NUMPAD_6,
    Numpad7 = KB_SCANCODE_NUMPAD_7,
    Numpad8 = KB_SCANCODE_NUMPAD_8,
    Numpad9 = KB_SCANCODE_NUMPAD_9,
    Numpad0 = KB_SCANCODE_NUMPAD_0,
    NumpadDot = KB_SCANCODE_NUMPAD_DOT,
    NumpadEquals = KB_SCANCODE_NUMPAD_EQUALS,
    Menu = KB_SCANCODE_MENU,
    LeftControl = KB_SCANCODE_LEFT_CONTROL,
    LeftShift = KB_SCANCODE_LEFT_SHIFT,
    LeftAlt = KB_SCANCODE_LEFT_ALT,
    Super = KB_SCANCODE_SUPER,
    RightControl = KB_SCANCODE_RIGHT_CONTROL,
    RightShift = KB_SCANCODE_RIGHT_SHIFT,
    RightAlt = KB_SCANCODE_RIGHT_ALT,
}

#[repr(u32)]
#[derive(Clone, Debug, IntoPrimitive, TryFromPrimitive)]
pub enum KeyboardCommand {
    Escape = KB_COMMAND_ESCAPE,
    F1 = KB_COMMAND_F1,
    F2 = KB_COMMAND_F2,
    F3 = KB_COMMAND_F3,
    F4 = KB_COMMAND_F4,
    F5 = KB_COMMAND_F5,
    F6 = KB_COMMAND_F6,
    F7 = KB_COMMAND_F7,
    F8 = KB_COMMAND_F8,
    F9 = KB_COMMAND_F9,
    F10 = KB_COMMAND_F10,
    F11 = KB_COMMAND_F11,
    F12 = KB_COMMAND_F12,
    Backspace = KB_COMMAND_BACKSPACE,
    Tab = KB_COMMAND_TAB,
    CapsLock = KB_COMMAND_CAPS_LOCK,
    Enter = KB_COMMAND_ENTER,
    Menu = KB_COMMAND_MENU,
    PrintScreen = KB_COMMAND_PRINT_SCREEN,
    ScrollLock = KB_COMMAND_SCROLL_LOCK,
    Break = KB_COMMAND_BREAK,
    Insert = KB_COMMAND_INSERT,
    Home = KB_COMMAND_HOME,
    PageUp = KB_COMMAND_PAGE_UP,
    Delete = KB_COMMAND_DELETE,
    End = KB_COMMAND_END,
    PageDown = KB_COMMAND_PAGE_DOWN,
    ArrowUp = KB_COMMAND_ARROW_UP,
    ArrowLeft = KB_COMMAND_ARROW_LEFT,
    ArrowDown = KB_COMMAND_ARROW_DOWN,
    ArrowRight = KB_COMMAND_ARROW_RIGHT,
    NumpadNumLock = KB_COMMAND_NUMPAD_NUM_LOCK,
    NumpadEnter = KB_COMMAND_NUMPAD_ENTER,
    NumpadDot = KB_COMMAND_NUMPAD_DOT,
    Super = KB_COMMAND_SUPER,
}

#[repr(u32)]
#[derive(Clone, Debug, IntoPrimitive, TryFromPrimitive)]
pub enum KeyboardModifiers {
    None = KB_MODIFIER_NONE,
    Shift = KB_MODIFIER_SHIFT,
    Control = KB_MODIFIER_CONTROL,
    Super = KB_MODIFIER_SUPER,
    Alt = KB_MODIFIER_ALT,
}

pub fn get_key_name(scancode: KeyboardScancode) -> String {
    unsafe { CStr::from_ptr(argus_get_key_name(scancode.into())).to_string_lossy().to_string() }
}

pub fn is_key_pressed(scancode: KeyboardScancode) -> bool {
    unsafe { argus_is_key_pressed(scancode.into()) }
}
