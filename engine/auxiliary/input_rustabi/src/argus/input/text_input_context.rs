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
use lowlevel_rustabi::argus::lowlevel::FfiWrapper;
use crate::input_cabi::*;

pub struct TextInputContext {
    handle: argus_text_input_context_t,
}

impl FfiWrapper for TextInputContext {
    fn of(handle: *mut c_void) -> Self {
        Self { handle }
    }
}

impl TextInputContext {
    pub fn new() -> Self {
        Self::of(unsafe { argus_text_input_context_create() })
    }

    pub fn get_current_text(&self) -> String {
        unsafe {
            CStr::from_ptr(argus_text_input_context_get_current_text(self.handle))
                .to_string_lossy().to_string()
        }
    }

    pub fn activate(&mut self) {
        unsafe { argus_text_input_context_activate(self.handle); }
    }

    pub fn deactivate(&mut self) {
        unsafe { argus_text_input_context_deactivate(self.handle); }
    }

    pub fn release(&mut self) {
        unsafe { argus_text_input_context_release(self.handle); }
    }

}
