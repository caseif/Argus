use argus_core::{register_module, LifecycleStage};
use crate::ResourceManager;

#[register_module(id = "resman", depends(core))]
pub fn update_lifecycle_resman(stage: LifecycleStage) {
    match stage {
        LifecycleStage::PostInit => {
            ResourceManager::instance().discover_resources();
        }
        _ => {}
    }
}
