use crate::register_module;
use crate::{ClientConfig, CoreConfig, EngineManager, LifecycleStage};

const CONFIG_KEY_CLIENT: &str = "client";
const CONFIG_KEY_CORE: &str = "core";

#[register_module(crate = crate, id = "core")]
pub fn update_lifecycle_core(stage: LifecycleStage) {
    match stage {
        LifecycleStage::PreInit => {
            EngineManager::instance().add_config_deserializer::<ClientConfig>(CONFIG_KEY_CLIENT);
            EngineManager::instance().add_config_deserializer::<CoreConfig>(CONFIG_KEY_CORE);
        }
        LifecycleStage::Init => {
            EngineManager::instance().load_config(None).unwrap();
        }
        _ => {}
    }
}
