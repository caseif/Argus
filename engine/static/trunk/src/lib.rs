include!(concat!(env!("OUT_DIR"), "/module_defs.rs"));

pub mod engine;
pub mod event;
pub mod module;

use crate::engine::EngineHandle;
use crate::module::LifecycleStage;
use crate::module::register_module;

use lowlevel::logging::Logger;

use static_init::constructor;

const MODULE_TRUNK: &str = "trunk";

pub(crate) static LOGGER: Logger = Logger::new(MODULE_TRUNK);

#[constructor(0)]
extern "C" fn init_trunk() {
    register_module(MODULE_TRUNK, update_lifecycle_trunk);
}

fn update_lifecycle_trunk(engine: &EngineHandle, stage: LifecycleStage) {
    match stage {
        LifecycleStage::PostDeinit => {
            //kill_game_thread();
        }
        _ => ()
    }
}
