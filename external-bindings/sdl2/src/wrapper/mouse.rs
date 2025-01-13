use std::collections::HashSet;
use std::ptr::addr_of_mut;
use num_enum::{IntoPrimitive, TryFromPrimitive};
use crate::bindings::*;

#[allow(clippy::unnecessary_cast)]
#[derive(Clone, Copy, Debug, Eq, Hash, IntoPrimitive, PartialEq, TryFromPrimitive)]
#[repr(u8)]
pub enum SdlMouseButton {
    Left = SDL_BUTTON_LEFT as u8,
    Middle = SDL_BUTTON_MIDDLE as u8,
    Right = SDL_BUTTON_RIGHT as u8,
    X1 = SDL_BUTTON_X1 as u8,
    X2 = SDL_BUTTON_X2 as u8,
}

impl SdlMouseButton {
    pub fn get_mask(&self) -> u8 {
        (1 << (u8::from(*self) - 1)) as u8
    }
}

#[allow(clippy::unnecessary_cast)]
#[derive(Clone, Copy, Debug, Eq, Hash, IntoPrimitive, PartialEq, TryFromPrimitive)]
#[repr(u32)]
pub enum SdlMouseWheelDirection {
    /// The scroll direction is flipped / natural
    Flipped = SDL_MOUSEWHEEL_FLIPPED as u32,
    /// The scroll direction is normal
    Normal = SDL_MOUSEWHEEL_NORMAL as u32,
}

pub struct SdlMouseState {
    pub position_x: i32,
    pub position_y: i32,
    pub pressed_buttons: HashSet<SdlMouseButton>,
}

pub fn sdl_get_mouse_state() -> SdlMouseState {
    let mut pos_x: i32 = 0;
    let mut pos_y: i32 = 0;
    let mut pressed_buttons = HashSet::new();
    let button_field = unsafe { SDL_GetMouseState(addr_of_mut!(pos_x), addr_of_mut!(pos_y)) };
    for button in &[
        SdlMouseButton::Left,
        SdlMouseButton::Middle,
        SdlMouseButton::Right,
        SdlMouseButton::X1,
        SdlMouseButton::X2
    ] {
        if (button_field & button.get_mask() as u32) != 0 {
            pressed_buttons.insert(*button);
        }
    }
    SdlMouseState {
        position_x: pos_x,
        position_y: pos_y,
        pressed_buttons,
    }
}
