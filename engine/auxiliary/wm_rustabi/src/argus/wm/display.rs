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

use std::ptr::null_mut;

use lowlevel_rustabi::argus::lowlevel::Vector2i;
use lowlevel_rustabi::util::cstr_to_string;

use crate::wm_cabi::*;

pub struct Display {
    handle: argus_display_const_t,
}

pub type DisplayMode = argus_display_mode_t;

impl Display {
    pub(crate) fn of(handle: argus_display_const_t) -> Self {
        return Self { handle };
    }

    pub(crate) fn get_handle(&self) -> argus_display_const_t {
        return self.handle;
    }

    pub fn get_available_displays() -> Vec<Self> {
        unsafe {
            let mut count = 0;
            argus_display_get_available_displays(&mut count, null_mut());
            let mut displays = Vec::<argus_display_const_t>::new();
            displays.reserve(count);

            argus_display_get_available_displays(null_mut(), displays.as_mut_ptr());

            return displays.into_iter().map(|disp| Display::of(disp as argus_display_t)).collect();
        }
    }

    pub fn get_name(&self) -> String {
        unsafe {
            return cstr_to_string(argus_display_get_name(self.get_handle()));
        }
    }

    pub fn get_position(&self) -> Vector2i {
        unsafe {
            return argus_display_get_position(self.get_handle());
        }
    }

    pub fn get_display_modes(&self) -> Vec<DisplayMode> {
        unsafe {
            let mut count = 0;
            argus_display_get_display_modes(self.get_handle(), &mut count, null_mut());
            let mut modes = Vec::<DisplayMode>::new();
            modes.reserve(count);
            argus_display_get_display_modes(self.get_handle(), null_mut(), modes.as_mut_ptr());
            return modes;
        }
    }
}
