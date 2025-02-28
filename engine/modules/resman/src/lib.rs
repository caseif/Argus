#![feature(used_with_arg)]

mod mappings;
mod module_init;
mod resource;
mod resource_event;
mod resource_loader;
mod resource_manager;

pub use resource::*;
pub use resource_event::*;
pub use resource_loader::*;
pub use resource_manager::*;

pub use arp::ResourceIdentifier;

argus_logging::crate_logger!(LOGGER, "argus/resman");
