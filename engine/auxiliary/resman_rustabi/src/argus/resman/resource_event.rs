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
use core_rustabi::argus::core::ArgusFfiEvent;
use core_rustabi::core_cabi::argus_event_t;
use num_enum::{IntoPrimitive, TryFromPrimitive};

use crate::argus::resman::{unwrap_resource_prototype, Resource, ResourcePrototype};
use crate::resman_cabi::*;

#[derive(Eq, Ord, PartialEq, PartialOrd, IntoPrimitive, TryFromPrimitive)]
#[repr(u32)]
pub enum ResourceEventType {
    Load = RESOURCE_EVENT_TYPE_LOAD,
    Unload = RESOURCE_EVENT_TYPE_UNLOAD,
}

pub struct ResourceEvent {
    handle: argus_resource_event_t,
}

impl ResourceEvent {
    pub fn of(handle: argus_resource_event_t) -> Self {
        Self { handle }
    }

    pub fn get_subtype(&self) -> ResourceEventType {
        unsafe {
            ResourceEventType::try_from(argus_resource_event_get_subtype(self.handle))
                .expect("Invalid ResourceEventType ordinal")
        }
    }

    pub fn get_prototype(&self) -> ResourcePrototype {
        unsafe { unwrap_resource_prototype(argus_resource_event_get_prototype(self.handle)) }
    }

    pub fn get_resource(&mut self) -> Resource {
        unsafe { Resource::of(argus_resource_event_get_resource(self.handle)) }
    }
}

impl ArgusFfiEvent for ResourceEvent {
    fn get_type_id() -> &'static str {
        "resource"
    }

    fn of(handle: argus_event_t) -> Self {
        Self { handle }
    }

    fn get_handle(&self) -> argus_event_t {
        self.handle
    }
}
