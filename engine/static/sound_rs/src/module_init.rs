use core_rs::{register_module, LifecycleStage};

#[register_module(id = "sound", depends(core, wm))]
pub fn update_lifecycle_sound_rs(stage: LifecycleStage) {
    //TODO
}
