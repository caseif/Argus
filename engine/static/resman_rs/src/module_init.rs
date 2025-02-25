use core_rustabi::argus::core::LifecycleStage;
use crate::ResourceManager;

#[no_mangle]
pub extern "C" fn update_lifecycle_resman_rs(stage: LifecycleStage) {
    match stage {
        LifecycleStage::PostInit => {
            ResourceManager::instance().discover_resources();
        }
        _ => {}
    }
}
