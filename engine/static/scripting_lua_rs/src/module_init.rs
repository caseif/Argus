use core_rs::{register_module, LifecycleStage};
use resman_rs::ResourceManager;
use scripting_rs::ScriptManager;
use crate::constants::LUA_MEDIA_TYPES;
use crate::loader::lua_script_loader::LuaScriptLoader;
use crate::lua_language_plugin::LuaLanguagePlugin;

#[register_module(id = "scripting_lua", depends(core, resman, scripting))]
pub fn update_lifecycle_scripting_lua_rs(
    stage: LifecycleStage
) {
    match stage {
        LifecycleStage::Init => {
            ResourceManager::instance()
                .register_loader(Vec::from(LUA_MEDIA_TYPES), LuaScriptLoader {});
            ScriptManager::instance().register_language_plugin(LuaLanguagePlugin::new());
        }
        _ => {}
    }
}
