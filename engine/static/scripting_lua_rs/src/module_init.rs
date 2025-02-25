use core_rustabi::argus::core::LifecycleStage;
use resman_rs::ResourceManager;
use scripting_rs::ScriptManager;
use crate::constants::LUA_MEDIA_TYPES;
use crate::loader::lua_script_loader::LuaScriptLoader;
use crate::lua_language_plugin::LuaLanguagePlugin;

#[no_mangle]
pub extern "C" fn update_lifecycle_scripting_lua_rs(
    stage: core_rustabi::core_cabi::LifecycleStage
) {
    match LifecycleStage::try_from(stage).unwrap() {
        LifecycleStage::Init => {
            ResourceManager::instance()
                .register_loader(Vec::from(LUA_MEDIA_TYPES), LuaScriptLoader {});
            ScriptManager::instance().register_language_plugin(LuaLanguagePlugin::new());
        }
        _ => {}
    }
}
