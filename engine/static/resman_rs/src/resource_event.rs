use std::any::Any;
use core_rs::ArgusEvent;
use crate::{Resource, ResourcePrototype};

pub struct ResourceEvent {
    subtype: ResourceEventType,
    resource: Resource,
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[repr(u32)]
pub enum ResourceEventType {
    Load,
    Unload,
}

impl ArgusEvent for ResourceEvent {
    fn as_any_ref(&self) -> &dyn Any {
        self
    }
}

impl ResourceEvent {
    pub(crate) fn new(subtype: ResourceEventType, resource: Resource) -> Self {
        Self { subtype, resource }
    }

    pub fn get_subtype(&self) -> ResourceEventType {
        self.subtype
    }

    pub fn get_prototype(&self) -> &ResourcePrototype {
        self.resource.get_prototype()
    }

    pub fn get_resource(&mut self) -> &Resource {
        &self.resource
    }
}
