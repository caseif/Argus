use argus_core::{register_module, LifecycleStage};

#[register_module(id = "ui", depends(core, input, render, resman, wm))]
pub fn update_lifecycle_ui(stage: LifecycleStage) {
    //TODO
}
