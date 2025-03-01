use argus_core::{register_module, LifecycleStage};

#[register_module(id = "sound", depends(core, wm))]
pub fn update_lifecycle_sound(_stage: LifecycleStage) {
    //TODO
}
