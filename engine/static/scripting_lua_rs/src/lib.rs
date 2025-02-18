use argus_logging::crate_logger;

pub(crate) mod constants;
pub(crate) mod context;
pub(crate) mod lua_language_plugin;
pub(crate) mod loaded_script;
pub(crate) mod module_init;
pub(crate) mod loader;
mod handles;
mod lua_util;

pub use module_init::update_lifecycle_scripting_lua_rs;

crate_logger!(LOGGER, "argus/scripting_lua_rs");
