use core_rustabi::argus::core::{get_scripting_parameters, run_on_game_thread, LifecycleStage};
use resman_rustabi::argus::resman::ResourceManager;
use scripting_rs::{load_script_with_new_context, ScriptManager};
use crate::constants::LUA_MEDIA_TYPES;
use crate::loader::lua_script_loader::LuaScriptLoader;
use crate::lua_language_plugin::LuaLanguagePlugin;

#[no_mangle]
pub extern "C" fn update_lifecycle_scripting_lua_rs(
    stage: core_rustabi::core_cabi::LifecycleStage
) {
    match LifecycleStage::try_from(stage).unwrap() {
        LifecycleStage::Init => {
            ResourceManager::get_instance()
                .register_loader(Vec::from(LUA_MEDIA_TYPES), LuaScriptLoader {});
            ScriptManager::instance().register_language_plugin(LuaLanguagePlugin::new());
        }
        _ => {}
    }
}
