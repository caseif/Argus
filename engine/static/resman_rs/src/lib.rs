mod mappings;
mod module_init;
mod resource;
mod resource_event;
mod resource_loader;
mod resource_manager;

pub use module_init::update_lifecycle_resman_rs;
pub use resource::*;
pub use resource_event::*;
pub use resource_loader::*;
pub use resource_manager::*;

pub use arp::ResourceIdentifier;

argus_logging::crate_logger!(LOGGER, "resman_rs");
