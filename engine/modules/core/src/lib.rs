#![feature(used_with_arg)]

// public modules
mod buffered_map;
mod engine;
mod error;
mod event;
mod manager;
mod module;
mod screen_space;

pub use buffered_map::*;
pub use engine::*;
pub use error::*;
pub use event::*;
pub use manager::*;
pub use module::*;
pub use screen_space::*;

pub mod internal;

// private modules
mod module_init;

// re-exports
pub use argus_core_macros::*;

// globals
argus_logging::crate_logger!(LOGGER, "argus/core");
