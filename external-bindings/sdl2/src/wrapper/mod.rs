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

#![allow(unused_imports)]

use bitflags::bitflags;
use crate::bindings::{SDL_Init, SDL_InitSubSystem, SDL_Quit, SDL_QuitSubSystem, SDL_WasInit, SDL_TRUE};

pub mod error;
pub mod events;
pub mod gamecontroller;
pub mod hints;
pub mod joystick;
pub mod keyboard;
pub mod mouse;
pub mod rect;
pub mod video;

bitflags! {
    #[derive(Clone, Copy)]
    pub struct SdlInitFlags: u32 {
        const Timer = crate::bindings::SDL_INIT_TIMER;
        const Audio = crate::bindings::SDL_INIT_AUDIO;
        const Video = crate::bindings::SDL_INIT_VIDEO;
        const Joystick = crate::bindings::SDL_INIT_JOYSTICK;
        const Haptic = crate::bindings::SDL_INIT_HAPTIC;
        const GameController = crate::bindings::SDL_INIT_GAMECONTROLLER;
        const Events = crate::bindings::SDL_INIT_EVENTS;
        const Sensor = crate::bindings::SDL_INIT_SENSOR;
        const NoParachute = crate::bindings::SDL_INIT_NOPARACHUTE;
        const Everything = crate::bindings::SDL_INIT_EVERYTHING;
    }
}

pub fn sdl_init(flags: SdlInitFlags) -> i32 {
    unsafe { SDL_Init(flags.bits()) }
}
pub fn sdl_init_subsystem(flags: SdlInitFlags) -> i32 {
    unsafe { SDL_InitSubSystem(flags.bits()) }
}
pub fn sdl_quit_subsystem(flags: SdlInitFlags) {
    unsafe { SDL_QuitSubSystem(flags.bits()) };
}
pub fn sdl_was_init(flags: SdlInitFlags) -> bool {
    unsafe { SDL_WasInit(flags.bits()) == SDL_TRUE.into() }
}
pub fn sdl_quit() {
    unsafe { SDL_Quit() };
}
