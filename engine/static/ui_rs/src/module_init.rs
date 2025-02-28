use core_rs::{register_module, LifecycleStage};

#[register_module(id = "ui", depends(core, input, render, resman, wm))]
pub fn update_lifecycle_ui_rs(stage: LifecycleStage) {
    //TODO
}
