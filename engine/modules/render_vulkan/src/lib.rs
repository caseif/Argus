#![feature(used_with_arg)]
#![feature(hash_extract_if)] //TODO: remove once Rust 1.87 is out

pub(crate) mod loader;
pub(crate) mod module_init;
pub(crate) mod renderer;
pub(crate) mod setup;
pub(crate) mod state;
pub(crate) mod util;
mod resources;
