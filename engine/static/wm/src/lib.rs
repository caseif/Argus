use static_init::{constructor, destructor};

#[constructor(0)]
extern "C" fn register_wm() {
    trunk::module::register_module("wm", update_lifecycle);
}

fn update_lifecycle(engine: &trunk::engine::EngineHandle, stage: trunk::module::LifecycleStage) {
    //TODO
}
