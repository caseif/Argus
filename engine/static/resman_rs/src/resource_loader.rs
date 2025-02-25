use std::any::Any;
use std::io::Read;
use crate::{ResourceError, ResourceManager, ResourcePrototype};

pub trait ResourceLoader: Send + Sync {
    fn load_resource(
        &self,
        manager: &ResourceManager,
        prototype: &ResourcePrototype,
        read_callback: &mut dyn Read,
        size: u64,
    ) -> Result<Box<dyn Any + Send + Sync>, ResourceError>;

    fn deinit_resource(&self, obj: Box<dyn Any + Send + Sync>) {
        _ = obj;
    }
}
