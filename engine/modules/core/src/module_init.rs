use crate::{EngineManager, LifecycleStage};
use crate::register_module;

#[register_module(crate = crate, id = "core")]
pub fn update_lifecycle_core(stage: LifecycleStage) {
    match stage {
        LifecycleStage::Init => {
            EngineManager::instance().commit_config();
        }
        _ => {}
    }
}
