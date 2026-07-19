#![feature(used_with_arg)]

use argus_logging::crate_logger;

crate_logger!(LOGGER, "argus/render_vulkan");

pub(crate) mod defines;
pub(crate) mod loader;
pub(crate) mod module_init;
pub(crate) mod renderer;
pub(crate) mod state;

mod resources;
