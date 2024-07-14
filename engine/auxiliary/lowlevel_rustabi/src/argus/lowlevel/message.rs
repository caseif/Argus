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

use crate::lowlevel_cabi::*;
use crate::util::*;

pub type MessageDispatcher = message_dispatcher_t;

pub trait Message {
    fn get_message_type_id() -> &'static str;
}

pub fn set_message_dispatcher(dispatcher: MessageDispatcher) {
    unsafe {
        argus_set_message_dispatcher(dispatcher);
    }
}

pub fn broadcast_message<T: Message>(message: &T) {
    unsafe {
        argus_broadcast_message(
            str_to_cstring(T::get_message_type_id()).as_ptr(),
            &message as *const _ as *const std::ffi::c_void,
        );
    }
}
