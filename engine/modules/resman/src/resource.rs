use std::any::Any;
use std::sync::{Arc, Weak};
use arp::ResourceIdentifier;
use crate::ResourceManager;

#[derive(Clone, Debug)]
#[must_use]
pub struct Resource {
    pub(crate) prototype: ResourcePrototype,
    pub(crate) underlying: Arc<UnderlyingResource>,
}

pub struct WeakResource {
    prototype: ResourcePrototype,
    data: Weak<UnderlyingResource>,
}

#[derive(Debug)]
pub(crate) struct UnderlyingResource {
    prototype: ResourcePrototype,
    data: Box<dyn Any + Send + Sync>,
}

#[derive(Clone, Debug, Eq, Hash, PartialEq)]
pub struct ResourcePrototype {
    pub uid: ResourceIdentifier,
    pub media_type: String,
}

impl Resource {
    pub(crate) fn of(prototype: ResourcePrototype, obj: Box<dyn Any + Send + Sync>) -> Self {
        Self {
            prototype: prototype.clone(),
            underlying: Arc::new(UnderlyingResource { prototype, data: obj }),
        }
    }

    pub fn get_prototype(&self) -> &ResourcePrototype {
        &self.prototype
    }

    pub fn get_data(&self) -> &(dyn Any + Send + Sync) {
        self.underlying.data.as_ref()
    }

    pub fn get<T: 'static>(&self) -> Option<&T> {
        self.get_data().downcast_ref()
    }

    pub fn downgrade(&self) -> WeakResource {
        WeakResource {
            prototype: self.prototype.clone(),
            data: Arc::downgrade(&self.underlying),
        }
    }

    pub fn ref_count(&self) -> usize {
        Weak::strong_count(&self.downgrade().data)
    }
}

impl WeakResource {
    pub fn get_prototype(&self) -> &ResourcePrototype {
        &self.prototype
    }

    pub fn upgrade(&self) -> Option<Resource> {
        let arc = self.data.upgrade()?;
        Some(Resource {
            prototype: self.prototype.clone(),
            underlying: arc,
        })
    }
}

impl Drop for UnderlyingResource {
    fn drop(&mut self) {
        ResourceManager::instance().notify_unload(&self.prototype);
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub enum ResourceErrorReason {
    Generic,
    InvalidIdentifier,
    NotFound,
    NotLoaded,
    AlreadyLoaded,
    NoLoader,
    LoadFailed,
    MalformedContent,
    InvalidContent,
    UnsupportedContent,
    UnexpectedReferenceType,
}

#[derive(Clone, Debug)]
pub struct ResourceError {
    pub reason: ResourceErrorReason,
    pub uid: String,
    pub info: String,
}

impl ResourceError {
    pub fn new(reason: ResourceErrorReason, uid: impl Into<String>, info: impl Into<String>)
        -> Self {
        Self {
            reason,
            uid: uid.into(),
            info: info.into(),
        }
    }
}
