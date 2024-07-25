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

use crate::core_cabi::*;
use lowlevel_rustabi::util::*;

pub fn get_client_id() -> String {
    unsafe {
        return cstr_to_string(argus_get_client_id());
    }
}

pub fn set_client_id(id: &str) {
    unsafe {
        let id_c = str_to_cstring(id);
        argus_set_client_id(id_c.as_ptr());
    }
}

pub fn get_client_name() -> String {
    unsafe {
        return cstr_to_string(argus_get_client_name());
    }
}

pub fn set_client_name(id: &str) {
    unsafe {
        let id_c = str_to_cstring(id);
        argus_set_client_name(id_c.as_ptr());
    }
}

pub fn get_client_version() -> String {
    unsafe {
        return cstr_to_string(argus_get_client_version());
    }
}

pub fn set_client_version(id: &str) {
    unsafe {
        let id_c = str_to_cstring(id);
        argus_set_client_version(id_c.as_ptr());
    }
}
