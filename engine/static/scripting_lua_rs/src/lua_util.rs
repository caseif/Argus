use crate::context::ManagedLuaState;
use crate::lua_language_plugin::*;
use lua::bindings::*;
use std::ffi::CStr;
use std::rc::Rc;
use parking_lot::ReentrantMutex;

const REG_KEY_PLUGIN_PTR: &CStr = c"argus_plugin";
const REG_KEY_CONTEXT_DATA_PTR: &CStr = c"argus_context_data";

pub(crate) unsafe fn create_lua_state(plugin: &LuaLanguagePlugin)
    -> *mut lua_State {
    let state = luaL_newstate();
    if state.is_null() {
        panic!("Failed to create Lua state");
    }

    luaL_openlibs(state);

    state
}

pub(crate) fn destroy_lua_state(state: *mut lua_State) {
    unsafe { lua_close(state); }
}

pub(crate) fn to_managed_state(state: *mut lua_State) -> Rc<ReentrantMutex<ManagedLuaState>> {
    unsafe {
        lua_getfield(state, LUA_REGISTRYINDEX, REG_KEY_CONTEXT_DATA_PTR.as_ptr());
        let ret_val =
            (*lua_touserdata(state, -1).cast::<Rc<ReentrantMutex<ManagedLuaState>>>()).clone();
        lua_pop(state, 1);
        ret_val
    }
}
