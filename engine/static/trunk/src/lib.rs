include!(concat!(env!("OUT_DIR"), "/module_defs.rs"));

pub mod engine;
pub mod event;
pub mod module;

use std::sync::RwLock;

use crate::engine::kill_game_thread;
use crate::module::LifecycleStage;
use crate::module::register_module;
use lowlevel::logging::Logger;

use lazy_static::lazy_static;
use static_init::constructor;

const MODULE_CORE: &str = "core";

lazy_static! {
    pub(crate) static ref logger: Logger = Logger::new(MODULE_CORE);

    pub(crate) static ref g_trunk_initializing: RwLock<bool> = RwLock::new(false);
    pub(crate) static ref g_trunk_initialized: RwLock<bool> = RwLock::new(false);
}

#[constructor(0)]
extern "C" fn init_trunk() {
    register_module(MODULE_CORE, update_lifecycle_core);
}

fn update_lifecycle_core(stage: LifecycleStage) {
    match stage {
        LifecycleStage::PreInit => {
            logger.fatal_if(!*(*g_trunk_initializing).read().unwrap() && !*(*g_trunk_initialized).read().unwrap(),
                "Cannot initialize engine more than once.");

            *(*g_trunk_initializing).write().unwrap() = true;
        }
        LifecycleStage::Init => {
            *(*g_trunk_initialized).write().unwrap() = true;
        }
        LifecycleStage::PostDeinit => {
            kill_game_thread();
        }
        _ => ()
    }
}
